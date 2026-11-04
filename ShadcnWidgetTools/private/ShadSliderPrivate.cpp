#include "ShadSliderPrivate.h"

#include <QStyle>
#include <QtGlobal>

#include "ShadSlider.h"
#include "ShadSliderMetrics.h"
#include "ShadTransition.h"

ShadSliderPrivate::ShadSliderPrivate()
{
}

ShadSliderPrivate::~ShadSliderPrivate()
{
}

bool ShadSliderPrivate::isUpsideDown() const
{
    Q_Q(const ShadSlider);
    if (q->orientation() == Qt::Horizontal)
    {
        return q->invertedAppearance() != (q->layoutDirection() == Qt::RightToLeft);
    }
    return !q->invertedAppearance();
}

int ShadSliderPrivate::thumbSpan() const
{
    Q_Q(const ShadSlider);
    const int extent = q->orientation() == Qt::Horizontal ? q->width() : q->height();
    return qMax(0, extent - 2 * ShadSliderMetrics::kRingWidth - ShadSliderMetrics::kThumbSize);
}

int ShadSliderPrivate::positionOfValue(int value) const
{
    Q_Q(const ShadSlider);
    return QStyle::sliderPositionFromValue(q->minimum(), q->maximum(), value, thumbSpan(), isUpsideDown());
}

QPointF ShadSliderPrivate::thumbCenter(int value) const
{
    Q_Q(const ShadSlider);
    const qreal offset = ShadSliderMetrics::kRingWidth + ShadSliderMetrics::kThumbSize / 2.0 + positionOfValue(value);
    if (q->orientation() == Qt::Horizontal)
    {
        return QPointF(offset, q->height() / 2.0);
    }
    return QPointF(q->width() / 2.0, offset);
}

QRectF ShadSliderPrivate::thumbRect(int value) const
{
    const QPointF center = thumbCenter(value);
    const qreal half = ShadSliderMetrics::kThumbSize / 2.0;
    return QRectF(center.x() - half, center.y() - half, ShadSliderMetrics::kThumbSize, ShadSliderMetrics::kThumbSize);
}

QRectF ShadSliderPrivate::thumbRect(Thumb thumb) const
{
    Q_Q(const ShadSlider);
    return thumbRect(thumb == Thumb::Lower ? _lowerValue : q->sliderPosition());
}

QRectF ShadSliderPrivate::trackRect() const
{
    Q_Q(const ShadSlider);
    const qreal margin = ShadSliderMetrics::kRingWidth;
    const qreal thickness = ShadSliderMetrics::kTrackThickness;
    if (q->orientation() == Qt::Horizontal)
    {
        return QRectF(margin, (q->height() - thickness) / 2.0, q->width() - 2 * margin, thickness);
    }
    return QRectF((q->width() - thickness) / 2.0, margin, thickness, q->height() - 2 * margin);
}

QRectF ShadSliderPrivate::rangeRect() const
{
    Q_Q(const ShadSlider);
    const QRectF track = trackRect();
    const bool isHorizontal = q->orientation() == Qt::Horizontal;
    const QPointF upperCenter = thumbCenter(q->sliderPosition());
    qreal start = 0.0;
    qreal end = 0.0;
    if (_isRangeMode)
    {
        // 范围模式填充两个滑块中心之间的区域
        const QPointF lowerCenter = thumbCenter(_lowerValue);
        const qreal lowerCoordinate = isHorizontal ? lowerCenter.x() : lowerCenter.y();
        const qreal upperCoordinate = isHorizontal ? upperCenter.x() : upperCenter.y();
        start = qMin(lowerCoordinate, upperCoordinate);
        end = qMax(lowerCoordinate, upperCoordinate);
    }
    else
    {
        // 最小值所在端点为 0 时从轨道起点填充到滑块中心，否则从滑块中心填充到轨道终点
        const qreal centerCoordinate = isHorizontal ? upperCenter.x() : upperCenter.y();
        const bool fillFromStart = positionOfValue(q->minimum()) == 0;
        const qreal trackStart = isHorizontal ? track.left() : track.top();
        const qreal trackEnd = isHorizontal ? track.right() : track.bottom();
        start = fillFromStart ? trackStart : centerCoordinate;
        end = fillFromStart ? centerCoordinate : trackEnd;
    }
    if (isHorizontal)
    {
        return QRectF(start, track.top(), end - start, track.height());
    }
    return QRectF(track.left(), start, track.width(), end - start);
}

