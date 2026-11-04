#include "ShadPasskeyBackend.h"

#include <QByteArray>
#include <QChar>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QThread>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <string>
#include <vector>

#include "ShadDef.h"
#include "ShadPasskey.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <webauthn.h>

namespace
{
    struct WebAuthnApi
    {
        decltype(&WebAuthNAuthenticatorMakeCredential) makeCredential{nullptr};
        decltype(&WebAuthNAuthenticatorGetAssertion) getAssertion{nullptr};
        decltype(&WebAuthNFreeCredentialAttestation) freeCredentialAttestation{nullptr};
        decltype(&WebAuthNFreeAssertion) freeAssertion{nullptr};
        decltype(&WebAuthNGetCancellationId) getCancellationId{nullptr};
        decltype(&WebAuthNCancelCurrentOperation) cancelCurrentOperation{nullptr};
        decltype(&WebAuthNGetErrorName) getErrorName{nullptr};

        bool isLoaded() const
        {
            return makeCredential != nullptr && getAssertion != nullptr && freeCredentialAttestation != nullptr && freeAssertion != nullptr;
        }
    };

    template <typename Function>
    Function resolveFunction(HMODULE module, const char* name)
    {
        return reinterpret_cast<Function>(reinterpret_cast<void*>(GetProcAddress(module, name)));
    }

    const WebAuthnApi& webAuthnApi()
    {
        static const WebAuthnApi api = []()
        {
            WebAuthnApi result;
            const HMODULE module = LoadLibraryExW(L"webauthn.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (module == nullptr)
            {
                return result;
            }
            result.makeCredential = resolveFunction<decltype(result.makeCredential)>(module, "WebAuthNAuthenticatorMakeCredential");
            result.getAssertion = resolveFunction<decltype(result.getAssertion)>(module, "WebAuthNAuthenticatorGetAssertion");
            result.freeCredentialAttestation = resolveFunction<decltype(result.freeCredentialAttestation)>(module, "WebAuthNFreeCredentialAttestation");
            result.freeAssertion = resolveFunction<decltype(result.freeAssertion)>(module, "WebAuthNFreeAssertion");
            result.getCancellationId = resolveFunction<decltype(result.getCancellationId)>(module, "WebAuthNGetCancellationId");
            result.cancelCurrentOperation = resolveFunction<decltype(result.cancelCurrentOperation)>(module, "WebAuthNCancelCurrentOperation");
            result.getErrorName = resolveFunction<decltype(result.getErrorName)>(module, "WebAuthNGetErrorName");
            return result;
        }();
        return api;
    }

    QByteArray clientDataJson(const char* type, const QByteArray& challenge, const QString& rpId)
    {
        const QByteArray encodedChallenge = challenge.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        return QByteArrayLiteral("{\"type\":\"") + type + QByteArrayLiteral("\",\"challenge\":\"") + encodedChallenge + QByteArrayLiteral("\",\"origin\":\"https://") + rpId.toUtf8() + QByteArrayLiteral("\",\"crossOrigin\":false}");
    }

    QByteArray toByteArray(const BYTE* data, DWORD size)
    {
        if (data == nullptr || size == 0)
        {
            return QByteArray();
        }
        return QByteArray(reinterpret_cast<const char*>(data), static_cast<qsizetype>(size));
    }

    PBYTE toBytePointer(const QByteArray& data)
    {
        return const_cast<PBYTE>(reinterpret_cast<const BYTE*>(data.constData()));
    }

    std::vector<WEBAUTHN_CREDENTIAL> credentialList(const QList<QByteArray>& ids)
    {
        std::vector<WEBAUTHN_CREDENTIAL> result;
        result.reserve(static_cast<size_t>(ids.size()));
        for (const QByteArray& id : ids)
        {
            WEBAUTHN_CREDENTIAL credential{};
            credential.dwVersion = WEBAUTHN_CREDENTIAL_CURRENT_VERSION;
            credential.cbId = static_cast<DWORD>(id.size());
            credential.pbId = toBytePointer(id);
            credential.pwszCredentialType = WEBAUTHN_CREDENTIAL_TYPE_PUBLIC_KEY;
            result.push_back(credential);
        }
        return result;
    }

    DWORD userVerificationRequirement(ShadPasskeyType::UserVerification userVerification)
    {
        switch (userVerification)
        {
            case ShadPasskeyType::Required:
                return WEBAUTHN_USER_VERIFICATION_REQUIREMENT_REQUIRED;
            case ShadPasskeyType::Discouraged:
                return WEBAUTHN_USER_VERIFICATION_REQUIREMENT_DISCOURAGED;
            case ShadPasskeyType::Preferred:
                break;
        }
        return WEBAUTHN_USER_VERIFICATION_REQUIREMENT_PREFERRED;
    }

    DWORD timeoutMilliseconds(int timeout)
    {
        return timeout > 0 ? static_cast<DWORD>(timeout) : 0;
    }

    void postError(QObject* context, const ShadPasskeyBackend::ErrorCallback& onError, HRESULT result)
    {
        ShadPasskeyType::Error error = ShadPasskeyType::Other;
        QString detail;
        if (result == NTE_USER_CANCELLED || result == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            error = ShadPasskeyType::Cancelled;
        }
        else if (result == HRESULT_FROM_WIN32(ERROR_TIMEOUT))
        {
            error = ShadPasskeyType::Timeout;
        }
        else if (result == NTE_NOT_SUPPORTED)
        {
            error = ShadPasskeyType::NotSupported;
        }
        else
        {
            const WebAuthnApi& api = webAuthnApi();
            const QString name = api.getErrorName != nullptr ? QString::fromWCharArray(api.getErrorName(result)) : QString();
            const QString code = QStringLiteral("0x%1").arg(static_cast<quint32>(result), 8, 16, QLatin1Char('0'));
            detail = name.isEmpty() ? code : QStringLiteral("%1 (%2)").arg(name, code);
        }
        QMetaObject::invokeMethod(context, [onError, error, detail]() { onError(error, detail); }, Qt::QueuedConnection);
    }

    HWND windowHandle(QWidget* window)
    {
        if (window != nullptr)
        {
            return reinterpret_cast<HWND>(window->window()->winId());
        }
        return GetForegroundWindow();
    }
}

struct ShadPasskeyBackend::Impl
{
    QObject context;
    QThread* thread{nullptr};
    GUID cancellationId{};
    bool hasCancellationId{false};

