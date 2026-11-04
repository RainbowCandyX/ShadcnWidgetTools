#ifndef CHARTSCATTERPAGE_H
#define CHARTSCATTERPAGE_H

#include "ExampleBasePage.h"

// 散点图示例
class ChartScatterPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ChartScatterPage(QWidget* parent = nullptr);
        ~ChartScatterPage() override;
};

#endif // CHARTSCATTERPAGE_H
