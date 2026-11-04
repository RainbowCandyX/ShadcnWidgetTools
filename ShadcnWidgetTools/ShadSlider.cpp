#include "ShadSlider.h"

#include <QColor>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QWheelEvent>
#include <QtGlobal>

#include <utility>

#include "ShadSliderMetrics.h"
#include "ShadSliderPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // 绘制单个滑块：bg-white border-primary shadow-sm，hover / focus-visible 时外圈 ring-4 ring-ring/50
    void paintThumb(QPainter* painter, const QRectF& thumbRect, qreal ringProgress)
    {
        using namespace ShadStyleHelper;
        const qreal thumbRadius = ShadSliderMetrics::kThumbSize / 2.0;
        if (ringProgress > 0.0)
        {
            drawFocusRing(painter, thumbRect, thumbRadius, withAlpha(shadTheme->getColor(ShadThemeType::Ring), 0.5 * ringProgress), ShadSliderMetrics::kRingWidth * ringProgress);
        }
        drawShadowSm(painter, thumbRect, thumbRadius);
        fillRoundedRect(painter, thumbRect, thumbRadius, QColor(Qt::white));
        strokeRoundedRect(painter, thumbRect, thumbRadius, shadTheme->getColor(ShadThemeType::Primary));
    }
} // namespace

ShadSlider::ShadSlider(QWidget* parent)
    : ShadSlider(Qt::Horizontal, parent)
{
}

ShadSlider::ShadSlider(Qt::Orientation orientation, QWidget* parent)
    : QSlider(orientation, parent), d_ptr(new ShadSliderPrivate())
{
    Q_D(ShadSlider);
    d->q_ptr = this;
    d->_upperRingTransition = new ShadTransition(this);
    d->_lowerRingTransition = new ShadTransition(this);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    // QSlider 会从样式读取布局项边距（macOS 上不为 0），组件自绘，直接使用控件矩形
    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadSlider::~ShadSlider()
{
}

void ShadSlider::setRangeMode(bool rangeMode)
{
    Q_D(ShadSlider);
    if (d->_isRangeMode == rangeMode)
    {
        return;
    }
    d->_isRangeMode = rangeMode;
    d->_activeThumb = ShadSliderPrivate::Thumb::Upper;
    if (rangeMode)
    {
        const int lowerValue = qBound(minimum(), d->_lowerValue, sliderPosition());
        if (lowerValue != d->_lowerValue)
        {
            d->_lowerValue = lowerValue;
            Q_EMIT lowerValueChanged(lowerValue);
        }
        Q_EMIT rangeValuesChanged(d->_lowerValue, value());
    }
    d->updateRingState();
    update();
}

bool ShadSlider::isRangeMode() const
{
    Q_D(const ShadSlider);
    return d->_isRangeMode;
}

void ShadSlider::setLowerValue(int value)
{
    Q_D(ShadSlider);
    int boundedValue = qBound(minimum(), value, maximum());
    if (d->_isRangeMode)
    {
        boundedValue = qMin(boundedValue, sliderPosition());
    }
    if (boundedValue == d->_lowerValue)
    {
        return;
    }
    d->_lowerValue = boundedValue;
    update();
    Q_EMIT lowerValueChanged(boundedValue);
    if (d->_isRangeMode)
    {
        Q_EMIT rangeValuesChanged(boundedValue, this->value());
    }
}

int ShadSlider::getLowerValue() const
{
    Q_D(const ShadSlider);
    return d->_lowerValue;
}

void ShadSlider::setRangeValues(int lower, int upper)
{
    if (upper < lower)
    {
        std::swap(lower, upper);
    }
    // 先设置上限，避免新的下限被旧上限截断
    setValue(upper);
    setLowerValue(lower);
}

QSize ShadSlider::sizeHint() const
{
    const int thickness = ShadSliderMetrics::kThumbSize + 2 * ShadSliderMetrics::kRingWidth;
    if (orientation() == Qt::Horizontal)
    {
        return QSize(ShadSliderMetrics::kDefaultLength, thickness);
    }
    return QSize(thickness, ShadSliderMetrics::kDefaultLength);
}

QSize ShadSlider::minimumSizeHint() const
{
    const int thickness = ShadSliderMetrics::kThumbSize + 2 * ShadSliderMetrics::kRingWidth;
    const int length = 2 * ShadSliderMetrics::kThumbSize + 2 * ShadSliderMetrics::kRingWidth;
    if (orientation() == Qt::Horizontal)
    {
        return QSize(length, thickness);
    }
    return QSize(thickness, length);
}

void ShadSlider::sliderChange(QAbstractSlider::SliderChange change)
{
    Q_D(ShadSlider);
    if (d->_isRangeMode && (change == QAbstractSlider::SliderValueChange || change == QAbstractSlider::SliderRangeChange))
    {
        // 上限或取值范围变化后，下限跟随调整以保持 lower <= upper
        const int lowerValue = qBound(minimum(), d->_lowerValue, value());
        if (lowerValue != d->_lowerValue)
        {
            d->_lowerValue = lowerValue;
            Q_EMIT lowerValueChanged(lowerValue);
        }
        if (change == QAbstractSlider::SliderValueChange)
        {
            Q_EMIT rangeValuesChanged(d->_lowerValue, value());
        }
    }
    QSlider::sliderChange(change);
}

void ShadSlider::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadSlider);
    if (!d->_isRangeMode)
    {
        QSlider::keyPressEvent(event);
        return;
    }
    // 范围模式下方向键作用于当前活动滑块：右 / 上增大，左 / 下减小
    const bool isLower = d->_activeThumb == ShadSliderPrivate::Thumb::Lower;
    const qint64 current = isLower ? d->_lowerValue : value();
    qint64 target = current;
    switch (event->key())
    {
        case Qt::Key_Right:
        case Qt::Key_Up:
            target = current + singleStep();
            break;
        case Qt::Key_Left:
        case Qt::Key_Down:
            target = current - singleStep();
            break;
        case Qt::Key_PageUp:
            target = current + pageStep();
            break;
        case Qt::Key_PageDown:
            target = current - pageStep();
            break;
        case Qt::Key_Home:
            target = minimum();
            break;
        case Qt::Key_End:
            target = maximum();
            break;
        default:
            QSlider::keyPressEvent(event);
            return;
    }
    const int boundedTarget = static_cast<int>(qBound(static_cast<qint64>(minimum()), target, static_cast<qint64>(maximum())));
    if (isLower)
    {
        setLowerValue(boundedTarget);
    }
    else
    {
        setValue(qMax(boundedTarget, d->_lowerValue));
    }
    event->accept();
}

