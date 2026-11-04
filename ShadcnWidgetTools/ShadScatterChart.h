#ifndef SHADSCATTERCHART_H
#define SHADSCATTERCHART_H

#include <QColor>
#include <QList>
#include <QPointF>
#include <QString>
#include <QtGlobal>

#include "ShadChart.h"
#include "ShadDef.h"
#include "ShadProperty.h"

class QWidget;

// 散点图：X、Y 为数值轴，范围按数据自动取整；GPU 绘制时追加的点只上传新增部分，CPU 绘制时点数多会变慢
class ShadScatterChartPrivate;

class SHADCN_EXPORT ShadScatterChart : public ShadChart
{
    Q_OBJECT
    Q_PROPERTY(qreal pointRadius READ getPointRadius WRITE setPointRadius)
    Q_PROPERTY(bool gridVisible READ isGridVisible WRITE setGridVisible)
    Q_PROPERTY(bool xAxisVisible READ isXAxisVisible WRITE setXAxisVisible)
    Q_PROPERTY(bool yAxisVisible READ isYAxisVisible WRITE setYAxisVisible)
    Q_PROPERTY(bool legendVisible READ isLegendVisible WRITE setLegendVisible)
    Q_PROPERTY(bool tooltipVisible READ isTooltipVisible WRITE setTooltipVisible)
    Q_PROPERTY(int xTickCount READ getXTickCount WRITE setXTickCount)
    Q_PROPERTY(int yTickCount READ getYTickCount WRITE setYTickCount)

    public:
        explicit ShadScatterChart(QWidget* parent = nullptr);
        ~ShadScatterChart() override;

        // 返回系列序号；颜色为主题色或固定颜色
        int addSeries(const QString& label, const QList<QPointF>& points, ShadThemeType::ThemeColor colorRole = ShadThemeType::Chart1);
        int addSeries(const QString& label, const QList<QPointF>& points, const QColor& color);
        void setSeriesPoints(int index, const QList<QPointF>& points);
        QList<QPointF> getSeriesPoints(int index) const;
        // 系列名称（图例与提示卡片中显示），例如切换语言时修改；不影响数据点与缩放
        void setSeriesLabel(int index, const QString& label);
        QString getSeriesLabel(int index) const;
        // maxCount 大于 0 时只保留最近 maxCount 个点
        void appendPoints(int index, const QList<QPointF>& points, int maxCount = 0);
        void clearSeries();
        int getSeriesCount() const;

        // 点半径，默认 3，最小 0.5
        void setPointRadius(qreal radius);
        qreal getPointRadius() const;

        // 水平网格线，默认显示
        void setGridVisible(bool visible);
        bool isGridVisible() const;
        // 刻度文字，默认显示
        void setXAxisVisible(bool visible);
        bool isXAxisVisible() const;
        void setYAxisVisible(bool visible);
        bool isYAxisVisible() const;
        // 图例，默认隐藏
        void setLegendVisible(bool visible);
        bool isLegendVisible() const;
        // 悬停提示，默认开启
        void setTooltipVisible(bool visible);
        bool isTooltipVisible() const;
        // 刻度数量，默认 5，最少 2
        void setXTickCount(int count);
        int getXTickCount() const;
        void setYTickCount(int count);
        int getYTickCount() const;

    private:
        Q_DISABLE_COPY(ShadScatterChart)
        ShadScatterChartPrivate* scatterPrivate() const;
};

#endif // SHADSCATTERCHART_H
