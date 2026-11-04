#ifndef SHADNOTIFICATION_H
#define SHADNOTIFICATION_H

#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QObject;
class QPaintEvent;
class QResizeEvent;

class ShadNotificationPrivate;

class SHADCN_EXPORT ShadNotification : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadNotification)

    public:
        static void info(const QString& title, const QString& text, int duration = 3000, ShadNotificationType::Position position = ShadNotificationType::TopRight, QWidget* parent = nullptr);
        static void success(const QString& title, const QString& text, int duration = 3000, ShadNotificationType::Position position = ShadNotificationType::TopRight, QWidget* parent = nullptr);
        static void warning(const QString& title, const QString& text, int duration = 3000, ShadNotificationType::Position position = ShadNotificationType::TopRight, QWidget* parent = nullptr);
        static void error(const QString& title, const QString& text, int duration = 3000, ShadNotificationType::Position position = ShadNotificationType::TopRight, QWidget* parent = nullptr);
        static void closeAll(QWidget* parent = nullptr);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        ShadNotification(ShadNotificationType::Type type, ShadNotificationType::Position position, const QString& title, const QString& text, int duration, QWidget* parent);
        ~ShadNotification() override;
};

#endif
