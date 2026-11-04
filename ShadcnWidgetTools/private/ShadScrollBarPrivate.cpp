#include "ShadScrollBarPrivate.h"

#include <QStyle>
#include <QtMath>

#include "ShadScrollBar.h"
#include "ShadScrollBarMetrics.h"
#include "ShadTransition.h"

ShadScrollBarPrivate::ShadScrollBarPrivate()
{
}

ShadScrollBarPrivate::~ShadScrollBarPrivate()
{
}

QRectF ShadScrollBarPrivate::trackRect() const
{
    Q_Q(const ShadScrollBar);
    // p-px：四周 1px 内边距，主轴两端再加 trackInset
    const qreal padding = ShadScrollBarMetrics::kPadding;
    if (q->orientation() == Qt::Horizontal)
    {
        return QRectF(padding + _trackInset, padding, q->width() - 2 * (padding + _trackInset), q->height() - 2 * padding);
    }
    return QRectF(padding, padding + _trackInset, q->width() - 2 * padding, q->height() - 2 * (padding + _trackInset));
}

bool ShadScrollBarPrivate::isReversed() const
{
    Q_Q(const ShadScrollBar);
    if (q->orientation() == Qt::Horizontal)
    {
        return q->invertedAppearance() != (q->layoutDirection() == Qt::RightToLeft);
    }
    return q->invertedAppearance();
}

QRectF ShadScrollBarPrivate::thumbRect() const
{
    Q_Q(const ShadScrollBar);
    const QRectF logicalRect = logicalThumbRect();
    if (logicalRect.isEmpty() || !isReversed())
    {
        return logicalRect;
    }
    // 沿主轴以组件中线为轴镜像（轨道两端留白对称，镜像后仍在轨道内）
    const QRectF bounds(q->rect());
    if (q->orientation() == Qt::Horizontal)
    {
        return QRectF(bounds.left() + bounds.right() - logicalRect.right(), logicalRect.top(), logicalRect.width(), logicalRect.height());
    }
    return QRectF(logicalRect.left(), bounds.top() + bounds.bottom() - logicalRect.bottom(), logicalRect.width(), logicalRect.height());
}

QRectF ShadScrollBarPrivate::logicalThumbRect() const
{
    Q_Q(const ShadScrollBar);
    const QRectF track = trackRect();
    const bool isHorizontal = q->orientation() == Qt::Horizontal;
    const qreal trackLength = isHorizontal ? track.width() : track.height();
    const qint64 range = static_cast<qint64>(q->maximum()) - q->minimum();
    if (range <= 0 || trackLength <= 0)
    {
        return QRectF();
    }
    // 滑块长度与可见比例成正比，并保证最小长度便于拖动
    const qreal pageStep = qMax(1, q->pageStep());
    const qreal minLength = qMin(static_cast<qreal>(ShadScrollBarMetrics::kMinThumbLength), trackLength);
    const qreal thumbLength = qBound(minLength, trackLength * pageStep / (range + pageStep), trackLength);
    const qreal available = trackLength - thumbLength;
    const qreal ratio = static_cast<qreal>(static_cast<qint64>(q->sliderPosition()) - q->minimum()) / range;
    const qreal start = (isHorizontal ? track.left() : track.top()) + available * qBound<qreal>(0.0, ratio, 1.0);
    if (isHorizontal)
    {
        return QRectF(start, track.top(), thumbLength, track.height());
    }
    return QRectF(track.left(), start, track.width(), thumbLength);
}

qreal ShadScrollBarPrivate::mainCoordinate(const QPoint& pos) const
{
    Q_Q(const ShadScrollBar);
    const bool isHorizontal = q->orientation() == Qt::Horizontal;
    const qreal coordinate = isHorizontal ? pos.x() : pos.y();
    if (!isReversed())
    {
        return coordinate;
    }
    // 与 thumbRect 的镜像方式一致，换算回逻辑坐标
    return (isHorizontal ? q->width() : q->height()) - coordinate;
}

int ShadScrollBarPrivate::valueFromThumbStart(qreal thumbStart) const
{
    Q_Q(const ShadScrollBar);
    const QRectF track = trackRect();
    const QRectF thumb = thumbRect();
    const bool isHorizontal = q->orientation() == Qt::Horizontal;
    const qreal trackStart = isHorizontal ? track.left() : track.top();
    const qreal available = (isHorizontal ? track.width() - thumb.width() : track.height() - thumb.height());
    if (available <= 0)
    {
        return q->minimum();
    }
    return QStyle::sliderValueFromPosition(q->minimum(), q->maximum(), qRound(thumbStart - trackStart), qRound(available));
}

void ShadScrollBarPrivate::updateHoverState(const QPoint& pos)
{
    Q_Q(ShadScrollBar);
    _isThumbHovered = q->rect().contains(pos) && thumbRect().contains(pos);
    _hoverTransition->animateTo((_isThumbHovered || _isDragging) ? 1.0 : 0.0);
}
