#ifndef SHADAREACHART_H
#define SHADAREACHART_H

#include "ShadCategoryChart.h"
#include "ShadDef.h"
#include "ShadProperty.h"

class QWidget;

// 面积图（shadcn Area Chart）：曲线默认 Natural，默认不堆叠，填充默认纯色 40%
class SHADCN_EXPORT ShadAreaChart : public ShadCategoryChart
{
    Q_OBJECT
    Q_PROPERTY(ShadChartType::CurveType curveType READ getCurveType WRITE setCurveType)
    Q_PROPERTY(ShadChartType::StackMode stackMode READ getStackMode WRITE setStackMode)
    Q_PROPERTY(ShadChartType::FillStyle fillStyle READ getFillStyle WRITE setFillStyle)

    public:
        explicit ShadAreaChart(QWidget* parent = nullptr);
        ~ShadAreaChart() override;

        void setCurveType(ShadChartType::CurveType curveType);
        ShadChartType::CurveType getCurveType() const;
        void setStackMode(ShadChartType::StackMode stackMode);
        ShadChartType::StackMode getStackMode() const;
        void setFillStyle(ShadChartType::FillStyle fillStyle);
        ShadChartType::FillStyle getFillStyle() const;

    private:
        Q_DISABLE_COPY(ShadAreaChart)
};

#endif // SHADAREACHART_H
