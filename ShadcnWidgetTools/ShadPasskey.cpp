#include "ShadPasskey.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <functional>
#include <utility>

#include "ShadDef.h"
#include "ShadPasskeyBackend.h"
#include "ShadPasskeyPrivate.h"

ShadPasskey::ShadPasskey(QObject* parent)
    : QObject(parent), d_ptr(new ShadPasskeyPrivate())
{
    Q_D(ShadPasskey);
    d->q_ptr = this;
    d->_timeoutTimer = new QTimer(this);
    d->_timeoutTimer->setSingleShot(true);
    connect(d->_timeoutTimer, &QTimer::timeout, this, [d]()
    {
        d->_isTimedOut = true;
        d->_backend->cancel();
    });
    d->_simulationTimer = new QTimer(this);
    d->_simulationTimer->setSingleShot(true);
    connect(d->_simulationTimer, &QTimer::timeout, this, [d]()
    {
        const std::function<void()> deliver = std::move(d->_simulationResult);
        d->finish();
        if (deliver)
        {
            deliver();
        }
    });
}

ShadPasskey::~ShadPasskey()
{
    Q_D(ShadPasskey);
    d->_backend.reset();
}

bool ShadPasskey::isSupported()
{
    return ShadPasskeyBackend::isSupported();
}

QString ShadPasskey::toBase64Url(const QByteArray& data)
{
    return QString::fromLatin1(data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QByteArray ShadPasskey::fromBase64Url(const QString& text)
{
    return QByteArray::fromBase64(text.toLatin1(), QByteArray::Base64UrlEncoding);
}

void ShadPasskey::setWindow(QWidget* window)
{
    Q_D(ShadPasskey);
    d->_window = window;
}

QWidget* ShadPasskey::getWindow() const
{
    Q_D(const ShadPasskey);
    return d->_window.data();
}

bool ShadPasskey::isBusy() const
{
    Q_D(const ShadPasskey);
    return d->_isBusy;
}

void ShadPasskey::setSimulated(bool simulated)
{
    Q_D(ShadPasskey);
    if (d->_isSimulated == simulated)
    {
        return;
    }
    d->_isSimulated = simulated;
    Q_EMIT simulatedChanged(simulated);
}

bool ShadPasskey::isSimulated() const
{
    Q_D(const ShadPasskey);
    return d->_isSimulated;
}

bool ShadPasskey::registerCredential(const ShadPasskeyRegistrationOptions& options)
{
    Q_D(ShadPasskey);
    if (d->_isBusy)
    {
        return false;
    }
    d->setBusy(true);
    if (!d->_isSimulated && !isSupported())
    {
        d->failLater(ShadPasskeyType::NotSupported);
        return true;
    }
    if (options.rpId.isEmpty() || options.challenge.isEmpty() || options.userId.isEmpty() || options.userName.isEmpty())
    {
        d->failLater(ShadPasskeyType::InvalidOptions);
        return true;
    }
    if (d->_isSimulated)
    {
        d->_simulatedCredentialId = ShadPasskeyPrivate::randomBytes(ShadPasskeyPrivate::kSimulatedIdSize);
        d->_simulatedUserId = options.userId;
        d->_simulatedSignCount = 0;
        ShadPasskeyCredential credential;
        credential.credentialId = d->_simulatedCredentialId;
        credential.clientDataJson = ShadPasskeyPrivate::clientDataJson("webauthn.create", options.challenge, options.rpId);
        credential.attestationObject = ShadPasskeyPrivate::randomBytes(ShadPasskeyPrivate::kSimulatedSignatureSize);
        d->simulate([this, credential]() { Q_EMIT registrationFinished(credential); });
        return true;
    }
    d->start(options.timeout);
    d->_backend->registerCredential(options, d->resolveWindow(), [this, d](const ShadPasskeyCredential& credential)
    {
        d->finish();
        Q_EMIT registrationFinished(credential);
    }, [d](ShadPasskeyType::Error error, const QString& detail) { d->fail(error, detail); });
    return true;
}

bool ShadPasskey::authenticate(const ShadPasskeyAuthenticationOptions& options)
{
    Q_D(ShadPasskey);
    if (d->_isBusy)
    {
        return false;
    }
    d->setBusy(true);
    if (!d->_isSimulated && !isSupported())
    {
        d->failLater(ShadPasskeyType::NotSupported);
        return true;
    }
    if (options.rpId.isEmpty() || options.challenge.isEmpty())
    {
        d->failLater(ShadPasskeyType::InvalidOptions);
        return true;
    }
    if (d->_isSimulated)
    {
        if (d->_simulatedCredentialId.isEmpty())
        {
            d->_simulatedCredentialId = ShadPasskeyPrivate::randomBytes(ShadPasskeyPrivate::kSimulatedIdSize);
            d->_simulatedUserId = ShadPasskeyPrivate::randomBytes(ShadPasskeyPrivate::kSimulatedIdSize);
        }
        ++d->_simulatedSignCount;
        ShadPasskeyAssertion assertion;
        assertion.credentialId = d->_simulatedCredentialId;
        assertion.clientDataJson = ShadPasskeyPrivate::clientDataJson("webauthn.get", options.challenge, options.rpId);
        assertion.authenticatorData = ShadPasskeyPrivate::authenticatorData(options.rpId, d->_simulatedSignCount);
        assertion.signature = ShadPasskeyPrivate::randomBytes(ShadPasskeyPrivate::kSimulatedSignatureSize);
        assertion.userHandle = d->_simulatedUserId;
        d->simulate([this, assertion]() { Q_EMIT authenticationFinished(assertion); });
        return true;
    }
    d->start(options.timeout);
    d->_backend->authenticate(options, d->resolveWindow(), [this, d](const ShadPasskeyAssertion& assertion)
    {
        d->finish();
        Q_EMIT authenticationFinished(assertion);
    }, [d](ShadPasskeyType::Error error, const QString& detail) { d->fail(error, detail); });
    return true;
}

void ShadPasskey::cancel()
{
    Q_D(ShadPasskey);
    if (d->_simulationTimer->isActive())
    {
        d->fail(ShadPasskeyType::Cancelled, QString());
        return;
    }
    if (d->_isBusy)
    {
        d->_backend->cancel();
    }
}
