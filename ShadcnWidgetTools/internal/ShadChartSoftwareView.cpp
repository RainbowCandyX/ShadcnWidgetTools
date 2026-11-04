#include "ShadChartSoftwareView.h"

#include <QPaintEvent>
#include <QPainter>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadChartPrivate.h"

ShadChartSoftwareView::ShadChartSoftwareView(ShadChartPrivate* chartPrivate, QWidget* parent)
    : QWidget(parent), _chartPrivate(chartPrivate)
{
    // paintEvent 会铺满背景
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

ShadChartSoftwareView::~ShadChartSoftwareView()
{
}

void ShadChartSoftwareView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    {
        QPainter painter(this);
        // 与 GPU 层一样用表面颜色铺底
        painter.fillRect(rect(), _chartPrivate->surfaceColor());
        _chartPrivate->paintPlot(&painter);
    }
    Q_EMIT framePainted();
}
