#include "ShadHueSlider.h"

#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadSliderMetrics.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // 可视宽度，与 ShadColorArea 相同
    constexpr int kTrackLength = 240;
    // 轨道高度
    constexpr int kTrackHeight = 12;
    // 渐变色标数量：每 60 度一个
    constexpr int kGradientStops = 6;
    // 键盘步长（度）
    constexpr qreal kKeyStepDegrees = 1.0;
    constexpr qreal kLargeKeyStepDegrees = 10.0;
    constexpr qreal kFullCircleDegrees = 360.0;

    constexpr int kMargin = ShadSliderMetrics::kRingWidth;
    constexpr qreal kThumbRadius = ShadSliderMetrics::kThumbSize / 2.0;
} // namespace

ShadHueSlider::ShadHueSlider(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    _ringTransition = new ShadTransition(this);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadHueSlider::~ShadHueSlider()
{
}

void ShadHueSlider::setHue(qreal hue)
{
    _hue = qBound(0.0, hue, 1.0);
    update();
}

qreal ShadHueSlider::getHue() const
{
    return _hue;
}

QSize ShadHueSlider::sizeHint() const
{
    return QSize(kTrackLength + 2 * kMargin, ShadSliderMetrics::kThumbSize + 2 * kMargin);
}

QSize ShadHueSlider::minimumSizeHint() const
{
    return sizeHint();
}

void ShadHueSlider::enterEvent(QEnterEvent* event)
{
    _isHovered = true;
    updateRing();
    QWidget::enterEvent(event);
}

void ShadHueSlider::leaveEvent(QEvent* event)
{
    _isHovered = false;
    updateRing();
    QWidget::leaveEvent(event);
}

void ShadHueSlider::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QWidget::focusInEvent(event);
    updateRing();
}

void ShadHueSlider::focusOutEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QWidget::focusOutEvent(event);
    updateRing();
}

void ShadHueSlider::keyPressEvent(QKeyEvent* event)
{
    const qreal step = (event->modifiers().testFlag(Qt::ShiftModifier) ? kLargeKeyStepDegrees : kKeyStepDegrees) / kFullCircleDegrees;
    switch (event->key())
    {
        case Qt::Key_Left:
        case Qt::Key_Down:
            applyUserHue(_hue - step);
            break;
        case Qt::Key_Right:
        case Qt::Key_Up:
            applyUserHue(_hue + step);
            break;
        case Qt::Key_Home:
            applyUserHue(0.0);
            break;
        case Qt::Key_End:
            applyUserHue(1.0);
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    event->accept();
}

void ShadHueSlider::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    event->accept();
    updateFromPosition(event->position().x());
}

void ShadHueSlider::mouseMoveEvent(QMouseEvent* event)
{
    if (!event->buttons().testFlag(Qt::LeftButton))
    {
        QWidget::mouseMoveEvent(event);
        return;
    }
    event->accept();
    updateFromPosition(event->position().x());
}

void ShadHueSlider::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF trackRect(kMargin, (height() - kTrackHeight) / 2.0, width() - 2 * kMargin, kTrackHeight);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 0 到 360 度的彩虹渐变，两端同为红色
    QLinearGradient gradient(trackRect.topLeft(), trackRect.topRight());
    for (int index = 0; index <= kGradientStops; ++index)
    {
        const qreal position = static_cast<qreal>(index) / kGradientStops;
        gradient.setColorAt(position, QColor::fromHsvF(position, 1.0, 1.0));
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawRoundedRect(trackRect, kTrackHeight / 2.0, kTrackHeight / 2.0);

    // 滑块：bg-white border-primary shadow-sm，hover / focus-visible 时外圈 ring-4 ring-ring/50
    const qreal travel = trackRect.width() - 2 * kThumbRadius;
    const QPointF center(trackRect.left() + kThumbRadius + _hue * travel, height() / 2.0);
    const QRectF thumbRect(center.x() - kThumbRadius, center.y() - kThumbRadius, ShadSliderMetrics::kThumbSize, ShadSliderMetrics::kThumbSize);
    const qreal ringProgress = _ringTransition->value();
    if (ringProgress > 0.0)
    {
        drawFocusRing(&painter, thumbRect, kThumbRadius, withAlpha(shadTheme->getColor(ShadThemeType::Ring), 0.5 * ringProgress), ShadSliderMetrics::kRingWidth * ringProgress);
    }
    drawShadowSm(&painter, thumbRect, kThumbRadius);
    fillRoundedRect(&painter, thumbRect, kThumbRadius, QColor(Qt::white));
    strokeRoundedRect(&painter, thumbRect, kThumbRadius, shadTheme->getColor(ShadThemeType::Primary));
}

void ShadHueSlider::updateFromPosition(qreal x)
{
    const qreal travel = width() - 2 * kMargin - 2 * kThumbRadius;
    if (travel <= 0.0)
    {
        return;
    }
    applyUserHue((x - kMargin - kThumbRadius) / travel);
}

void ShadHueSlider::applyUserHue(qreal hue)
{
    const qreal boundedHue = qBound(0.0, hue, 1.0);
    if (qFuzzyCompare(boundedHue + 1.0, _hue + 1.0))
    {
        return;
    }
    _hue = boundedHue;
    update();
    Q_EMIT hueChanged(_hue);
}

void ShadHueSlider::updateRing()
{
    const bool isRingVisible = _isHovered || (_isFocusVisible && hasFocus());
    _ringTransition->animateTo(isRingVisible ? 1.0 : 0.0);
}
