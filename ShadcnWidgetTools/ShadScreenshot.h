#ifndef SHADSCREENSHOT_H
#define SHADSCREENSHOT_H

#include <QImage>
#include <QKeySequence>
#include <QObject>
#include <QString>

#include "ShadProperty.h"

class QWidget;
class ShadScreenshotPrivate;

class SHADCN_EXPORT ShadScreenshot : public QObject
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadScreenshot)
    Q_PROPERTY(bool ownWindowsVisible READ isOwnWindowsVisible WRITE setOwnWindowsVisible)
    Q_PROPERTY(bool cornerPinButtonVisible READ isCornerPinButtonVisible WRITE setCornerPinButtonVisible)

    public:
        explicit ShadScreenshot(QObject* parent = nullptr);
        ~ShadScreenshot() override;

        static bool isSupported();

        void setOwnWindowsVisible(bool visible);
        bool isOwnWindowsVisible() const;

        void setCornerPinButtonVisible(bool visible);
        bool isCornerPinButtonVisible() const;

        void setWindow(QWidget* window);
        QWidget* getWindow() const;

        bool start();
        void cancel();
        bool isActive() const;

        void closeAllPins();
        int pinCount() const;

        static bool isShortcutSupported();
        bool setShortcut(const QKeySequence& sequence);
        QKeySequence getShortcut() const;

    Q_SIGNALS:
        void captured(const QImage& image);
        void pinned(const QImage& image);
        void saved(const QString& filePath);
        void saveFailed(const QString& filePath);
        void canceled();
};

#endif
