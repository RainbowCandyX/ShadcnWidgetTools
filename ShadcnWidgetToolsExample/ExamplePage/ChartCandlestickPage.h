#ifndef CHARTCANDLESTICKPAGE_H
#define CHARTCANDLESTICKPAGE_H

#include "ExampleBasePage.h"

// K 线图示例
class ChartCandlestickPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ChartCandlestickPage(QWidget* parent = nullptr);
        ~ChartCandlestickPage() override;
};

#endif // CHARTCANDLESTICKPAGE_H
