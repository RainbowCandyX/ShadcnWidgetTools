#include "ShadCategoryChart.h"

#include <QColor>
#include <QList>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "ShadCategoryChartPrivate.h"

ShadCategoryChart::ShadCategoryChart(ShadCategoryChartPrivate& dd, QWidget* parent)
    : ShadChart(dd, parent)
{
}

ShadCategoryChart::~ShadCategoryChart()
{
}

ShadCategoryChartPrivate* ShadCategoryChart::categoryPrivate() const
{
    return static_cast<ShadCategoryChartPrivate*>(d_ptr.data());
}

void ShadCategoryChart::setCategories(const QStringList& categories)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    d->_categories = categories;
    d->requestUpdate();
}

QStringList ShadCategoryChart::getCategories() const
{
    return categoryPrivate()->_categories;
}

void ShadCategoryChart::setTickLabels(const QStringList& labels)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    d->_tickLabels = labels;
    d->requestUpdate();
}

QStringList ShadCategoryChart::getTickLabels() const
{
    return categoryPrivate()->_tickLabels;
}

int ShadCategoryChart::addSeries(const QString& label, const QList<qreal>& values, ShadThemeType::ThemeColor colorRole)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    ShadCategoryChartPrivate::Series series;
    series.label = label;
    series.values = values;
    series.colorRole = colorRole;
    d->_series.append(series);
    d->requestUpdate();
    return d->_series.size() - 1;
}

int ShadCategoryChart::addSeries(const QString& label, const QList<qreal>& values, const QColor& color)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    ShadCategoryChartPrivate::Series series;
    series.label = label;
    series.values = values;
    series.customColor = color;
    d->_series.append(series);
    d->requestUpdate();
    return d->_series.size() - 1;
}

void ShadCategoryChart::setSeriesValues(int index, const QList<qreal>& values)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (index < 0 || index >= d->_series.size())
    {
        return;
    }
    d->_series[index].values = values;
    d->requestUpdate();
}

void ShadCategoryChart::setSeriesLabel(int index, const QString& label)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (index < 0 || index >= d->_series.size() || d->_series.at(index).label == label)
    {
        return;
    }
    d->_series[index].label = label;
    d->requestUpdate();
}

QString ShadCategoryChart::getSeriesLabel(int index) const
{
    const ShadCategoryChartPrivate* d = categoryPrivate();
    if (index < 0 || index >= d->_series.size())
    {
        return QString();
    }
    return d->_series.at(index).label;
}

QList<qreal> ShadCategoryChart::getSeriesValues(int index) const
{
    const ShadCategoryChartPrivate* d = categoryPrivate();
    if (index < 0 || index >= d->_series.size())
    {
        return QList<qreal>();
    }
    return d->_series.at(index).values;
}

void ShadCategoryChart::setSeriesIcon(int index, ShadIconType::IconName icon)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (index < 0 || index >= d->_series.size())
    {
        return;
    }
    d->_series[index].icon = icon;
    d->requestUpdate();
}

void ShadCategoryChart::clearSeries()
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    d->_series.clear();
    d->requestUpdate();
}

int ShadCategoryChart::getSeriesCount() const
{
    return categoryPrivate()->_series.size();
}

void ShadCategoryChart::appendData(const QStringList& categories, const QList<QList<qreal>>& seriesValues, int maxCount)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    const qsizetype appendCount = categories.size();
    d->_categories.append(categories);
    for (int series = 0; series < d->_series.size(); ++series)
    {
        QList<qreal>& values = d->_series[series].values;
        const QList<qreal> incoming = seriesValues.value(series);
        if (appendCount == 0)
        {
            // 未传入分类：按系列自身长度追加
            values.append(incoming);
            continue;
        }
        // 与分类对齐：先补齐或截断到追加前的分类数，再追加与新分类等量的数值
        values.resize(d->_categories.size() - appendCount);
        for (qsizetype i = 0; i < appendCount; ++i)
        {
            values.append(incoming.value(i, 0.0));
        }
    }
    if (maxCount > 0)
    {
        const qsizetype overflow = d->categoryCount() - maxCount;
        if (overflow > 0)
        {
            d->_categories.remove(0, qMin(overflow, d->_categories.size()));
            d->_tickLabels.remove(0, qMin(overflow, d->_tickLabels.size()));
            for (ShadCategoryChartPrivate::Series& series : d->_series)
            {
                series.values.remove(0, qMin(overflow, series.values.size()));
            }
            d->shiftView(static_cast<qreal>(overflow));
        }
    }
    d->requestUpdate();
}

void ShadCategoryChart::setTooltipIndicator(ShadChartType::TooltipIndicator indicator)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_tooltipIndicator == indicator)
    {
        return;
    }
    d->_tooltipIndicator = indicator;
    d->requestUpdate();
}

ShadChartType::TooltipIndicator ShadCategoryChart::getTooltipIndicator() const
{
    return categoryPrivate()->_tooltipIndicator;
}

void ShadCategoryChart::setGridVisible(bool visible)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_isGridVisible == visible)
    {
        return;
    }
    d->_isGridVisible = visible;
    d->requestUpdate();
}

bool ShadCategoryChart::isGridVisible() const
{
    return categoryPrivate()->_isGridVisible;
}

void ShadCategoryChart::setXAxisVisible(bool visible)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_isXAxisVisible == visible)
    {
        return;
    }
    d->_isXAxisVisible = visible;
    d->requestUpdate();
}

bool ShadCategoryChart::isXAxisVisible() const
{
    return categoryPrivate()->_isXAxisVisible;
}

void ShadCategoryChart::setYAxisVisible(bool visible)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_isYAxisVisible == visible)
    {
        return;
    }
    d->_isYAxisVisible = visible;
    d->requestUpdate();
}

bool ShadCategoryChart::isYAxisVisible() const
{
    return categoryPrivate()->_isYAxisVisible;
}

void ShadCategoryChart::setLegendVisible(bool visible)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_isLegendVisible == visible)
    {
        return;
    }
    d->_isLegendVisible = visible;
    d->requestUpdate();
}

bool ShadCategoryChart::isLegendVisible() const
{
    return categoryPrivate()->_isLegendVisible;
}

void ShadCategoryChart::setTooltipVisible(bool visible)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_isTooltipVisible == visible)
    {
        return;
    }
    d->_isTooltipVisible = visible;
    d->requestUpdate();
}

bool ShadCategoryChart::isTooltipVisible() const
{
    return categoryPrivate()->_isTooltipVisible;
}

void ShadCategoryChart::setYTickCount(int count)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    const int boundedCount = qMax(2, count);
    if (d->_yTickCount == boundedCount)
    {
        return;
    }
    d->_yTickCount = boundedCount;
    d->requestUpdate();
}

int ShadCategoryChart::getYTickCount() const
{
    return categoryPrivate()->_yTickCount;
}

void ShadCategoryChart::setZoomEnabled(bool enabled)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_isZoomEnabled == enabled)
    {
        return;
    }
    d->_isZoomEnabled = enabled;
    if (!enabled)
    {
        d->resetZoom();
    }
}

bool ShadCategoryChart::isZoomEnabled() const
{
    return categoryPrivate()->_isZoomEnabled;
}

bool ShadCategoryChart::isZoomed() const
{
    return categoryPrivate()->_isZoomed;
}

void ShadCategoryChart::resetZoom()
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (!d->_isZoomed)
    {
        return;
    }
    d->resetZoom();
}