void ShadSlider::wheelEvent(QWheelEvent* event)
{
    Q_D(ShadSlider);
    // 范围模式下滚轮无法确定作用于哪个滑块，交给父组件处理（例如滚动页面）
    if (d->_isRangeMode)
    {
        event->ignore();
        return;
    }
    QSlider::wheelEvent(event);
}

void ShadSlider::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadSlider);
    if (event->button() != Qt::LeftButton || !isEnabled())
    {
        QSlider::mousePressEvent(event);
        return;
    }
    // 点击轨道任意位置时离鼠标更近的滑块直接跳到该位置并开始拖动
    const QPoint pos = event->position().toPoint();
    ShadSliderPrivate::Thumb thumb = d->thumbAt(pos);
    if (thumb == ShadSliderPrivate::Thumb::None)
    {
        thumb = d->nearestThumb(pos);
    }
    d->_draggingThumb = thumb;
    d->_activeThumb = thumb;
    if (thumb == ShadSliderPrivate::Thumb::Upper)
    {
        setSliderDown(true);
    }
    d->moveThumb(thumb, d->valueFromPosition(pos));
    d->updateRingState();
    event->accept();
}

void ShadSlider::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadSlider);
    const QPoint pos = event->position().toPoint();
    if (d->_draggingThumb != ShadSliderPrivate::Thumb::None)
    {
        d->moveThumb(d->_draggingThumb, d->valueFromPosition(pos));
        event->accept();
        return;
    }
    const ShadSliderPrivate::Thumb hoveredThumb = d->thumbAt(pos);
    if (hoveredThumb != d->_hoveredThumb)
    {
        d->_hoveredThumb = hoveredThumb;
        d->updateRingState();
    }
    event->accept();
}

void ShadSlider::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadSlider);
    if (event->button() != Qt::LeftButton || d->_draggingThumb == ShadSliderPrivate::Thumb::None)
    {
        QSlider::mouseReleaseEvent(event);
        return;
    }
    if (d->_draggingThumb == ShadSliderPrivate::Thumb::Upper)
    {
        setSliderDown(false);
    }
    d->_draggingThumb = ShadSliderPrivate::Thumb::None;
    d->_hoveredThumb = d->thumbAt(event->position().toPoint());
    d->updateRingState();
    event->accept();
}

void ShadSlider::leaveEvent(QEvent* event)
{
    Q_D(ShadSlider);
    d->_hoveredThumb = ShadSliderPrivate::Thumb::None;
    d->updateRingState();
    QSlider::leaveEvent(event);
}

void ShadSlider::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadSlider);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QSlider::focusInEvent(event);
    d->updateRingState();
}

void ShadSlider::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadSlider);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QSlider::focusOutEvent(event);
    d->updateRingState();
}

void ShadSlider::changeEvent(QEvent* event)
{
    Q_D(ShadSlider);
    if (event->type() == QEvent::EnabledChange)
    {
        if (!isEnabled() && d->_draggingThumb != ShadSliderPrivate::Thumb::None)
        {
            if (d->_draggingThumb == ShadSliderPrivate::Thumb::Upper)
            {
                setSliderDown(false);
            }
            d->_draggingThumb = ShadSliderPrivate::Thumb::None;
        }
        d->updateRingState();
        update();
    }
    QSlider::changeEvent(event);
}

void ShadSlider::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadSlider);
    using namespace ShadStyleHelper;
    const QRectF trackRect = d->trackRect();
    const qreal trackRadius = ShadSliderMetrics::kTrackThickness / 2.0;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    // 轨道 bg-muted，已选范围 bg-primary（裁剪在圆角轨道内）
    fillRoundedRect(&painter, trackRect, trackRadius, shadTheme->getColor(ShadThemeType::Muted));
    QPainterPath trackPath;
    trackPath.addRoundedRect(trackRect, trackRadius, trackRadius);
    painter.save();
    painter.setClipPath(trackPath);
    painter.fillRect(d->rangeRect(), shadTheme->getColor(ShadThemeType::Primary));
    painter.restore();

    // 下限滑块先画，上限滑块在上层
    if (d->_isRangeMode)
    {
        paintThumb(&painter, d->thumbRect(ShadSliderPrivate::Thumb::Lower), d->_lowerRingTransition->value());
    }
    paintThumb(&painter, d->thumbRect(ShadSliderPrivate::Thumb::Upper), d->_upperRingTransition->value());
}
