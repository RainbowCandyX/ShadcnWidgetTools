#include "ShadPasskeyPrivate.h"

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QMetaObject>
#include <QObject>
#include <QRandomGenerator>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtEndian>
#include <QtGlobal>

#include <functional>

#include "ShadPasskey.h"
#include "ShadPasskeyBackend.h"

ShadPasskeyPrivate::ShadPasskeyPrivate()
    : _backend(std::make_unique<ShadPasskeyBackend>())
{
}

ShadPasskeyPrivate::~ShadPasskeyPrivate()
{
}

QWidget* ShadPasskeyPrivate::resolveWindow() const
{
    Q_Q(const ShadPasskey);
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

void ShadPasskeyPrivate::setBusy(bool busy)
{
    Q_Q(ShadPasskey);
    if (_isBusy == busy)
    {
        return;
    }
    _isBusy = busy;
    Q_EMIT q->busyChanged(busy);
}

void ShadPasskeyPrivate::start(int timeout)
{
    _isTimedOut = false;
    if (timeout > 0)
    {
        _timeoutTimer->start(timeout);
    }
}

void ShadPasskeyPrivate::finish()
{
    _timeoutTimer->stop();
    _simulationTimer->stop();
    _simulationResult = nullptr;
    _isTimedOut = false;
    setBusy(false);
}

void ShadPasskeyPrivate::fail(ShadPasskeyType::Error error, const QString& detail)
{
    Q_Q(ShadPasskey);
    const ShadPasskeyType::Error result = _isTimedOut ? ShadPasskeyType::Timeout : error;
    finish();
    Q_EMIT q->failed(result, errorMessage(result, detail));
}

void ShadPasskeyPrivate::failLater(ShadPasskeyType::Error error)
{
    Q_Q(ShadPasskey);
    QMetaObject::invokeMethod(q, [this, error]() { fail(error, QString()); }, Qt::QueuedConnection);
}

void ShadPasskeyPrivate::simulate(const std::function<void()>& deliver)
{
    _simulationResult = deliver;
    _simulationTimer->start(kSimulationDelay);
}

QByteArray ShadPasskeyPrivate::randomBytes(int size)
{
    QByteArray data(size, Qt::Uninitialized);
    for (char& byte : data)
    {
        byte = static_cast<char>(QRandomGenerator::system()->bounded(256));
    }
    return data;
}

QByteArray ShadPasskeyPrivate::clientDataJson(const char* type, const QByteArray& challenge, const QString& rpId)
{
    const QByteArray encodedChallenge = challenge.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return QByteArrayLiteral("{\"type\":\"") + type + QByteArrayLiteral("\",\"challenge\":\"") + encodedChallenge + QByteArrayLiteral("\",\"origin\":\"https://") + rpId.toUtf8() + QByteArrayLiteral("\",\"crossOrigin\":false}");
}

QByteArray ShadPasskeyPrivate::authenticatorData(const QString& rpId, quint32 signCount)
{
    QByteArray counter(sizeof(quint32), Qt::Uninitialized);
    qToBigEndian<quint32>(signCount, counter.data());
    return QCryptographicHash::hash(rpId.toUtf8(), QCryptographicHash::Sha256) + QByteArray(1, '\x05') + counter;
}

QString ShadPasskeyPrivate::errorMessage(ShadPasskeyType::Error error, const QString& detail)
{
    switch (error)
    {
        case ShadPasskeyType::Cancelled:
            return QCoreApplication::translate("ShadPasskey", "已取消");
        case ShadPasskeyType::NotSupported:
            return QCoreApplication::translate("ShadPasskey", "当前平台不支持通行密钥");
        case ShadPasskeyType::Timeout:
            return QCoreApplication::translate("ShadPasskey", "请求超时");
        case ShadPasskeyType::DomainNotAllowed:
            return QCoreApplication::translate("ShadPasskey", "应用未与该域名关联");
        case ShadPasskeyType::InvalidOptions:
            return QCoreApplication::translate("ShadPasskey", "参数不完整");
        case ShadPasskeyType::Other:
            break;
    }
    if (detail.isEmpty())
    {
        return QCoreApplication::translate("ShadPasskey", "认证失败");
    }
    return QCoreApplication::translate("ShadPasskey", "系统返回错误：%1").arg(detail);
}
