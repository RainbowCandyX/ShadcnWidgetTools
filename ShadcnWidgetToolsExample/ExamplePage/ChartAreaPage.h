#ifndef CHARTAREAPAGE_H
#define CHARTAREAPAGE_H

#include <QPointer>

#include "ExampleBasePage.h"
#include "ShadDef.h"
#include "ShadWidget.h"

// 面积图示例：默认、线性、阶梯、图例、堆叠、百分比堆叠、图标、渐变、坐标轴、交互、绘制 API 与压力测试
class ChartAreaPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ChartAreaPage(QWidget* parent = nullptr);
        ~ChartAreaPage() override;

    private:
        // 其他 API 的新窗口
        QPointer<ShadWidget> _renderApiWindow;
        ShadChartType::RenderApi _renderApiWindowApi{ShadChartType::Auto};
};

#endif // CHARTAREAPAGE_H
