#include "ShadMarquee.h"

#include <QColor>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QEvent>
#include <QFont>
#include <QFontMetricsF>
#include <QHideEvent>
#include <QImage>
#include <QLinearGradient>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtMath>

#include <cmath>

#include "ShadMarqueePrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kFontPixelSize = 14;
    constexpr int kMinimumHeight = 20;
    constexpr int kDefaultWidth = 240;
    constexpr int kMinimumWidth = 40;
} // namespace

ShadMarquee::ShadMarquee(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadMarqueePrivate())
{
    Q_D(ShadMarquee);
    d->q_ptr = this;
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    d->_animation = new QVariantAnimation(this);
    d->_animation->setStartValue(0.0);
    d->_animation->setEndValue(1.0);
    d->_animation->setLoopCount(-1);
    d->_animation->setEasingCurve(QEasingCurve::Linear);
    connect(d->_animation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_offset = value.toReal();
        update();
    });
    ShadStyleHelper::applyFont(this, kFontPixelSize, QFont::Normal);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadMarquee::ShadMarquee(const QString& text, QWidget* parent)
    : ShadMarquee(parent)
{
    setText(text);
}

ShadMarquee::~ShadMarquee()
{
    Q_D(ShadMarquee);
    // 私有数据先于子对象释放，断开捕获 d 的连接
    QObject::disconnect(d->_animation, nullptr, this, nullptr);
}

void ShadMarquee::setText(const QString& text)
{
    Q_D(ShadMarquee);
    if (d->_text == text)
    {
        return;
    }
    d->_text = text;
    d->updateTextWidth();
}

QString ShadMarquee::getText() const
{
    Q_D(const ShadMarquee);
    return d->_text;
}

void ShadMarquee::setSpeed(int speed)
{
    Q_D(ShadMarquee);
    const int value = qMax(0, speed);
    if (d->_speed == value)
    {
        return;
    }
    d->_speed = value;
    d->configureAnimation();
}

int ShadMarquee::getSpeed() const
{
    Q_D(const ShadMarquee);
    return d->_speed;
}

void ShadMarquee::setGap(int gap)
{
    Q_D(ShadMarquee);
    const int value = qMax(0, gap);
    if (d->_gap == value)
    {
        return;
    }
    d->_gap = value;
    d->configureAnimation();
}

int ShadMarquee::getGap() const
{
    Q_D(const ShadMarquee);
    return d->_gap;
}

void ShadMarquee::setReversed(bool reversed)
{
    Q_D(ShadMarquee);
    if (d->_isReversed == reversed)
    {
        return;
    }
    d->_isReversed = reversed;
    update();
}

bool ShadMarquee::isReversed() const
{
    Q_D(const ShadMarquee);
    return d->_isReversed;
}

void ShadMarquee::setPauseOnHover(bool pause)
{
    Q_D(ShadMarquee);
    if (d->_isPauseOnHover == pause)
    {
        return;
    }
    d->_isPauseOnHover = pause;
    d->updateAnimation();
}

bool ShadMarquee::isPauseOnHover() const
{
    Q_D(const ShadMarquee);
    return d->_isPauseOnHover;
}

void ShadMarquee::setOverflowOnly(bool overflowOnly)
{
    Q_D(ShadMarquee);
    if (d->_isOverflowOnly == overflowOnly)
    {
        return;
    }
    d->_isOverflowOnly = overflowOnly;
    d->updateAnimation();
    update();
}

bool ShadMarquee::isOverflowOnly() const
{
    Q_D(const ShadMarquee);
    return d->_isOverflowOnly;
}

void ShadMarquee::setFadeWidth(int width)
{
    Q_D(ShadMarquee);
    const int value = qMax(0, width);
    if (d->_fadeWidth == value)
    {
        return;
    }
    d->_fadeWidth = value;
    update();
}

int ShadMarquee::getFadeWidth() const
{
    Q_D(const ShadMarquee);
    return d->_fadeWidth;
}

void ShadMarquee::setColorRole(ShadThemeType::ThemeColor colorRole)
{
    Q_D(ShadMarquee);
    d->_colorRole = colorRole;
    update();
}

