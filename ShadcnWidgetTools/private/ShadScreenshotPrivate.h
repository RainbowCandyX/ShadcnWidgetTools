#ifndef SHADSCREENSHOTPRIVATE_H
#define SHADSCREENSHOTPRIVATE_H

#include <QImage>
#include <QList>
#include <QMetaObject>
#include <QPoint>
#include <QPointer>
#include <QWidget>

#include "ShadProperty.h"
#include "ShadScreenCapture.h"

class QTimer;
class ShadGlobalShortcut;
class ShadScreenshot;
class ShadScreenshotOverlay;
class ShadScreenshotPin;

class ShadScreenshotPrivate
{
    SHAD_D_CREATE(ShadScreenshot)

    public:
        ShadScreenshotPrivate();
        ~ShadScreenshotPrivate();

        QWidget* resolveWindow() const;
        bool captureAndShowOverlays();
        void closeOverlays();
        void finish(const QImage& image);
        void save(const QImage& image);
        void pin(const QImage& image, const QPoint& globalPosition);
        void lockOtherOverlays(ShadScreenshotOverlay* owner, bool hasSelection);

        ShadScreenCapture _capture;
        QList<QPointer<ShadScreenshotOverlay>> _overlays;
        QList<QPointer<ShadScreenshotPin>> _pins;
        QPointer<QWidget> _window;
        QPointer<QWidget> _host;
        QMetaObject::Connection _applicationStateConnection;
        QTimer* _captureTimer{nullptr};
        ShadGlobalShortcut* _shortcut{nullptr};
        bool _isOwnWindowsVisible{false};
        bool _isCornerPinButtonVisible{false};
        bool _isWaitingForCapture{false};
};

#endif
