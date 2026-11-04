#include "ShadBiometricBackend.h"

#include <QChar>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <memory>
#include <mutex>

#include "ShadDef.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <unknwn.h>
#include <winbio.h>
#include <roapi.h>
#include <asyncinfo.h>
#include <windows.foundation.h>
#include <windows.security.credentials.ui.h>
#include <UserConsentVerifierInterop.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <wrl/ftm.h>
#include <wrl/implements.h>
#include <wrl/wrappers/corewrappers.h>

namespace
{
    namespace WinFoundation = ABI::Windows::Foundation;
    namespace CredentialsUI = ABI::Windows::Security::Credentials::UI;

    using VerificationOperation = WinFoundation::IAsyncOperation<CredentialsUI::UserConsentVerificationResult>;
    using VerificationHandler = WinFoundation::IAsyncOperationCompletedHandler<CredentialsUI::UserConsentVerificationResult>;

    struct Delivery
    {
        std::mutex mutex;
        QObject* receiver{nullptr};
        ShadBiometricBackend::Callback callback;
    };

    void post(const std::shared_ptr<Delivery>& delivery, bool isSuccess, ShadBiometricType::Error error, const QString& detail)
    {
        const std::lock_guard<std::mutex> lock(delivery->mutex);
        if (delivery->receiver == nullptr)
        {
            return;
        }
        const ShadBiometricBackend::Callback callback = delivery->callback;
        QMetaObject::invokeMethod(delivery->receiver, [callback, isSuccess, error, detail]() { callback(isSuccess, error, detail); }, Qt::QueuedConnection);
        delivery->receiver = nullptr;
    }

    bool hasBiometricUnit(WINBIO_BIOMETRIC_TYPE type)
    {
        WINBIO_UNIT_SCHEMA* units = nullptr;
        SIZE_T count = 0;
        const HRESULT result = WinBioEnumBiometricUnits(type, &units, &count);
        if (units != nullptr)
        {
            WinBioFree(units);
        }
        return SUCCEEDED(result) && count > 0;
    }

    QString hresultText(HRESULT code)
    {
        return QStringLiteral("0x%1").arg(static_cast<quint32>(code), 8, 16, QLatin1Char('0'));
    }

    void handleCompleted(const std::shared_ptr<Delivery>& delivery, VerificationOperation* operation, WinFoundation::AsyncStatus status)
    {
        if (status == WinFoundation::AsyncStatus::Canceled)
        {
            post(delivery, false, ShadBiometricType::Cancelled, QString());
            return;
        }
        if (status != WinFoundation::AsyncStatus::Completed)
        {
            HRESULT code = E_FAIL;
            Microsoft::WRL::ComPtr<WinFoundation::IAsyncInfo> info;
            if (SUCCEEDED(operation->QueryInterface(IID_PPV_ARGS(&info))))
            {
                info->get_ErrorCode(&code);
            }
            post(delivery, false, ShadBiometricType::Other, hresultText(code));
            return;
        }
        CredentialsUI::UserConsentVerificationResult result = CredentialsUI::UserConsentVerificationResult_Canceled;
        const HRESULT hr = operation->GetResults(&result);
        if (FAILED(hr))
        {
            post(delivery, false, ShadBiometricType::Other, hresultText(hr));
            return;
        }
        switch (result)
        {
            case CredentialsUI::UserConsentVerificationResult_Verified:
                post(delivery, true, ShadBiometricType::Other, QString());
                return;
            case CredentialsUI::UserConsentVerificationResult_DeviceNotPresent:
            case CredentialsUI::UserConsentVerificationResult_DisabledByPolicy:
                post(delivery, false, ShadBiometricType::NotSupported, QString());
                return;
            case CredentialsUI::UserConsentVerificationResult_NotConfiguredForUser:
                post(delivery, false, ShadBiometricType::NotEnrolled, QString());
                return;
            case CredentialsUI::UserConsentVerificationResult_RetriesExhausted:
                post(delivery, false, ShadBiometricType::Lockout, QString());
                return;
            case CredentialsUI::UserConsentVerificationResult_Canceled:
                post(delivery, false, ShadBiometricType::Cancelled, QString());
                return;
            case CredentialsUI::UserConsentVerificationResult_DeviceBusy:
                post(delivery, false, ShadBiometricType::Other, QStringLiteral("DeviceBusy"));
                return;
        }
        post(delivery, false, ShadBiometricType::Other, QString());
    }
}