ShadThemeType::ThemeColor ShadMarquee::getColorRole() const
{
    Q_D(const ShadMarquee);
    return d->_colorRole;
}

void ShadMarquee::setRunning(bool running)
{
    Q_D(ShadMarquee);
    if (d->_isRunning == running)
    {
        return;
    }
    d->_isRunning = running;
    d->updateAnimation();
    update();
}

bool ShadMarquee::isRunning() const
{
    Q_D(const ShadMarquee);
    return d->_isRunning;
}

QSize ShadMarquee::sizeHint() const
{
    return QSize(kDefaultWidth, minimumSizeHint().height());
}

QSize ShadMarquee::minimumSizeHint() const
{
    return QSize(kMinimumWidth, qMax(kMinimumHeight, qCeil(QFontMetricsF(font()).height())));
}

void ShadMarquee::changeEvent(QEvent* event)
{
    Q_D(ShadMarquee);
    if (event->type() == QEvent::FontChange)
    {
        d->updateTextWidth();
        updateGeometry();
    }
    else if (event->type() == QEvent::EnabledChange || event->type() == QEvent::LayoutDirectionChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadMarquee::enterEvent(QEnterEvent* event)
{
    Q_D(ShadMarquee);
    QWidget::enterEvent(event);
    d->_isHovered = true;
    d->updateAnimation();
}

void ShadMarquee::leaveEvent(QEvent* event)
{
    Q_D(ShadMarquee);
    QWidget::leaveEvent(event);
    d->_isHovered = false;
    d->updateAnimation();
}

void ShadMarquee::showEvent(QShowEvent* event)
{
    Q_D(ShadMarquee);
    QWidget::showEvent(event);
    d->updateAnimation();
}

void ShadMarquee::hideEvent(QHideEvent* event)
{
    Q_D(ShadMarquee);
    QWidget::hideEvent(event);
    d->updateAnimation();
}

void ShadMarquee::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadMarquee);
    QWidget::resizeEvent(event);
    // overflowOnly 时宽度变化影响是否滚动
    d->updateAnimation();
}

void ShadMarquee::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadMarquee);
    if (d->_text.isEmpty() || width() <= 0 || height() <= 0)
    {
        return;
    }
    const QFontMetricsF metrics(font());
    const qreal baseline = (height() - metrics.height()) / 2.0 + metrics.ascent();
    const qreal ratio = devicePixelRatioF();
    const QColor color = shadTheme->getColor(d->_colorRole);
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    QPainter painter(this);
    painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
    if (!d->isScrollable())
    {
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setFont(font());
        painter.setPen(color);
        painter.drawText(QPointF(isRightToLeft ? width() - d->_textWidth : 0.0, baseline), d->_text);
        return;
    }

    const qreal period = d->_textWidth + d->_gap;
    const bool isLeftward = isRightToLeft == d->_isReversed;
    const qreal firstLeft = isLeftward ? -d->_offset : d->_offset - period;
    const auto drawContent = [this, d, color, baseline, ratio, period, firstLeft](QPainter* target)
    {
        target->setRenderHint(QPainter::TextAntialiasing, true);
        target->setFont(font());
        target->setPen(color);
        for (qreal left = firstLeft; left < width(); left += period)
        {
            // 对齐设备像素，避免滚动时抖动
            target->drawText(QPointF(std::round(left * ratio) / ratio, baseline), d->_text);
        }
    };
    if (d->_fadeWidth <= 0)
    {
        drawContent(&painter);
        return;
    }
    QImage image(size() * ratio, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(ratio);
    image.fill(Qt::transparent);
    QPainter imagePainter(&image);
    drawContent(&imagePainter);
    const qreal fadeStop = qMin(0.5, d->_fadeWidth / qreal(width()));
    QLinearGradient gradient(0.0, 0.0, width(), 0.0);
    gradient.setColorAt(0.0, Qt::transparent);
    gradient.setColorAt(fadeStop, Qt::black);
    gradient.setColorAt(1.0 - fadeStop, Qt::black);
    gradient.setColorAt(1.0, Qt::transparent);
    imagePainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    imagePainter.fillRect(rect(), gradient);
    imagePainter.end();
    painter.drawImage(QPointF(0.0, 0.0), image);
}
