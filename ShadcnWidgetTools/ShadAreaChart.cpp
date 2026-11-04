#include "ShadAreaChart.h"

#include "ShadCategoryChartPrivate.h"
#include "ShadDef.h"

ShadAreaChart::ShadAreaChart(QWidget* parent)
    : ShadCategoryChart(*new ShadCategoryChartPrivate(ShadCategoryChartPrivate::Area), parent)
{
}

ShadAreaChart::~ShadAreaChart()
{
}

void ShadAreaChart::setCurveType(ShadChartType::CurveType curveType)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_curveType == curveType)
    {
        return;
    }
    d->_curveType = curveType;
    d->requestUpdate();
}

ShadChartType::CurveType ShadAreaChart::getCurveType() const
{
    return categoryPrivate()->_curveType;
}

void ShadAreaChart::setStackMode(ShadChartType::StackMode stackMode)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_stackMode == stackMode)
    {
        return;
    }
    d->_stackMode = stackMode;
    d->requestUpdate();
}

ShadChartType::StackMode ShadAreaChart::getStackMode() const
{
    return categoryPrivate()->_stackMode;
}

void ShadAreaChart::setFillStyle(ShadChartType::FillStyle fillStyle)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_fillStyle == fillStyle)
    {
        return;
    }
    d->_fillStyle = fillStyle;
    d->requestUpdate();
}

ShadChartType::FillStyle ShadAreaChart::getFillStyle() const
{
    return categoryPrivate()->_fillStyle;
}
