#ifndef SHADSCREENCAPTURE_H
#define SHADSCREENCAPTURE_H

#include <QList>
#include <QPair>
#include <QPixmap>
#include <QPointer>
#include <QRect>
#include <QWidget>

class QScreen;

class ShadScreenCapture
{
    public:
        struct Screenshot
        {
            QScreen* screen{nullptr};
            QPixmap pixmap;
        };

        static constexpr int kHideWindowsDelayMs = 150;

        ShadScreenCapture();
        ~ShadScreenCapture();

        static bool isSupported();
        static bool requestPermission();
        static QList<Screenshot> grabScreens();
        static QList<QRect> windowRects(bool includeOwnWindows);

        void hideOwnWindows();
        void restoreOwnWindows();

    private:
        QList<QPair<QPointer<QWidget>, qreal>> _hiddenWindows;
};

#endif
