#include "ShadBiometric.h"

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadBiometricBackend.h"
#include "ShadBiometricPrivate.h"
#include "ShadDef.h"

ShadBiometric::ShadBiometric(QObject* parent)
    : QObject(parent), d_ptr(new ShadBiometricPrivate())
{
    Q_D(ShadBiometric);
    d->q_ptr = this;
}

ShadBiometric::~ShadBiometric()
{
    Q_D(ShadBiometric);
    d->_backend.reset();
}

bool ShadBiometric::isSupported()
{
    return ShadBiometricBackend::isSupported();
}

void ShadBiometric::setPolicy(ShadBiometricType::Policy policy)
{
    Q_D(ShadBiometric);
    d->_policy = policy;
}

ShadBiometricType::Policy ShadBiometric::getPolicy() const
{
    Q_D(const ShadBiometric);
    return d->_policy;
}

ShadBiometricType::Method ShadBiometric::availableMethod() const
{
    Q_D(const ShadBiometric);
    return ShadBiometricBackend::availableMethod(d->_policy);
}

void ShadBiometric::setWindow(QWidget* window)
{
    Q_D(ShadBiometric);
    d->_window = window;
}

QWidget* ShadBiometric::getWindow() const
{
    Q_D(const ShadBiometric);
    return d->_window.data();
}

bool ShadBiometric::isBusy() const
{
    Q_D(const ShadBiometric);
    return d->_isBusy;
}

bool ShadBiometric::authenticate(const QString& reason)
{
    Q_D(ShadBiometric);
    if (d->_isBusy)
    {
        return false;
    }
    d->setBusy(true);
    if (!isSupported())
    {
        QMetaObject::invokeMethod(this, [d]() { d->finish(false, ShadBiometricType::NotSupported, QString()); }, Qt::QueuedConnection);
        return true;
    }
    const QString text = reason.isEmpty() ? tr("验证你的身份") : reason;
    d->_backend->authenticate(d->_policy, text, d->resolveWindow(), [d](bool isSuccess, ShadBiometricType::Error error, const QString& detail) { d->finish(isSuccess, error, detail); });
    return true;
}

void ShadBiometric::cancel()
{
    Q_D(ShadBiometric);
    if (d->_isBusy)
    {
        d->_backend->cancel();
    }
}
