#include "ShadScreenColorPicker.h"

#include <QColor>
#include <QCursor>
#include <QGuiApplication>
#include <QList>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QTimer>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadScreenCapture.h"
#include "ShadScreenPickerOverlay.h"

ShadScreenColorPicker::ShadScreenColorPicker(QWidget* host)
    : QObject(host), _host(host)
{
    _captureTimer = new QTimer(this);
    _captureTimer->setSingleShot(true);
    // 等系统刷新画面后再截屏
    _captureTimer->setInterval(ShadScreenCapture::kHideWindowsDelayMs);
    connect(_captureTimer, &QTimer::timeout, this, [this]()
    {
        _isWaitingForCapture = false;
        if (!captureAndShowOverlays())
        {
            Q_EMIT canceled();
        }
    });
}

ShadScreenColorPicker::~ShadScreenColorPicker()
{
    // 恢复窗口透明度，否则窗口会一直看不见
    _captureTimer->stop();
    _capture.restoreOwnWindows();
    QObject::disconnect(_applicationStateConnection);
    // 覆盖窗口是发起取色窗口的子窗口，此时该窗口可能正在删除子对象，这里只隐藏并延迟删除
    for (const QPointer<ShadScreenPickerOverlay>& overlay : _overlays)
    {
        if (!overlay.isNull())
        {
            QObject::disconnect(overlay.data(), nullptr, this, nullptr);
            overlay->hide();
            overlay->deleteLater();
        }
    }
}

bool ShadScreenColorPicker::isSupported()
{
    return ShadScreenCapture::isSupported();
}

void ShadScreenColorPicker::setOwnWindowsVisible(bool visible)
{
    _isOwnWindowsVisible = visible;
}

bool ShadScreenColorPicker::isOwnWindowsVisible() const
{
    return _isOwnWindowsVisible;
}

bool ShadScreenColorPicker::start()
{
    if (isActive() || _host.isNull() || !isSupported())
    {
        return false;
    }
    // 没有屏幕录制权限时只请求授权，本次不开始取色
    if (!ShadScreenCapture::requestPermission())
    {
        return false;
    }
    if (_isOwnWindowsVisible)
    {
        return captureAndShowOverlays();
    }
    _capture.hideOwnWindows();
    _isWaitingForCapture = true;
    _captureTimer->start();
    return true;
}

bool ShadScreenColorPicker::captureAndShowOverlays()
{
    if (_host.isNull())
    {
        _capture.restoreOwnWindows();
        return false;
    }
    // 先截取全部屏幕再显示覆盖窗口，截图中不含覆盖窗口
    const QList<ShadScreenCapture::Screenshot> screenshots = ShadScreenCapture::grabScreens();
    if (screenshots.isEmpty())
    {
        _capture.restoreOwnWindows();
        return false;
    }
    // 截屏后立即记录窗口位置，此时覆盖窗口尚未显示，不在列表中
    const QList<QRect> windowRects = ShadScreenCapture::windowRects(_isOwnWindowsVisible);
    QWidget* window = _host->window();
    for (const ShadScreenCapture::Screenshot& screenshot : screenshots)
    {
        ShadScreenPickerOverlay* overlay = new ShadScreenPickerOverlay(screenshot.screen, screenshot.pixmap, window);
        overlay->setWindowRects(windowRects);
        connect(overlay, &ShadScreenPickerOverlay::colorPicked, this, [this](const QColor& color)
        {
            closeOverlays();
            Q_EMIT colorPicked(color);
        });
        connect(overlay, &ShadScreenPickerOverlay::canceled, this, [this]() { cancel(); });
        _overlays.append(overlay);
        overlay->show();
    }
    // 覆盖窗口已盖住屏幕，恢复透明度不会被看到
    _capture.restoreOwnWindows();
    // 光标所在屏幕的覆盖窗口接收键盘输入
    const QPoint cursorPosition = QCursor::pos();
    for (const QPointer<ShadScreenPickerOverlay>& overlay : _overlays)
    {
        overlay->syncWithCursor();
        if (overlay->geometry().contains(cursorPosition))
        {
            overlay->raise();
            overlay->activateWindow();
            overlay->setFocus(Qt::OtherFocusReason);
        }
    }
    // 切换到其他程序时取消
    _applicationStateConnection = connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state)
    {
        if (state != Qt::ApplicationActive)
        {
            cancel();
        }
    });
    return true;
}

void ShadScreenColorPicker::cancel()
{
    if (!isActive())
    {
        return;
    }
    if (_isWaitingForCapture)
    {
        _captureTimer->stop();
        _isWaitingForCapture = false;
        _capture.restoreOwnWindows();
        Q_EMIT canceled();
        return;
    }
    closeOverlays();
    Q_EMIT canceled();
}

bool ShadScreenColorPicker::isActive() const
{
    return _isWaitingForCapture || !_overlays.isEmpty();
}

void ShadScreenColorPicker::closeOverlays()
{
    QObject::disconnect(_applicationStateConnection);
    // 信号可能来自覆盖窗口自身，延迟删除
    for (const QPointer<ShadScreenPickerOverlay>& overlay : _overlays)
    {
        if (!overlay.isNull())
        {
            overlay->hide();
            overlay->deleteLater();
        }
    }
    _overlays.clear();
    if (!_host.isNull())
    {
        _host->window()->activateWindow();
    }
}
