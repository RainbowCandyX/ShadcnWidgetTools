#include "ShadBiometricPrivate.h"

#include <QApplication>
#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QWidget>

#include "ShadBiometric.h"
#include "ShadBiometricBackend.h"

ShadBiometricPrivate::ShadBiometricPrivate()
    : _backend(std::make_unique<ShadBiometricBackend>())
{
}

ShadBiometricPrivate::~ShadBiometricPrivate()
{
}

QWidget* ShadBiometricPrivate::resolveWindow() const
{
    Q_Q(const ShadBiometric);
    if (!_window.isNull())
    {
        return _window->window();
    }
    QWidget* parentWidget = qobject_cast<QWidget*>(q->parent());
    if (parentWidget != nullptr)
    {
        return parentWidget->window();
    }
    return QApplication::activeWindow();
}

void ShadBiometricPrivate::setBusy(bool busy)
{
    Q_Q(ShadBiometric);
    if (_isBusy == busy)
    {
        return;
    }
    _isBusy = busy;
    Q_EMIT q->busyChanged(busy);
}

void ShadBiometricPrivate::finish(bool isSuccess, ShadBiometricType::Error error, const QString& detail)
{
    Q_Q(ShadBiometric);
    setBusy(false);
    if (isSuccess)
    {
        Q_EMIT q->succeeded();
        return;
    }
    Q_EMIT q->failed(error, errorMessage(error, detail));
}

QString ShadBiometricPrivate::errorMessage(ShadBiometricType::Error error, const QString& detail)
{
    switch (error)
    {
        case ShadBiometricType::Cancelled:
            return QCoreApplication::translate("ShadBiometric", "已取消");
        case ShadBiometricType::NotSupported:
            return QCoreApplication::translate("ShadBiometric", "当前设备不支持身份验证");
        case ShadBiometricType::NotEnrolled:
            return QCoreApplication::translate("ShadBiometric", "尚未设置指纹或 Windows Hello");
        case ShadBiometricType::Lockout:
            return QCoreApplication::translate("ShadBiometric", "尝试次数过多，已暂时锁定");
        case ShadBiometricType::Failed:
            return QCoreApplication::translate("ShadBiometric", "身份验证未通过");
        case ShadBiometricType::Other:
            break;
    }
    if (detail.isEmpty())
    {
        return QCoreApplication::translate("ShadBiometric", "身份验证失败");
    }
    return QCoreApplication::translate("ShadBiometric", "系统返回错误：%1").arg(detail);
}
