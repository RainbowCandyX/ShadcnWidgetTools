#include "ShadStatistic.h"

#include <QAbstractAnimation>
#include <QColor>
#include <QDateTime>
#include <QEasingCurve>
#include <QEvent>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtNumeric>

#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStatisticPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadStatistic::ShadStatistic(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadStatisticPrivate())
{
    Q_D(ShadStatistic);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    d->_valueAnimation = new QVariantAnimation(this);
    d->_valueAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_valueAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_displayValue = value.toDouble();
        update();
    });

    d->_countdownTimer = new QTimer(this);
    d->_countdownTimer->setSingleShot(true);
    d->_countdownTimer->setTimerType(Qt::PreciseTimer);
    connect(d->_countdownTimer, &QTimer::timeout, this, [d]() { d->tickCountdown(); });

    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadStatistic::ShadStatistic(const QString& title, QWidget* parent)
    : ShadStatistic(parent)
{
    setTitle(title);
}

ShadStatistic::~ShadStatistic()
{
}

void ShadStatistic::setTitle(const QString& title)
{
    Q_D(ShadStatistic);
    if (d->_title == title)
    {
        return;
    }
    d->_title = title;
    d->refresh();
}

QString ShadStatistic::getTitle() const
{
    Q_D(const ShadStatistic);
    return d->_title;
}

void ShadStatistic::setTitleIcon(ShadIconType::IconName icon)
{
    Q_D(ShadStatistic);
    if (d->_titleIcon == icon)
    {
        return;
    }
    d->_titleIcon = icon;
    d->refresh();
}

ShadIconType::IconName ShadStatistic::getTitleIcon() const
{
    Q_D(const ShadStatistic);
    return d->_titleIcon;
}

void ShadStatistic::setValue(double value)
{
    Q_D(ShadStatistic);
    if (qIsNaN(value))
    {
        return;
    }
    const bool wasCountdown = d->_isCountdownMode;
    if (wasCountdown)
    {
        d->stopCountdown();
        d->_isCountdownMode = false;
    }
    else if (d->_value == value)
    {
        return;
    }
    const bool isChanged = d->_value != value;
    d->_value = value;
    d->startValueAnimation(value);
    d->refresh();
    if (isChanged)
    {
        Q_EMIT valueChanged(value);
    }
}

double ShadStatistic::getValue() const
{
    Q_D(const ShadStatistic);
    return d->_value;
}

void ShadStatistic::setPrecision(int precision)
{
    Q_D(ShadStatistic);
    precision = qBound(0, precision, 15);
    if (d->_precision == precision)
    {
        return;
    }
    d->_precision = precision;
    d->refresh();
}

int ShadStatistic::getPrecision() const
{
    Q_D(const ShadStatistic);
    return d->_precision;
}

void ShadStatistic::setGroupSeparatorShown(bool isShown)
{
    Q_D(ShadStatistic);
    if (d->_isGroupSeparatorShown == isShown)
    {
        return;
    }
    d->_isGroupSeparatorShown = isShown;
    d->refresh();
}

bool ShadStatistic::isGroupSeparatorShown() const
{
    Q_D(const ShadStatistic);
    return d->_isGroupSeparatorShown;
}

void ShadStatistic::setValueFontSize(int pixelSize)
{
    Q_D(ShadStatistic);
    pixelSize = qMax(8, pixelSize);
    if (d->_valueFontSize == pixelSize)
    {
        return;
    }
    d->_valueFontSize = pixelSize;
    d->refresh();
}

int ShadStatistic::getValueFontSize() const
{
    Q_D(const ShadStatistic);
    return d->_valueFontSize;
}

void ShadStatistic::setPrefix(const QString& prefix)
{
    Q_D(ShadStatistic);
    if (d->_prefix == prefix)
    {
        return;
    }
    d->_prefix = prefix;
    d->refresh();
}

QString ShadStatistic::getPrefix() const
{
    Q_D(const ShadStatistic);
    return d->_prefix;
}

void ShadStatistic::setSuffix(const QString& suffix)
{
    Q_D(ShadStatistic);
    if (d->_suffix == suffix)
    {
        return;
    }
    d->_suffix = suffix;
    d->refresh();
}

QString ShadStatistic::getSuffix() const
{
    Q_D(const ShadStatistic);
    return d->_suffix;
}

void ShadStatistic::setPrefixIcon(ShadIconType::IconName icon)
{
    Q_D(ShadStatistic);
    if (d->_prefixIcon == icon)
    {
        return;
    }
    d->_prefixIcon = icon;
    d->refresh();
}

ShadIconType::IconName ShadStatistic::getPrefixIcon() const
{
    Q_D(const ShadStatistic);
    return d->_prefixIcon;
}

