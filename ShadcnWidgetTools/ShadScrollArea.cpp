#include "ShadScrollArea.h"

#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFrame>
#include <QObject>
#include <QPalette>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTimer>
#include <QWidget>

#include "ShadScrollAreaPrivate.h"
#include "ShadScrollBar.h"

ShadScrollArea::ShadScrollArea(QWidget* parent)
    : QScrollArea(parent), d_ptr(new ShadScrollAreaPrivate())
{
    Q_D(ShadScrollArea);
    d->q_ptr = this;
    setFrameShape(QFrame::NoFrame);
    setWidgetResizable(true);
    // 原生滚动条只负责维护滚动范围、响应滚轮与键盘，不显示；显示的是覆盖在视口上方的 ShadScrollBar
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    viewport()->setAutoFillBackground(false);
    // QScrollArea::setWidget 会强制开启内容组件的 autoFillBackground，用 Window 色填充背景；
    // 视口的 Window 色设为透明，内容组件继承后填充不可见，透出父组件绘制的主题背景
    QPalette viewportPalette = viewport()->palette();
    viewportPalette.setColor(QPalette::Window, QColor(0, 0, 0, 0));
    viewport()->setPalette(viewportPalette);

    d->_verticalBar = new ShadScrollBar(Qt::Vertical, this);
    d->_horizontalBar = new ShadScrollBar(Qt::Horizontal, this);
    d->_verticalBar->hide();
    d->_horizontalBar->hide();
    d->_hideTimer = new QTimer(this);
    d->_hideTimer->setSingleShot(true);
    d->_hideTimer->setInterval(ShadScrollAreaPrivate::kHideDelayMs);
    connect(d->_hideTimer, &QTimer::timeout, this, [d]() { d->updateOverlayBars(); });
    d->bindScrollBar(verticalScrollBar(), d->_verticalBar);
    d->bindScrollBar(horizontalScrollBar(), d->_horizontalBar);
}

ShadScrollArea::~ShadScrollArea()
{
}

ShadScrollBar* ShadScrollArea::getOverlayScrollBar(Qt::Orientation orientation) const
{
    Q_D(const ShadScrollArea);
    return orientation == Qt::Horizontal ? d->_horizontalBar : d->_verticalBar;
}

bool ShadScrollArea::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == widget() && event->type() == QEvent::LayoutRequest && widgetResizable())
    {
        setWidgetResizable(true);
    }
    return QScrollArea::eventFilter(watched, event);
}

void ShadScrollArea::enterEvent(QEnterEvent* event)
{
    Q_D(ShadScrollArea);
    d->_isHovered = true;
    d->_hideTimer->stop();
    d->updateOverlayBars();
    QScrollArea::enterEvent(event);
}

void ShadScrollArea::leaveEvent(QEvent* event)
{
    Q_D(ShadScrollArea);
    d->_isHovered = false;
    d->_hideTimer->start();
    QScrollArea::leaveEvent(event);
}

void ShadScrollArea::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadScrollArea);
    QScrollArea::resizeEvent(event);
    // 视口尺寸变化会改变原生滚动条的 pageStep，但不一定触发 rangeChanged
    d->syncScrollBar(verticalScrollBar(), d->_verticalBar);
    d->syncScrollBar(horizontalScrollBar(), d->_horizontalBar);
    d->updateOverlayBars();
}