struct ShadBiometricBackend::Impl
{
    QObject context;
    std::shared_ptr<Delivery> delivery;
    Microsoft::WRL::ComPtr<VerificationOperation> operation;

    void release()
    {
        if (delivery)
        {
            const std::lock_guard<std::mutex> lock(delivery->mutex);
            delivery->receiver = nullptr;
        }
        delivery.reset();
        operation.Reset();
    }
};

ShadBiometricBackend::ShadBiometricBackend()
    : _impl(std::make_unique<Impl>())
{
}

ShadBiometricBackend::~ShadBiometricBackend()
{
    cancel();
    _impl->release();
}

bool ShadBiometricBackend::isSupported()
{
    return true;
}

ShadBiometricType::Method ShadBiometricBackend::availableMethod(ShadBiometricType::Policy policy)
{
    Q_UNUSED(policy);
    if (hasBiometricUnit(WINBIO_TYPE_FINGERPRINT))
    {
        return ShadBiometricType::Fingerprint;
    }
    if (hasBiometricUnit(WINBIO_TYPE_FACIAL_FEATURES))
    {
        return ShadBiometricType::Face;
    }
    return ShadBiometricType::Password;
}

void ShadBiometricBackend::authenticate(ShadBiometricType::Policy policy, const QString& reason, QWidget* window, const Callback& onFinished)
{
    Q_UNUSED(policy);
    _impl->release();
    const std::shared_ptr<Delivery> delivery = std::make_shared<Delivery>();
    delivery->receiver = &_impl->context;
    delivery->callback = onFinished;
    _impl->delivery = delivery;
    const HWND hwnd = window != nullptr ? reinterpret_cast<HWND>(window->window()->winId()) : GetForegroundWindow();
    Microsoft::WRL::ComPtr<IUserConsentVerifierInterop> factory;
    HRESULT result = RoGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Windows_Security_Credentials_UI_UserConsentVerifier).Get(), IID_PPV_ARGS(&factory));
    if (FAILED(result))
    {
        post(delivery, false, ShadBiometricType::Other, hresultText(result));
        return;
    }
    Microsoft::WRL::Wrappers::HString message;
    result = message.Set(reinterpret_cast<const wchar_t*>(reason.utf16()), static_cast<unsigned int>(reason.size()));
    if (FAILED(result))
    {
        post(delivery, false, ShadBiometricType::Other, hresultText(result));
        return;
    }
    Microsoft::WRL::ComPtr<VerificationOperation> operation;
    result = factory->RequestVerificationForWindowAsync(hwnd, message.Get(), IID_PPV_ARGS(&operation));
    if (FAILED(result))
    {
        post(delivery, false, ShadBiometricType::Other, hresultText(result));
        return;
    }
    _impl->operation = operation;
    const auto handler = Microsoft::WRL::Callback<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, VerificationHandler, Microsoft::WRL::FtmBase>>([delivery](VerificationOperation* completed, WinFoundation::AsyncStatus status) -> HRESULT { handleCompleted(delivery, completed, status); return S_OK; });
    result = handler ? operation->put_Completed(handler.Get()) : E_OUTOFMEMORY;
    if (FAILED(result))
    {
        post(delivery, false, ShadBiometricType::Other, hresultText(result));
    }
}

void ShadBiometricBackend::cancel()
{
    if (!_impl->operation)
    {
        return;
    }
    Microsoft::WRL::ComPtr<WinFoundation::IAsyncInfo> info;
    HRESULT result = _impl->operation.As(&info);
    if (SUCCEEDED(result))
    {
        result = info->Cancel();
    }
    if (FAILED(result))
    {
        post(_impl->delivery, false, ShadBiometricType::Other, hresultText(result));
    }
}
