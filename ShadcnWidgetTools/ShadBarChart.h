#ifndef SHADBARCHART_H
#define SHADBARCHART_H

#include <QtGlobal>

#include "ShadCategoryChart.h"
#include "ShadDef.h"
#include "ShadProperty.h"

class QWidget;

// 柱状图（shadcn Bar Chart）：竖向柱子，默认不堆叠（各系列并排）；悬停只显示提示卡片
// 可见分类多于绘图区像素宽度时按像素列合并，每列每个系列保留最高的一根
class SHADCN_EXPORT ShadBarChart : public ShadCategoryChart
{
    Q_OBJECT
    Q_PROPERTY(ShadChartType::StackMode stackMode READ getStackMode WRITE setStackMode)
    Q_PROPERTY(qreal barRadius READ getBarRadius WRITE setBarRadius)

    public:
        explicit ShadBarChart(QWidget* parent = nullptr);
        ~ShadBarChart() override;

        void setStackMode(ShadChartType::StackMode stackMode);
        ShadChartType::StackMode getStackMode() const;
        // 圆角，默认 4，0 为直角；堆叠时只圆整根柱子的两端
        void setBarRadius(qreal radius);
        qreal getBarRadius() const;

    private:
        Q_DISABLE_COPY(ShadBarChart)
};

#endif // SHADBARCHART_H
