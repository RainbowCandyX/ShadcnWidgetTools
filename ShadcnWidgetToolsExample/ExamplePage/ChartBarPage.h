#ifndef CHARTBARPAGE_H
#define CHARTBARPAGE_H

#include "ExampleBasePage.h"

// 柱状图示例
class ChartBarPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ChartBarPage(QWidget* parent = nullptr);
        ~ChartBarPage() override;
};

#endif // CHARTBARPAGE_H
