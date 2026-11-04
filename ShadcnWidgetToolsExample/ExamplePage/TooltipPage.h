#ifndef TOOLTIPPAGE_H
#define TOOLTIPPAGE_H

#include "ExampleBasePage.h"

class TooltipPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit TooltipPage(QWidget* parent = nullptr);
        ~TooltipPage() override;
};

#endif // TOOLTIPPAGE_H
