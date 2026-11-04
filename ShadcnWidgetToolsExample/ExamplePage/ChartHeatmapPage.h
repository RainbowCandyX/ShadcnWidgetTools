#ifndef CHARTHEATMAPPAGE_H
#define CHARTHEATMAPPAGE_H

#include "ExampleBasePage.h"

// 热力图示例
class ChartHeatmapPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ChartHeatmapPage(QWidget* parent = nullptr);
        ~ChartHeatmapPage() override;
};

#endif // CHARTHEATMAPPAGE_H
