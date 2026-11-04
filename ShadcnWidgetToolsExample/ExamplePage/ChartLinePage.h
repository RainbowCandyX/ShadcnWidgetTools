#ifndef CHARTLINEPAGE_H
#define CHARTLINEPAGE_H

#include "ExampleBasePage.h"

// 折线图示例
class ChartLinePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ChartLinePage(QWidget* parent = nullptr);
        ~ChartLinePage() override;
};

#endif // CHARTLINEPAGE_H
