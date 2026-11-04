#include "ShadPasskeyBackend.h"

#include <QString>
#include <QtGlobal>

struct ShadPasskeyBackend::Impl
{
};

ShadPasskeyBackend::ShadPasskeyBackend()
{
}

ShadPasskeyBackend::~ShadPasskeyBackend()
{
}

bool ShadPasskeyBackend::isSupported()
{
    return false;
}

void ShadPasskeyBackend::registerCredential(const ShadPasskeyRegistrationOptions& options, QWidget* window, const RegistrationCallback& onFinished, const ErrorCallback& onError)
{
    Q_UNUSED(options);
    Q_UNUSED(window);
    Q_UNUSED(onFinished);
    onError(ShadPasskeyType::NotSupported, QString());
}

void ShadPasskeyBackend::authenticate(const ShadPasskeyAuthenticationOptions& options, QWidget* window, const AuthenticationCallback& onFinished, const ErrorCallback& onError)
{
    Q_UNUSED(options);
    Q_UNUSED(window);
    Q_UNUSED(onFinished);
    onError(ShadPasskeyType::NotSupported, QString());
}

void ShadPasskeyBackend::cancel()
{
}
