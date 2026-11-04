#include "ShadScatterChart.h"

#include <QColor>
#include <QList>
#include <QPointF>
#include <QString>
#include <QtGlobal>

#include "ShadScatterChartPrivate.h"

namespace
{
    constexpr qreal kMinPointRadius = 0.5;
} // namespace

ShadScatterChart::ShadScatterChart(QWidget* parent)
    : ShadChart(*new ShadScatterChartPrivate(), parent)
{
}

ShadScatterChart::~ShadScatterChart()
{
}

ShadScatterChartPrivate* ShadScatterChart::scatterPrivate() const
{
    return static_cast<ShadScatterChartPrivate*>(d_ptr.data());
}

int ShadScatterChart::addSeries(const QString& label, const QList<QPointF>& points, ShadThemeType::ThemeColor colorRole)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    ShadScatterChartPrivate::Series series;
    series.label = label;
    series.points = points;
    series.colorRole = colorRole;
    d->_series.append(series);
    d->requestUpdate();
    return d->_series.size() - 1;
}

int ShadScatterChart::addSeries(const QString& label, const QList<QPointF>& points, const QColor& color)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    ShadScatterChartPrivate::Series series;
    series.label = label;
    series.points = points;
    series.customColor = color;
    d->_series.append(series);
    d->requestUpdate();
    return d->_series.size() - 1;
}

void ShadScatterChart::setSeriesLabel(int index, const QString& label)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    if (index < 0 || index >= d->_series.size() || d->_series.at(index).label == label)
    {
        return;
    }
    // 名称只用于图例与提示卡片，数据点不需要重新上传
    d->_series[index].label = label;
    d->requestUpdate();
}

QString ShadScatterChart::getSeriesLabel(int index) const
{
    const ShadScatterChartPrivate* d = scatterPrivate();
    if (index < 0 || index >= d->_series.size())
    {
        return QString();
    }
    return d->_series.at(index).label;
}

void ShadScatterChart::setSeriesPoints(int index, const QList<QPointF>& points)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    if (index < 0 || index >= d->_series.size())
    {
        return;
    }
    ShadScatterChartPrivate::Series& series = d->_series[index];
    series.points = points;
    series.pendingAppend = 0;
    series.isFullDirty = true;
    d->requestUpdate();
}

QList<QPointF> ShadScatterChart::getSeriesPoints(int index) const
{
    const ShadScatterChartPrivate* d = scatterPrivate();
    if (index < 0 || index >= d->_series.size())
    {
        return QList<QPointF>();
    }
    return d->_series.at(index).points;
}

void ShadScatterChart::appendPoints(int index, const QList<QPointF>& points, int maxCount)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    if (index < 0 || index >= d->_series.size())
    {
        return;
    }
    ShadScatterChartPrivate::Series& series = d->_series[index];
    const int boundedMaxCount = qMax(0, maxCount);
    // maxCount 即 GPU 环形缓冲的容量，变化后整组重写
    if (series.ringMaxCount != boundedMaxCount)
    {
        series.ringMaxCount = boundedMaxCount;
        series.isFullDirty = true;
    }
    series.points.append(points);
    series.pendingAppend += static_cast<int>(points.size());
    if (boundedMaxCount > 0 && series.points.size() > boundedMaxCount)
    {
        series.points.remove(0, series.points.size() - boundedMaxCount);
    }
    d->requestUpdate();
}

void ShadScatterChart::clearSeries()
{
    ShadScatterChartPrivate* d = scatterPrivate();
    d->_series.clear();
    d->requestUpdate();
}

int ShadScatterChart::getSeriesCount() const
{
    return scatterPrivate()->_series.size();
}

void ShadScatterChart::setPointRadius(qreal radius)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    const qreal boundedRadius = qMax(kMinPointRadius, radius);
    if (qFuzzyCompare(d->_pointRadius, boundedRadius))
    {
        return;
    }
    d->_pointRadius = boundedRadius;
    d->requestUpdate();
}

qreal ShadScatterChart::getPointRadius() const
{
    return scatterPrivate()->_pointRadius;
}

void ShadScatterChart::setGridVisible(bool visible)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    if (d->_isGridVisible == visible)
    {
        return;
    }
    d->_isGridVisible = visible;
    d->requestUpdate();
}

bool ShadScatterChart::isGridVisible() const
{
    return scatterPrivate()->_isGridVisible;
}

void ShadScatterChart::setXAxisVisible(bool visible)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    if (d->_isXAxisVisible == visible)
    {
        return;
    }
    d->_isXAxisVisible = visible;
    d->requestUpdate();
}

bool ShadScatterChart::isXAxisVisible() const
{
    return scatterPrivate()->_isXAxisVisible;
}

void ShadScatterChart::setYAxisVisible(bool visible)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    if (d->_isYAxisVisible == visible)
    {
        return;
    }
    d->_isYAxisVisible = visible;
    d->requestUpdate();
}

bool ShadScatterChart::isYAxisVisible() const
{
    return scatterPrivate()->_isYAxisVisible;
}

void ShadScatterChart::setLegendVisible(bool visible)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    if (d->_isLegendVisible == visible)
    {
        return;
    }
    d->_isLegendVisible = visible;
    d->requestUpdate();
}

bool ShadScatterChart::isLegendVisible() const
{
    return scatterPrivate()->_isLegendVisible;
}

void ShadScatterChart::setTooltipVisible(bool visible)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    if (d->_isTooltipVisible == visible)
    {
        return;
    }
    d->_isTooltipVisible = visible;
    d->requestUpdate();
}

bool ShadScatterChart::isTooltipVisible() const
{
    return scatterPrivate()->_isTooltipVisible;
}

void ShadScatterChart::setXTickCount(int count)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    const int boundedCount = qMax(2, count);
    if (d->_xTickCount == boundedCount)
    {
        return;
    }
    d->_xTickCount = boundedCount;
    d->requestUpdate();
}

int ShadScatterChart::getXTickCount() const
{
    return scatterPrivate()->_xTickCount;
}

void ShadScatterChart::setYTickCount(int count)
{
    ShadScatterChartPrivate* d = scatterPrivate();
    const int boundedCount = qMax(2, count);
    if (d->_yTickCount == boundedCount)
    {
        return;
    }
    d->_yTickCount = boundedCount;
    d->requestUpdate();
}

int ShadScatterChart::getYTickCount() const
{
    return scatterPrivate()->_yTickCount;
}
