#include "ShadScreenCapture.h"

#include <QApplication>
#include <QGuiApplication>
#include <QLatin1String>
#include <QList>
#include <QPair>
#include <QPixmap>
#include <QPointer>
#include <QRect>
#include <QScreen>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#ifdef Q_OS_MACOS
#include <CoreGraphics/CoreGraphics.h>
#endif

#include "ShadScreenWindowList.h"

ShadScreenCapture::ShadScreenCapture()
{
}

ShadScreenCapture::~ShadScreenCapture()
{
    restoreOwnWindows();
}

bool ShadScreenCapture::isSupported()
{
#ifdef Q_OS_WASM
    return false;
#else
    return !QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive);
#endif
}

bool ShadScreenCapture::requestPermission()
{
#ifdef Q_OS_MACOS
    if (!CGPreflightScreenCaptureAccess())
    {
        CGRequestScreenCaptureAccess();
        return false;
    }
#endif
    return true;
}

QList<ShadScreenCapture::Screenshot> ShadScreenCapture::grabScreens()
{
    QList<Screenshot> screenshots;
    for (QScreen* screen : QGuiApplication::screens())
    {
        const QPixmap pixmap = screen->grabWindow(0);
        if (!pixmap.isNull())
        {
            screenshots.append(Screenshot{screen, pixmap});
        }
    }
    return screenshots;
}

QList<QRect> ShadScreenCapture::windowRects(bool includeOwnWindows)
{
    return ShadScreenWindowList::windowRects(includeOwnWindows);
}

void ShadScreenCapture::hideOwnWindows()
{
    restoreOwnWindows();
    const QList<QWidget*> windows = QApplication::topLevelWidgets();
    for (QWidget* window : windows)
    {
        if (!window->isVisible() || window->isMinimized())
        {
            continue;
        }
        _hiddenWindows.append(qMakePair(QPointer<QWidget>(window), window->windowOpacity()));
        window->setWindowOpacity(0.0);
    }
}

void ShadScreenCapture::restoreOwnWindows()
{
    for (const QPair<QPointer<QWidget>, qreal>& entry : std::as_const(_hiddenWindows))
    {
        if (!entry.first.isNull())
        {
            entry.first->setWindowOpacity(entry.second);
        }
    }
    _hiddenWindows.clear();
}
