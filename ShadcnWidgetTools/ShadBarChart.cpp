#include "ShadBarChart.h"

#include <QtGlobal>

#include "ShadCategoryChartPrivate.h"
#include "ShadDef.h"

ShadBarChart::ShadBarChart(QWidget* parent)
    : ShadCategoryChart(*new ShadCategoryChartPrivate(ShadCategoryChartPrivate::Bar), parent)
{
}

ShadBarChart::~ShadBarChart()
{
}

void ShadBarChart::setStackMode(ShadChartType::StackMode stackMode)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    if (d->_stackMode == stackMode)
    {
        return;
    }
    d->_stackMode = stackMode;
    d->requestUpdate();
}

ShadChartType::StackMode ShadBarChart::getStackMode() const
{
    return categoryPrivate()->_stackMode;
}

void ShadBarChart::setBarRadius(qreal radius)
{
    ShadCategoryChartPrivate* d = categoryPrivate();
    const qreal boundedRadius = qMax(0.0, radius);
    if (qFuzzyCompare(d->_barRadius + 1.0, boundedRadius + 1.0))
    {
        return;
    }
    d->_barRadius = boundedRadius;
    d->requestUpdate();
}

qreal ShadBarChart::getBarRadius() const
{
    return categoryPrivate()->_barRadius;
}
