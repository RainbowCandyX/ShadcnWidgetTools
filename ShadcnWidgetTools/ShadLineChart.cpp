#include "ShadLineChart.h"

#include <QtGlobal>

#include "ShadCategoryChartPrivate.h"
#include "ShadDef.h"

namespace
{
    constexpr qreal kMinLineWidth = 0.5;
} // namespace

ShadLineChart::ShadLineChart(QWidget* parent)
    : ShadCategoryChart(*new ShadCategoryChartPrivate(ShadCategoryChartPrivate::Line), parent)
{
}

ShadLineChart::~ShadLineChart()
{
}

void ShadLineChart::setCurveType(ShadChartType::CurveType curveType)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_curveType == curveType)
    {
        return;
    }
    d->_curveType = curveType;
    d->requestUpdate();
}

ShadChartType::CurveType ShadLineChart::getCurveType() const
{
    return categoryPrivate()->_curveType;
}

void ShadLineChart::setLineWidth(qreal width)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    const qreal boundedWidth = qMax(kMinLineWidth, width);
    if (qFuzzyCompare(d->_lineWidth, boundedWidth))
    {
        return;
    }
    d->_lineWidth = boundedWidth;
    d->requestUpdate();
}

qreal ShadLineChart::getLineWidth() const
{
    return categoryPrivate()->_lineWidth;
}

void ShadLineChart::setDotsVisible(bool visible)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_isDotsVisible == visible)
    {
        return;
    }
    d->_isDotsVisible = visible;
    d->requestUpdate();
}

bool ShadLineChart::isDotsVisible() const
{
    return categoryPrivate()->_isDotsVisible;
}