void ShadStatistic::setTrend(ShadStatisticType::Trend trend)
{
    Q_D(ShadStatistic);
    if (d->_trend == trend)
    {
        return;
    }
    d->_trend = trend;
    d->refresh();
}

ShadStatisticType::Trend ShadStatistic::getTrend() const
{
    Q_D(const ShadStatistic);
    return d->_trend;
}

void ShadStatistic::setAnimationDuration(int milliseconds)
{
    Q_D(ShadStatistic);
    d->_animationDuration = qMax(0, milliseconds);
    if (d->_animationDuration == 0 && d->_valueAnimation->state() == QAbstractAnimation::Running)
    {
        d->_valueAnimation->stop();
        d->_displayValue = d->_value;
        d->refresh();
    }
}

int ShadStatistic::getAnimationDuration() const
{
    Q_D(const ShadStatistic);
    return d->_animationDuration;
}

void ShadStatistic::setCountdown(const QDateTime& target, const QString& format)
{
    Q_D(ShadStatistic);
    d->_valueAnimation->stop();
    d->stopCountdown();
    d->_isCountdownMode = true;
    d->_countdownTarget = target;
    d->_countdownFormat = format;
    d->_isCountdownActive = true;
    d->tickCountdown();
}

bool ShadStatistic::isCountdownActive() const
{
    Q_D(const ShadStatistic);
    return d->_isCountdownActive;
}

QSize ShadStatistic::sizeHint() const
{
    Q_D(const ShadStatistic);
    return d->computeSize();
}

QSize ShadStatistic::minimumSizeHint() const
{
    Q_D(const ShadStatistic);
    return d->computeSize();
}

void ShadStatistic::changeEvent(QEvent* event)
{
    Q_D(ShadStatistic);
    switch (event->type())
    {
        case QEvent::FontChange:
        case QEvent::LocaleChange:
        case QEvent::LayoutDirectionChange:
        case QEvent::EnabledChange:
            d->refresh();
            break;
        default:
            break;
    }
    QWidget::changeEvent(event);
}

void ShadStatistic::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadStatistic);
    const ShadStatisticPrivate::RowLayout rows = d->computeRows(d->displayText());
    const QRectF bounds(rect());
    const Qt::LayoutDirection direction = layoutDirection();
    const QColor mutedColor = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor valueColor = d->valueColor();
    const QFont titleFont = d->titleFont();
    const QFont valueFont = d->valueFont();
    const QFont affixFont = d->affixFont();
    const Qt::Alignment textAlignment = Qt::AlignLeft | Qt::AlignTop | Qt::AlignAbsolute;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);

    if (d->_titleIcon != ShadIconType::None)
    {
        ShadIcon::paint(&painter, ShadStyleHelper::visualRect(direction, bounds, rows.titleIconRect), d->_titleIcon, mutedColor, ShadStatisticPrivate::kIconSize);
    }
    if (!d->_title.isEmpty())
    {
        QRectF titleRect = rows.titleTextRect;
        titleRect.setWidth(qMin(titleRect.width(), bounds.width() - titleRect.left()));
        if (titleRect.width() > 0.0)
        {
            painter.setFont(titleFont);
            painter.setPen(mutedColor);
            painter.drawText(ShadStyleHelper::visualRect(direction, bounds, titleRect), Qt::AlignLeft | Qt::AlignVCenter | Qt::AlignAbsolute, ShadStyleHelper::elideText(titleFont, d->_title, titleRect.width()));
        }
    }
    if (d->trendIcon() != ShadIconType::None)
    {
        ShadIcon::paint(&painter, ShadStyleHelper::visualRect(direction, bounds, rows.trendIconRect), d->trendIcon(), valueColor, d->trendIconSize());
    }
    if (d->_prefixIcon != ShadIconType::None)
    {
        ShadIcon::paint(&painter, ShadStyleHelper::visualRect(direction, bounds, rows.prefixIconRect), d->_prefixIcon, mutedColor, ShadStatisticPrivate::kIconSize);
    }
    painter.setFont(affixFont);
    painter.setPen(mutedColor);
    if (!d->_prefix.isEmpty())
    {
        painter.drawText(ShadStyleHelper::visualRect(direction, bounds, rows.prefixRect), textAlignment, d->_prefix);
    }
    if (!d->_suffix.isEmpty())
    {
        painter.drawText(ShadStyleHelper::visualRect(direction, bounds, rows.suffixRect), textAlignment, d->_suffix);
    }
    painter.setFont(valueFont);
    painter.setPen(valueColor);
    painter.drawText(ShadStyleHelper::visualRect(direction, bounds, rows.valueRect), textAlignment, d->displayText());
}
