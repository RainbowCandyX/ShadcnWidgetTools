#include "ShadRadialProgress.h"

#include <QColor>
#include <QEasingCurve>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadRadialProgressPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    constexpr int kMinDiameter = 16;
    constexpr int kMinFontSize = 12;
    constexpr qreal kFontRatio = 0.2;
    constexpr qreal kGaugeTextOffsetRatio = 0.06;
    constexpr int kCircleStartAngle = 90 * 16;
    constexpr int kCircleSpan = 360 * 16;
    constexpr int kGaugeStartAngle = 225 * 16;
    constexpr int kGaugeSpan = 270 * 16;
    constexpr int kValueDurationMs = 300;
    constexpr int kRapidChangeIntervalMs = 100;
}

ShadRadialProgress::ShadRadialProgress(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadRadialProgressPrivate())
{
    Q_D(ShadRadialProgress);
    d->q_ptr = this;
    d->_valueTransition = new ShadTransition(this, kValueDurationMs);
    d->_valueTransition->setScaledByDistance(false);
    d->_valueTransition->setEasingCurve(QEasingCurve(QEasingCurve::OutCubic));
    d->_valueTransition->setRapidChangeInterval(kRapidChangeIntervalMs);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadRadialProgress::~ShadRadialProgress()
{
}

void ShadRadialProgress::setValue(int value)
{
    Q_D(ShadRadialProgress);
    value = qBound(0, value, 100);
    if (d->_value == value)
    {
        return;
    }
    d->_value = value;
    d->_valueTransition->animateTo(value / 100.0);
    Q_EMIT valueChanged(value);
}

int ShadRadialProgress::getValue() const
{
    Q_D(const ShadRadialProgress);
    return d->_value;
}

void ShadRadialProgress::setVariant(ShadRadialProgressType::Variant variant)
{
    Q_D(ShadRadialProgress);
    if (d->_variant == variant)
    {
        return;
    }
    d->_variant = variant;
    update();
}

ShadRadialProgressType::Variant ShadRadialProgress::getVariant() const
{
    Q_D(const ShadRadialProgress);
    return d->_variant;
}

void ShadRadialProgress::setStatus(ShadRadialProgressType::Status status)
{
    Q_D(ShadRadialProgress);
    if (d->_status == status)
    {
        return;
    }
    d->_status = status;
    update();
}

ShadRadialProgressType::Status ShadRadialProgress::getStatus() const
{
    Q_D(const ShadRadialProgress);
    return d->_status;
}

void ShadRadialProgress::setDiameter(int diameter)
{
    Q_D(ShadRadialProgress);
    diameter = qMax(kMinDiameter, diameter);
    if (d->_diameter == diameter)
    {
        return;
    }
    d->_diameter = diameter;
    updateGeometry();
    update();
}

int ShadRadialProgress::getDiameter() const
{
    Q_D(const ShadRadialProgress);
    return d->_diameter;
}

void ShadRadialProgress::setThickness(int thickness)
{
    Q_D(ShadRadialProgress);
    thickness = qMax(1, thickness);
    if (d->_thickness == thickness)
    {
        return;
    }
    d->_thickness = thickness;
    update();
}

int ShadRadialProgress::getThickness() const
{
    Q_D(const ShadRadialProgress);
    return d->_thickness;
}

void ShadRadialProgress::setTextVisible(bool visible)
{
    Q_D(ShadRadialProgress);
    if (d->_textVisible == visible)
    {
        return;
    }
    d->_textVisible = visible;
    update();
}

bool ShadRadialProgress::isTextVisible() const
{
    Q_D(const ShadRadialProgress);
    return d->_textVisible;
}

void ShadRadialProgress::setText(const QString& text)
{
    Q_D(ShadRadialProgress);
    if (d->_text == text)
    {
        return;
    }
    d->_text = text;
    update();
}

QString ShadRadialProgress::getText() const
{
    Q_D(const ShadRadialProgress);
    return d->_text;
}

QSize ShadRadialProgress::sizeHint() const
{
    Q_D(const ShadRadialProgress);
    return QSize(d->_diameter, d->_diameter);
}

QSize ShadRadialProgress::minimumSizeHint() const
{
    return sizeHint();
}

void ShadRadialProgress::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadRadialProgress);
    using namespace ShadStyleHelper;
    const bool isGauge = d->_variant == ShadRadialProgressType::Gauge;
    const qreal thickness = qMin<qreal>(d->_thickness, d->_diameter / 2.0);
    const QPointF center(width() / 2.0, height() / 2.0);
    const qreal arcSize = qMin(width(), height()) - thickness;
    const QRectF arcRect(center.x() - arcSize / 2.0, center.y() - arcSize / 2.0, arcSize, arcSize);
    const QColor color = d->statusColor();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    QPen trackPen(withAlpha(color, 0.2), thickness);
    trackPen.setCapStyle(Qt::RoundCap);
    painter.setPen(trackPen);
    painter.setBrush(Qt::NoBrush);
    if (isGauge)
    {
        painter.drawArc(arcRect, kGaugeStartAngle, -kGaugeSpan);
    }
    else
    {
        painter.drawEllipse(arcRect);
    }

    const qreal ratio = qBound<qreal>(0.0, d->_valueTransition->value(), 1.0);
    if (ratio > 0.0)
    {
        QPen indicatorPen(color, thickness);
        indicatorPen.setCapStyle(Qt::RoundCap);
        painter.setPen(indicatorPen);
        const int totalSpan = isGauge ? kGaugeSpan : kCircleSpan;
        const int startAngle = isGauge ? kGaugeStartAngle : kCircleStartAngle;
        painter.drawArc(arcRect, startAngle, -qRound(totalSpan * ratio));
    }

    if (!d->_textVisible)
    {
        return;
    }
    const QString text = d->displayText();
    if (text.isEmpty())
    {
        return;
    }
    QFont textFont = font();
    textFont.setPixelSize(qMax(kMinFontSize, qRound(d->_diameter * kFontRatio)));
    textFont.setWeight(QFont::DemiBold);
    painter.setFont(textFont);
    painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
    const qreal textOffset = isGauge ? d->_diameter * kGaugeTextOffsetRatio : 0.0;
    const QRectF textRect = QRectF(rect()).translated(0.0, textOffset);
    painter.drawText(textRect, Qt::AlignCenter, text);
}
