#ifndef STATUSBARPAGE_H
#define STATUSBARPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class StatusBarPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit StatusBarPage(QWidget* parent = nullptr);
        ~StatusBarPage() override;
};

#endif // STATUSBARPAGE_H
