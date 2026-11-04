#ifndef SHADLINECHART_H
#define SHADLINECHART_H

#include <QtGlobal>

#include "ShadCategoryChart.h"
#include "ShadDef.h"
#include "ShadProperty.h"

class QWidget;

// 折线图（shadcn Line Chart）：曲线默认 Natural，数据点默认隐藏
class SHADCN_EXPORT ShadLineChart : public ShadCategoryChart
{
    Q_OBJECT
    Q_PROPERTY(ShadChartType::CurveType curveType READ getCurveType WRITE setCurveType)
    Q_PROPERTY(qreal lineWidth READ getLineWidth WRITE setLineWidth)
    Q_PROPERTY(bool dotsVisible READ isDotsVisible WRITE setDotsVisible)

    public:
        explicit ShadLineChart(QWidget* parent = nullptr);
        ~ShadLineChart() override;

        void setCurveType(ShadChartType::CurveType curveType);
        ShadChartType::CurveType getCurveType() const;
        // 线宽，默认 2，最小 0.5
        void setLineWidth(qreal width);
        qreal getLineWidth() const;
        // 数据点半径为 3 加线宽的一半；数据过密抽稀后不显示
        void setDotsVisible(bool visible);
        bool isDotsVisible() const;

    private:
        Q_DISABLE_COPY(ShadLineChart)
};

#endif // SHADLINECHART_H
