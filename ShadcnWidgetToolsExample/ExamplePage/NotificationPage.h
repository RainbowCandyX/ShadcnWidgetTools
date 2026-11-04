#ifndef NOTIFICATIONPAGE_H
#define NOTIFICATIONPAGE_H

#include <QPointer>
#include <QString>

#include "ExampleBasePage.h"
#include "ShadDef.h"
#include "ShadWidget.h"

class QWidget;

class NotificationPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit NotificationPage(QWidget* parent = nullptr);
        ~NotificationPage() override;

    private:
        void notify(ShadNotificationType::Type type, const QString& title, const QString& text, int duration, QWidget* host = nullptr);
        ShadWidget* createChildWindow();

        QPointer<ShadWidget> _childWindow;
        ShadNotificationType::Position _position{ShadNotificationType::TopRight};
        int _duration{3000};
};

#endif
