#ifndef BACKTOPPAGE_H
#define BACKTOPPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class BackTopPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit BackTopPage(QWidget* parent = nullptr);
        ~BackTopPage() override;
};

#endif // BACKTOPPAGE_H
