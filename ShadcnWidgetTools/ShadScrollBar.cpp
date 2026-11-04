#include "ShadScrollBar.h"

#include <QColor>
#include <QContextMenuEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QtGlobal>

#include "ShadScrollBarMetrics.h"
#include "ShadScrollBarPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

ShadScrollBar::ShadScrollBar(QWidget* parent)
    : ShadScrollBar(Qt::Vertical, parent)
{
}

ShadScrollBar::ShadScrollBar(Qt::Orientation orientation, QWidget* parent)
    : QScrollBar(orientation, parent), d_ptr(new ShadScrollBarPrivate())
{
    Q_D(ShadScrollBar);
    d->q_ptr = this;
    d->_hoverTransition = new ShadTransition(this);
    setMouseTracking(true);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadScrollBar::~ShadScrollBar()
{
}

void ShadScrollBar::setTrackInset(int inset)
{
    Q_D(ShadScrollBar);
    d->_trackInset = qMax(0, inset);
    update();
}

int ShadScrollBar::getTrackInset() const
{
    Q_D(const ShadScrollBar);
    return d->_trackInset;
}

QSize ShadScrollBar::sizeHint() const
{
    if (orientation() == Qt::Horizontal)
    {
        return QSize(ShadScrollBarMetrics::kDefaultLength, ShadScrollBarMetrics::kThickness);
    }
    return QSize(ShadScrollBarMetrics::kThickness, ShadScrollBarMetrics::kDefaultLength);
}

QSize ShadScrollBar::minimumSizeHint() const
{
    if (orientation() == Qt::Horizontal)
    {
        return QSize(ShadScrollBarMetrics::kMinThumbLength, ShadScrollBarMetrics::kThickness);
    }
    return QSize(ShadScrollBarMetrics::kThickness, ShadScrollBarMetrics::kMinThumbLength);
}

void ShadScrollBar::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadScrollBar);
    if (event->button() != Qt::LeftButton || maximum() <= minimum())
    {
        event->ignore();
        return;
    }
    const QPoint pos = event->position().toPoint();
    // 命中判断用实际位置，拖动偏移与按页方向用逻辑坐标（RTL 等反向显示时两者镜像）
    const QRectF logicalThumb = d->logicalThumbRect();
    const qreal coordinate = d->mainCoordinate(pos);
    const qreal thumbStart = orientation() == Qt::Horizontal ? logicalThumb.left() : logicalThumb.top();
    if (d->thumbRect().contains(pos))
    {
        d->_isDragging = true;
        d->_dragOffset = coordinate - thumbStart;
        setSliderDown(true);
    }
    else
    {
        // 点击轨道空白处按页滚动
        triggerAction(coordinate < thumbStart ? QAbstractSlider::SliderPageStepSub : QAbstractSlider::SliderPageStepAdd);
    }
    d->updateHoverState(pos);
    event->accept();
}

void ShadScrollBar::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadScrollBar);
    const QPoint pos = event->position().toPoint();
    if (d->_isDragging)
    {
        setSliderPosition(d->valueFromThumbStart(d->mainCoordinate(pos) - d->_dragOffset));
    }
    d->updateHoverState(pos);
    event->accept();
}

void ShadScrollBar::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadScrollBar);
    if (event->button() == Qt::LeftButton && d->_isDragging)
    {
        d->_isDragging = false;
        setSliderDown(false);
    }
    d->updateHoverState(event->position().toPoint());
    event->accept();
}

void ShadScrollBar::leaveEvent(QEvent* event)
{
    Q_D(ShadScrollBar);
    d->_isThumbHovered = false;
    d->_hoverTransition->animateTo(d->_isDragging ? 1.0 : 0.0);
    QScrollBar::leaveEvent(event);
}

void ShadScrollBar::contextMenuEvent(QContextMenuEvent* event)
{
    // 不显示原生滚动条的"滚动到此处"菜单
    event->accept();
}

void ShadScrollBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadScrollBar);
    const QRectF thumb = d->thumbRect();
    if (thumb.isEmpty())
    {
        return;
    }
    using namespace ShadStyleHelper;
    // 滑块 bg-border，悬停或拖动时加深
    const QColor normalColor = shadTheme->getColor(ShadThemeType::Border);
    const QColor activeColor = withAlpha(shadTheme->getColor(ShadThemeType::MutedForeground), 0.5);
    const qreal radius = qMin(thumb.width(), thumb.height()) / 2.0;
    QPainter painter(this);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    fillRoundedRect(&painter, thumb, radius, mix(normalColor, activeColor, d->_hoverTransition->value()));
}
