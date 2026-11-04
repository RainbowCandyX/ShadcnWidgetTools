#ifndef ALERTPAGE_H
#define ALERTPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// Alert 示例
class AlertPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit AlertPage(QWidget* parent = nullptr);
        ~AlertPage() override;
};

#endif // ALERTPAGE_H
