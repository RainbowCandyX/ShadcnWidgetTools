#include "ShadScreenshot.h"

#include <QKeySequence>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QWidget>

#include "ShadGlobalShortcut.h"
#include "ShadScreenCapture.h"
#include "ShadScreenshotOverlay.h"
#include "ShadScreenshotPin.h"
#include "ShadScreenshotPrivate.h"

ShadScreenshot::ShadScreenshot(QObject* parent)
    : QObject(parent), d_ptr(new ShadScreenshotPrivate())
{
    Q_D(ShadScreenshot);
    d->q_ptr = this;
    d->_captureTimer = new QTimer(this);
    d->_captureTimer->setSingleShot(true);
    d->_captureTimer->setInterval(ShadScreenCapture::kHideWindowsDelayMs);
    connect(d->_captureTimer, &QTimer::timeout, this, [this, d]()
    {
        d->_isWaitingForCapture = false;
        if (!d->captureAndShowOverlays())
        {
            Q_EMIT canceled();
        }
    });
}

ShadScreenshot::~ShadScreenshot()
{
    Q_D(ShadScreenshot);
    d->_captureTimer->stop();
    d->_capture.restoreOwnWindows();
    QObject::disconnect(d->_applicationStateConnection);
    for (const QPointer<ShadScreenshotOverlay>& overlay : d->_overlays)
    {
        if (!overlay.isNull())
        {
            QObject::disconnect(overlay.data(), nullptr, this, nullptr);
            overlay->hide();
            overlay->deleteLater();
        }
    }
    closeAllPins();
}

bool ShadScreenshot::isSupported()
{
    return ShadScreenCapture::isSupported();
}

void ShadScreenshot::setOwnWindowsVisible(bool visible)
{
    Q_D(ShadScreenshot);
    d->_isOwnWindowsVisible = visible;
}

bool ShadScreenshot::isOwnWindowsVisible() const
{
    Q_D(const ShadScreenshot);
    return d->_isOwnWindowsVisible;
}

void ShadScreenshot::setCornerPinButtonVisible(bool visible)
{
    Q_D(ShadScreenshot);
    d->_isCornerPinButtonVisible = visible;
    for (const QPointer<ShadScreenshotOverlay>& overlay : d->_overlays)
    {
        if (!overlay.isNull())
        {
            overlay->setCornerPinButtonVisible(visible);
        }
    }
}

bool ShadScreenshot::isCornerPinButtonVisible() const
{
    Q_D(const ShadScreenshot);
    return d->_isCornerPinButtonVisible;
}

void ShadScreenshot::setWindow(QWidget* window)
{
    Q_D(ShadScreenshot);
    d->_window = window;
}

QWidget* ShadScreenshot::getWindow() const
{
    Q_D(const ShadScreenshot);
    return d->_window.data();
}

bool ShadScreenshot::start()
{
    Q_D(ShadScreenshot);
    if (isActive() || !isSupported() || !ShadScreenCapture::requestPermission())
    {
        return false;
    }
    d->_host = d->resolveWindow();
    if (d->_isOwnWindowsVisible)
    {
        return d->captureAndShowOverlays();
    }
    d->_capture.hideOwnWindows();
    d->_isWaitingForCapture = true;
    d->_captureTimer->start();
    return true;
}

void ShadScreenshot::cancel()
{
    Q_D(ShadScreenshot);
    if (d->_isWaitingForCapture)
    {
        d->_captureTimer->stop();
        d->_isWaitingForCapture = false;
        d->_capture.restoreOwnWindows();
        Q_EMIT canceled();
        return;
    }
    if (d->_overlays.isEmpty())
    {
        return;
    }
    d->closeOverlays();
    Q_EMIT canceled();
}

bool ShadScreenshot::isActive() const
{
    Q_D(const ShadScreenshot);
    return d->_isWaitingForCapture || !d->_overlays.isEmpty();
}

void ShadScreenshot::closeAllPins()
{
    Q_D(ShadScreenshot);
    const QList<QPointer<ShadScreenshotPin>> pins = d->_pins;
    d->_pins.clear();
    for (const QPointer<ShadScreenshotPin>& pin : pins)
    {
        if (!pin.isNull())
        {
            pin->close();
        }
    }
}

bool ShadScreenshot::isShortcutSupported()
{
    return ShadGlobalShortcut::isSupported();
}

bool ShadScreenshot::setShortcut(const QKeySequence& sequence)
{
    Q_D(ShadScreenshot);
    if (d->_shortcut == nullptr)
    {
        if (sequence.isEmpty())
        {
            return true;
        }
        d->_shortcut = new ShadGlobalShortcut(this);
        connect(d->_shortcut, &ShadGlobalShortcut::activated, this, [this]()
        {
            if (isActive())
            {
                return;
            }
            ShadGlobalShortcut::activateApplication();
            start();
        });
    }
    return d->_shortcut->setShortcut(sequence);
}

QKeySequence ShadScreenshot::getShortcut() const
{
    Q_D(const ShadScreenshot);
    return d->_shortcut != nullptr ? d->_shortcut->getShortcut() : QKeySequence();
}

int ShadScreenshot::pinCount() const
{
    Q_D(const ShadScreenshot);
    int count = 0;
    for (const QPointer<ShadScreenshotPin>& pin : d->_pins)
    {
        if (!pin.isNull() && pin->isVisible())
        {
            ++count;
        }
    }
    return count;
}
