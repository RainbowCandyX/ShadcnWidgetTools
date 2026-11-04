#include "ShadChartOverlay.h"

#include <QEvent>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadChartPrivate.h"
#if defined(SHADCN_CHART_GPU)
#include "ShadChartView.h"
#endif
#include "ShadTheme.h"

ShadChartOverlay::ShadChartOverlay(ShadChartPrivate* chartPrivate, QWidget* parent)
    : QWidget(parent), _chartPrivate(chartPrivate)
{
    setMouseTracking(true);
}

ShadChartOverlay::~ShadChartOverlay()
{
}

void ShadChartOverlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
#if defined(SHADCN_CHART_GPU)
    // 覆盖层没有设置不透明属性，GPU 层不会因被遮住而停止绘制
    const ShadChartView* view = _chartPrivate->_view;
    if (!_chartPrivate->_isPlotReady && view != nullptr && view->isVisible())
    {
        painter.fillRect(view->geometry(), _chartPrivate->surfaceColor());
    }
#endif
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    _chartPrivate->paintOverlay(&painter);
}

void ShadChartOverlay::mousePressEvent(QMouseEvent* event)
{
    // 按住左键拖动平移，拖动期间显示抓取光标
    if (event->button() == Qt::LeftButton && _chartPrivate->beginPan(event->position()))
    {
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadChartOverlay::mouseMoveEvent(QMouseEvent* event)
{
    _chartPrivate->panTo(event->position());
    _chartPrivate->updateHover(event->position());
    QWidget::mouseMoveEvent(event);
}

void ShadChartOverlay::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        _chartPrivate->endPan();
        unsetCursor();
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadChartOverlay::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && _chartPrivate->handleDoubleClick(event->position()))
    {
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool ShadChartOverlay::event(QEvent* event)
{
    if (event->type() == QEvent::NativeGesture)
    {
        const QNativeGestureEvent* gesture = static_cast<QNativeGestureEvent*>(event);
        if (gesture->gestureType() == Qt::ZoomNativeGesture && gesture->value() > -1.0 && _chartPrivate->zoomByFactor(gesture->position(), 1.0 + gesture->value()))
        {
            event->accept();
            return true;
        }
    }
    return QWidget::event(event);
}

void ShadChartOverlay::wheelEvent(QWheelEvent* event)
{
    // 未处理的滚轮事件交给父组件，例如滚动页面
    if (_chartPrivate->zoomAt(event->position(), event->angleDelta().y()))
    {
        event->accept();
        return;
    }
    event->ignore();
}

void ShadChartOverlay::leaveEvent(QEvent* event)
{
    _chartPrivate->clearHover();
    QWidget::leaveEvent(event);
}
