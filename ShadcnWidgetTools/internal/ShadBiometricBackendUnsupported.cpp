#include "ShadBiometricBackend.h"

#include <QString>
#include <QtGlobal>

#include "ShadDef.h"

struct ShadBiometricBackend::Impl
{
};

ShadBiometricBackend::ShadBiometricBackend()
{
}

ShadBiometricBackend::~ShadBiometricBackend()
{
}

bool ShadBiometricBackend::isSupported()
{
    return false;
}

ShadBiometricType::Method ShadBiometricBackend::availableMethod(ShadBiometricType::Policy policy)
{
    Q_UNUSED(policy);
    return ShadBiometricType::Unavailable;
}

void ShadBiometricBackend::authenticate(ShadBiometricType::Policy policy, const QString& reason, QWidget* window, const Callback& onFinished)
{
    Q_UNUSED(policy);
    Q_UNUSED(reason);
    Q_UNUSED(window);
    onFinished(false, ShadBiometricType::NotSupported, QString());
}

void ShadBiometricBackend::cancel()
{
}