int ShadSliderPrivate::valueFromPosition(const QPoint& pos) const
{
    Q_Q(const ShadSlider);
    const int start = ShadSliderMetrics::kRingWidth + ShadSliderMetrics::kThumbSize / 2;
    const int coordinate = (q->orientation() == Qt::Horizontal ? pos.x() : pos.y()) - start;
    int value = QStyle::sliderValueFromPosition(q->minimum(), q->maximum(), coordinate, thumbSpan(), isUpsideDown());
    // 按 singleStep 吸附，对应 Radix Slider 的 step
    const int step = qMax(1, q->singleStep());
    if (step > 1)
    {
        const qint64 offset = static_cast<qint64>(value) - q->minimum();
        const qint64 snapped = q->minimum() + qRound64(static_cast<double>(offset) / step) * step;
        // 三个参数统一为 qint64，避免 Qt 6.12 混合类型 qBound 重载产生歧义
        value = static_cast<int>(qBound(static_cast<qint64>(q->minimum()), snapped, static_cast<qint64>(q->maximum())));
    }
    return value;
}

ShadSliderPrivate::Thumb ShadSliderPrivate::thumbAt(const QPoint& pos) const
{
    if (thumbRect(Thumb::Upper).contains(pos))
    {
        return Thumb::Upper;
    }
    if (_isRangeMode && thumbRect(Thumb::Lower).contains(pos))
    {
        return Thumb::Lower;
    }
    return Thumb::None;
}

ShadSliderPrivate::Thumb ShadSliderPrivate::nearestThumb(const QPoint& pos) const
{
    Q_Q(const ShadSlider);
    if (!_isRangeMode)
    {
        return Thumb::Upper;
    }
    const bool isHorizontal = q->orientation() == Qt::Horizontal;
    const qreal coordinate = isHorizontal ? pos.x() : pos.y();
    const QPointF lowerCenter = thumbCenter(_lowerValue);
    const QPointF upperCenter = thumbCenter(q->sliderPosition());
    const qreal lowerDistance = qAbs(coordinate - (isHorizontal ? lowerCenter.x() : lowerCenter.y()));
    const qreal upperDistance = qAbs(coordinate - (isHorizontal ? upperCenter.x() : upperCenter.y()));
    if (!qFuzzyCompare(lowerDistance + 1.0, upperDistance + 1.0))
    {
        return lowerDistance < upperDistance ? Thumb::Lower : Thumb::Upper;
    }
    // 两个滑块重叠时按点击位置对应的数值决定方向
    return valueFromPosition(pos) < _lowerValue ? Thumb::Lower : Thumb::Upper;
}

void ShadSliderPrivate::moveThumb(Thumb thumb, int value)
{
    Q_Q(ShadSlider);
    if (thumb == Thumb::Lower)
    {
        q->setLowerValue(qMin(value, q->sliderPosition()));
        return;
    }
    q->setSliderPosition(_isRangeMode ? qMax(value, _lowerValue) : value);
}

void ShadSliderPrivate::updateRingState()
{
    Q_Q(ShadSlider);
    const bool isEnabled = q->isEnabled();
    const bool isKeyboardFocused = q->hasFocus() && _isFocusVisible;
    const bool isUpperActive = isEnabled && (_hoveredThumb == Thumb::Upper || _draggingThumb == Thumb::Upper || (isKeyboardFocused && _activeThumb == Thumb::Upper));
    const bool isLowerActive = isEnabled && _isRangeMode && (_hoveredThumb == Thumb::Lower || _draggingThumb == Thumb::Lower || (isKeyboardFocused && _activeThumb == Thumb::Lower));
    _upperRingTransition->animateTo(isUpperActive ? 1.0 : 0.0);
    _lowerRingTransition->animateTo(isLowerActive ? 1.0 : 0.0);
}
