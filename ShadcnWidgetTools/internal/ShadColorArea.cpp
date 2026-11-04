#include "ShadColorArea.h"

#include <QColor>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 可视区域边长
    constexpr int kAreaSize = 240;
    // 取色点半径与圆环宽度
    constexpr qreal kThumbRadius = 7.0;
    constexpr qreal kThumbRingWidth = 2.0;
    // 键盘步长
    constexpr qreal kKeyStep = 0.01;
    constexpr qreal kLargeKeyStep = 0.1;
} // namespace

ShadColorArea::ShadColorArea(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::CrossCursor);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadColorArea::~ShadColorArea()
{
}

void ShadColorArea::setHue(qreal hue)
{
    _hue = qBound(0.0, hue, 1.0);
    update();
}

void ShadColorArea::setSaturationValue(qreal saturation, qreal value)
{
    _saturation = qBound(0.0, saturation, 1.0);
    _value = qBound(0.0, value, 1.0);
    update();
}

qreal ShadColorArea::getSaturation() const
{
    return _saturation;
}

qreal ShadColorArea::getValue() const
{
    return _value;
}

QSize ShadColorArea::sizeHint() const
{
    const int side = kAreaSize + 2 * ShadStyleHelper::kFocusRingWidth;
    return QSize(side, side);
}

QSize ShadColorArea::minimumSizeHint() const
{
    return sizeHint();
}

void ShadColorArea::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QWidget::focusInEvent(event);
    update();
}

void ShadColorArea::focusOutEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QWidget::focusOutEvent(event);
    update();
}

void ShadColorArea::keyPressEvent(QKeyEvent* event)
{
    const qreal step = event->modifiers().testFlag(Qt::ShiftModifier) ? kLargeKeyStep : kKeyStep;
    qreal saturation = _saturation;
    qreal value = _value;
    switch (event->key())
    {
        case Qt::Key_Left:
            saturation -= step;
            break;
        case Qt::Key_Right:
            saturation += step;
            break;
        case Qt::Key_Up:
            value += step;
            break;
        case Qt::Key_Down:
            value -= step;
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    event->accept();
    applyUserValue(saturation, value);
}

void ShadColorArea::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    event->accept();
    updateFromPosition(event->position());
}

void ShadColorArea::mouseMoveEvent(QMouseEvent* event)
{
    if (!event->buttons().testFlag(Qt::LeftButton))
    {
        QWidget::mouseMoveEvent(event);
        return;
    }
    event->accept();
    updateFromPosition(event->position());
}

void ShadColorArea::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF areaRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 色相底色，叠加从左到右的白色渐变（饱和度）与从上到下的黑色渐变（明度）
    painter.save();
    painter.setClipPath(roundedRectPath(areaRect, radius));
    painter.fillRect(areaRect, QColor::fromHsvF(_hue, 1.0, 1.0));
    QLinearGradient saturationGradient(areaRect.topLeft(), areaRect.topRight());
    saturationGradient.setColorAt(0.0, QColor(255, 255, 255, 255));
    saturationGradient.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.fillRect(areaRect, saturationGradient);
    QLinearGradient valueGradient(areaRect.topLeft(), areaRect.bottomLeft());
    valueGradient.setColorAt(0.0, QColor(0, 0, 0, 0));
    valueGradient.setColorAt(1.0, QColor(0, 0, 0, 255));
    painter.fillRect(areaRect, valueGradient);
    painter.restore();

    strokeRoundedRect(&painter, areaRect, radius, shadTheme->getColor(ShadThemeType::Border));
    if (_isFocusVisible && hasFocus())
    {
        drawFocusRing(&painter, areaRect, radius, focusRingColor(false));
    }

    // 取色点：外圈细阴影 + 白色圆环，中间为当前颜色
    const QPointF center(areaRect.left() + _saturation * areaRect.width(), areaRect.top() + (1.0 - _value) * areaRect.height());
    painter.setPen(QPen(QColor(0, 0, 0, 77), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(center, kThumbRadius + kThumbRingWidth / 2.0 + 0.5, kThumbRadius + kThumbRingWidth / 2.0 + 0.5);
    painter.setPen(QPen(QColor(Qt::white), kThumbRingWidth));
    painter.setBrush(QColor::fromHsvF(_hue, _saturation, _value));
    painter.drawEllipse(center, kThumbRadius, kThumbRadius);
}

void ShadColorArea::updateFromPosition(const QPointF& position)
{
    const QRectF areaRect = ShadStyleHelper::insetRect(rect(), ShadStyleHelper::kFocusRingWidth);
    if (areaRect.width() <= 0.0 || areaRect.height() <= 0.0)
    {
        return;
    }
    const qreal saturation = (position.x() - areaRect.left()) / areaRect.width();
    const qreal value = 1.0 - (position.y() - areaRect.top()) / areaRect.height();
    applyUserValue(saturation, value);
}

void ShadColorArea::applyUserValue(qreal saturation, qreal value)
{
    const qreal boundedSaturation = qBound(0.0, saturation, 1.0);
    const qreal boundedValue = qBound(0.0, value, 1.0);
    if (qFuzzyCompare(boundedSaturation + 1.0, _saturation + 1.0) && qFuzzyCompare(boundedValue + 1.0, _value + 1.0))
    {
        return;
    }
    _saturation = boundedSaturation;
    _value = boundedValue;
    update();
    Q_EMIT saturationValueChanged(_saturation, _value);
}