    void prepare()
    {
        if (thread != nullptr)
        {
            thread->wait();
            delete thread;
            thread = nullptr;
        }
        const WebAuthnApi& api = webAuthnApi();
        hasCancellationId = api.getCancellationId != nullptr && api.cancelCurrentOperation != nullptr && SUCCEEDED(api.getCancellationId(&cancellationId));
    }
};

ShadPasskeyBackend::ShadPasskeyBackend()
    : _impl(std::make_unique<Impl>())
{
}

ShadPasskeyBackend::~ShadPasskeyBackend()
{
    cancel();
    if (_impl->thread != nullptr)
    {
        _impl->thread->wait();
        delete _impl->thread;
        _impl->thread = nullptr;
    }
}

bool ShadPasskeyBackend::isSupported()
{
    return webAuthnApi().isLoaded();
}

void ShadPasskeyBackend::registerCredential(const ShadPasskeyRegistrationOptions& options, QWidget* window, const RegistrationCallback& onFinished, const ErrorCallback& onError)
{
    _impl->prepare();
    const HWND hwnd = windowHandle(window);
    QObject* context = &_impl->context;
    GUID cancellationId = _impl->cancellationId;
    const bool hasCancellationId = _impl->hasCancellationId;
    _impl->thread = QThread::create([options, hwnd, context, cancellationId, hasCancellationId, onFinished, onError]() mutable
    {
        const WebAuthnApi& api = webAuthnApi();
        const std::wstring rpId = options.rpId.toStdWString();
        const std::wstring rpName = (options.rpName.isEmpty() ? options.rpId : options.rpName).toStdWString();
        const std::wstring userName = options.userName.toStdWString();
        const std::wstring displayName = (options.displayName.isEmpty() ? options.userName : options.displayName).toStdWString();
        const QByteArray clientData = clientDataJson("webauthn.create", options.challenge, options.rpId);

        WEBAUTHN_RP_ENTITY_INFORMATION rpInformation{};
        rpInformation.dwVersion = WEBAUTHN_RP_ENTITY_INFORMATION_CURRENT_VERSION;
        rpInformation.pwszId = rpId.c_str();
        rpInformation.pwszName = rpName.c_str();

        WEBAUTHN_USER_ENTITY_INFORMATION userInformation{};
        userInformation.dwVersion = WEBAUTHN_USER_ENTITY_INFORMATION_CURRENT_VERSION;
        userInformation.cbId = static_cast<DWORD>(options.userId.size());
        userInformation.pbId = toBytePointer(options.userId);
        userInformation.pwszName = userName.c_str();
        userInformation.pwszDisplayName = displayName.c_str();

        WEBAUTHN_COSE_CREDENTIAL_PARAMETER parameters[2]{};
        parameters[0].dwVersion = WEBAUTHN_COSE_CREDENTIAL_PARAMETER_CURRENT_VERSION;
        parameters[0].pwszCredentialType = WEBAUTHN_CREDENTIAL_TYPE_PUBLIC_KEY;
        parameters[0].lAlg = WEBAUTHN_COSE_ALGORITHM_ECDSA_P256_WITH_SHA256;
        parameters[1].dwVersion = WEBAUTHN_COSE_CREDENTIAL_PARAMETER_CURRENT_VERSION;
        parameters[1].pwszCredentialType = WEBAUTHN_CREDENTIAL_TYPE_PUBLIC_KEY;
        parameters[1].lAlg = WEBAUTHN_COSE_ALGORITHM_RSASSA_PKCS1_V1_5_WITH_SHA256;
        WEBAUTHN_COSE_CREDENTIAL_PARAMETERS parameterList{};
        parameterList.cCredentialParameters = 2;
        parameterList.pCredentialParameters = parameters;

        WEBAUTHN_CLIENT_DATA clientDataInformation{};
        clientDataInformation.dwVersion = WEBAUTHN_CLIENT_DATA_CURRENT_VERSION;
        clientDataInformation.cbClientDataJSON = static_cast<DWORD>(clientData.size());
        clientDataInformation.pbClientDataJSON = toBytePointer(clientData);
        clientDataInformation.pwszHashAlgId = WEBAUTHN_HASH_ALGORITHM_SHA_256;

        std::vector<WEBAUTHN_CREDENTIAL> excluded = credentialList(options.excludeCredentials);
        WEBAUTHN_AUTHENTICATOR_MAKE_CREDENTIAL_OPTIONS makeOptions{};
        makeOptions.dwVersion = WEBAUTHN_AUTHENTICATOR_MAKE_CREDENTIAL_OPTIONS_VERSION_2;
        makeOptions.dwTimeoutMilliseconds = timeoutMilliseconds(options.timeout);
        makeOptions.CredentialList.cCredentials = static_cast<DWORD>(excluded.size());
        makeOptions.CredentialList.pCredentials = excluded.empty() ? nullptr : excluded.data();
        makeOptions.dwAuthenticatorAttachment = WEBAUTHN_AUTHENTICATOR_ATTACHMENT_ANY;
        makeOptions.bRequireResidentKey = TRUE;
        makeOptions.dwUserVerificationRequirement = userVerificationRequirement(options.userVerification);
        makeOptions.dwAttestationConveyancePreference = WEBAUTHN_ATTESTATION_CONVEYANCE_PREFERENCE_NONE;
        makeOptions.pCancellationId = hasCancellationId ? &cancellationId : nullptr;

        PWEBAUTHN_CREDENTIAL_ATTESTATION attestation = nullptr;
        const HRESULT result = api.makeCredential(hwnd, &rpInformation, &userInformation, &parameterList, &clientDataInformation, &makeOptions, &attestation);
        if (FAILED(result) || attestation == nullptr)
        {
            if (attestation != nullptr)
            {
                api.freeCredentialAttestation(attestation);
            }
            postError(context, onError, FAILED(result) ? result : E_FAIL);
            return;
        }
        ShadPasskeyCredential credential;
        credential.credentialId = toByteArray(attestation->pbCredentialId, attestation->cbCredentialId);
        credential.clientDataJson = clientData;
        credential.attestationObject = toByteArray(attestation->pbAttestationObject, attestation->cbAttestationObject);
        api.freeCredentialAttestation(attestation);
        QMetaObject::invokeMethod(context, [onFinished, credential]() { onFinished(credential); }, Qt::QueuedConnection);
    });
    _impl->thread->start();
}

void ShadPasskeyBackend::authenticate(const ShadPasskeyAuthenticationOptions& options, QWidget* window, const AuthenticationCallback& onFinished, const ErrorCallback& onError)
{
    _impl->prepare();
    const HWND hwnd = windowHandle(window);
    QObject* context = &_impl->context;
    GUID cancellationId = _impl->cancellationId;
    const bool hasCancellationId = _impl->hasCancellationId;
    _impl->thread = QThread::create([options, hwnd, context, cancellationId, hasCancellationId, onFinished, onError]() mutable
    {
        const WebAuthnApi& api = webAuthnApi();
        const std::wstring rpId = options.rpId.toStdWString();
        const QByteArray clientData = clientDataJson("webauthn.get", options.challenge, options.rpId);

        WEBAUTHN_CLIENT_DATA clientDataInformation{};
        clientDataInformation.dwVersion = WEBAUTHN_CLIENT_DATA_CURRENT_VERSION;
        clientDataInformation.cbClientDataJSON = static_cast<DWORD>(clientData.size());
        clientDataInformation.pbClientDataJSON = toBytePointer(clientData);
        clientDataInformation.pwszHashAlgId = WEBAUTHN_HASH_ALGORITHM_SHA_256;

        std::vector<WEBAUTHN_CREDENTIAL> allowed = credentialList(options.allowCredentials);
        WEBAUTHN_AUTHENTICATOR_GET_ASSERTION_OPTIONS assertionOptions{};
        assertionOptions.dwVersion = WEBAUTHN_AUTHENTICATOR_GET_ASSERTION_OPTIONS_VERSION_3;
        assertionOptions.dwTimeoutMilliseconds = timeoutMilliseconds(options.timeout);
        assertionOptions.CredentialList.cCredentials = static_cast<DWORD>(allowed.size());
        assertionOptions.CredentialList.pCredentials = allowed.empty() ? nullptr : allowed.data();
        assertionOptions.dwAuthenticatorAttachment = WEBAUTHN_AUTHENTICATOR_ATTACHMENT_ANY;
        assertionOptions.dwUserVerificationRequirement = userVerificationRequirement(options.userVerification);
        assertionOptions.pCancellationId = hasCancellationId ? &cancellationId : nullptr;

        PWEBAUTHN_ASSERTION assertion = nullptr;
        const HRESULT result = api.getAssertion(hwnd, rpId.c_str(), &clientDataInformation, &assertionOptions, &assertion);
        if (FAILED(result) || assertion == nullptr)
        {
            if (assertion != nullptr)
            {
                api.freeAssertion(assertion);
            }
            postError(context, onError, FAILED(result) ? result : E_FAIL);
            return;
        }
        ShadPasskeyAssertion output;
        output.credentialId = toByteArray(assertion->Credential.pbId, assertion->Credential.cbId);
        output.clientDataJson = clientData;
        output.authenticatorData = toByteArray(assertion->pbAuthenticatorData, assertion->cbAuthenticatorData);
        output.signature = toByteArray(assertion->pbSignature, assertion->cbSignature);
        output.userHandle = toByteArray(assertion->pbUserId, assertion->cbUserId);
        api.freeAssertion(assertion);
        QMetaObject::invokeMethod(context, [onFinished, output]() { onFinished(output); }, Qt::QueuedConnection);
    });
    _impl->thread->start();
}

void ShadPasskeyBackend::cancel()
{
    if (_impl->thread == nullptr || !_impl->hasCancellationId || _impl->thread->isFinished())
    {
        return;
    }
    webAuthnApi().cancelCurrentOperation(&_impl->cancellationId);
}
