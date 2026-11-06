#include "ShadScreenshotPrivate.h"

#include <QApplication>
#include <QClipboard>
#include <QCursor>
#include <QGuiApplication>
#include <QImage>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadScreenCapture.h"
#include "ShadScreenshot.h"
#include "ShadScreenshotFile.h"
#include "ShadScreenshotOverlay.h"
#include "ShadScreenshotPin.h"

ShadScreenshotPrivate::ShadScreenshotPrivate()
{
}

ShadScreenshotPrivate::~ShadScreenshotPrivate()
{
}

QWidget* ShadScreenshotPrivate::resolveWindow() const
{
    Q_Q(const ShadScreenshot);
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

bool ShadScreenshotPrivate::captureAndShowOverlays()
{
    Q_Q(ShadScreenshot);
    const QList<ShadScreenCapture::Screenshot> screenshots = ShadScreenCapture::grabScreens();
    if (screenshots.isEmpty())
    {
        _capture.restoreOwnWindows();
        return false;
    }
    const QList<QRect> windowRects = ShadScreenCapture::windowRects(_isOwnWindowsVisible);
    for (const ShadScreenCapture::Screenshot& screenshot : screenshots)
    {
        ShadScreenshotOverlay* overlay = new ShadScreenshotOverlay(screenshot.screen, screenshot.pixmap, _host.data());
        overlay->setWindowRects(windowRects);
        overlay->setCornerPinButtonVisible(_isCornerPinButtonVisible);
        QObject::connect(overlay, &ShadScreenshotOverlay::selectionChanged, q, [this, overlay](bool hasSelection) { lockOtherOverlays(overlay, hasSelection); });
        QObject::connect(overlay, &ShadScreenshotOverlay::doneRequested, q, [this](const QImage& image) { finish(image); });
        QObject::connect(overlay, &ShadScreenshotOverlay::copyRequested, q, [this](const QImage& image) { finish(image); });
        QObject::connect(overlay, &ShadScreenshotOverlay::saveRequested, q, [this](const QImage& image) { save(image); });
        QObject::connect(overlay, &ShadScreenshotOverlay::pinRequested, q, [this](const QImage& image, const QPoint& globalPosition) { pin(image, globalPosition); });
        QObject::connect(overlay, &ShadScreenshotOverlay::canceled, q, [q]() { q->cancel(); });
        _overlays.append(overlay);
        overlay->show();
    }
    _capture.restoreOwnWindows();
    const QPoint cursorPosition = QCursor::pos();
    for (const QPointer<ShadScreenshotOverlay>& overlay : _overlays)
    {
        overlay->syncWithCursor();
        if (overlay->geometry().contains(cursorPosition))
        {
            overlay->raise();
            overlay->activateWindow();
            overlay->setFocus(Qt::OtherFocusReason);
        }
    }
    _applicationStateConnection = QObject::connect(qGuiApp, &QGuiApplication::applicationStateChanged, q, [q](Qt::ApplicationState state)
    {
        if (state != Qt::ApplicationActive)
        {
            q->cancel();
        }
    });
    return true;
}

void ShadScreenshotPrivate::closeOverlays()
{
    QObject::disconnect(_applicationStateConnection);
    for (const QPointer<ShadScreenshotOverlay>& overlay : _overlays)
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
        _host->activateWindow();
    }
}

void ShadScreenshotPrivate::finish(const QImage& image)
{
    Q_Q(ShadScreenshot);
    QGuiApplication::clipboard()->setImage(image);
    closeOverlays();
    Q_EMIT q->captured(image);
}

void ShadScreenshotPrivate::save(const QImage& image)
{
    Q_Q(ShadScreenshot);
    closeOverlays();
    const QString filePath = ShadScreenshotFile::choosePath(_host.data());
    if (filePath.isEmpty())
    {
        Q_EMIT q->canceled();
        return;
    }
    const QPointer<ShadScreenshot> screenshot(q);
    ShadScreenshotFile::save(image, filePath, [screenshot, image, filePath](bool isSaved)
    {
        if (screenshot.isNull())
        {
            return;
        }
        if (!isSaved)
        {
            Q_EMIT screenshot->saveFailed(filePath);
            return;
        }
        Q_EMIT screenshot->captured(image);
        Q_EMIT screenshot->saved(filePath);
    });
}

void ShadScreenshotPrivate::pin(const QImage& image, const QPoint& globalPosition)
{
    Q_Q(ShadScreenshot);
    closeOverlays();
    ShadScreenshotPin* pin = new ShadScreenshotPin(image, globalPosition);
    QObject::connect(pin, &ShadScreenshotPin::saved, q, &ShadScreenshot::saved);
    QObject::connect(pin, &ShadScreenshotPin::saveFailed, q, &ShadScreenshot::saveFailed);
    _pins.removeAll(QPointer<ShadScreenshotPin>());
    _pins.append(pin);
    pin->show();
    pin->raise();
    pin->activateWindow();
    Q_EMIT q->pinned(image);
}

void ShadScreenshotPrivate::lockOtherOverlays(ShadScreenshotOverlay* owner, bool hasSelection)
{
    for (const QPointer<ShadScreenshotOverlay>& overlay : _overlays)
    {
        if (!overlay.isNull() && overlay.data() != owner)
        {
            overlay->setLocked(hasSelection);
        }
    }
}
