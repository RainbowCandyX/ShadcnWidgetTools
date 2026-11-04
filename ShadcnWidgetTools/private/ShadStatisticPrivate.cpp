#include "ShadStatisticPrivate.h"

#include <QColor>
#include <QDateTime>
#include <QEasingCurve>
#include <QFont>
#include <QFontMetricsF>
#include <QLatin1Char>
#include <QLatin1String>
#include <QLocale>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtMath>

#include "ShadStatistic.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadStatisticPrivate::ShadStatisticPrivate()
{
}

ShadStatisticPrivate::~ShadStatisticPrivate()
{
}

QFont ShadStatisticPrivate::titleFont() const
{
    QFont result = q_ptr->font();
    result.setPixelSize(kTextSize);
    result.setWeight(QFont::Normal);
    return result;
}

QFont ShadStatisticPrivate::valueFont() const
{
    QFont result = q_ptr->font();
    result.setPixelSize(_valueFontSize);
    result.setWeight(QFont::DemiBold);
    return result;
}

QFont ShadStatisticPrivate::affixFont() const
{
    QFont result = q_ptr->font();
    result.setPixelSize(kTextSize);
    result.setWeight(QFont::Normal);
    return result;
}

QString ShadStatisticPrivate::formatValue(double value) const
{
    QLocale locale = q_ptr->locale();
    if (!_isGroupSeparatorShown)
    {
        locale.setNumberOptions(locale.numberOptions() | QLocale::OmitGroupSeparator);
    }
    return locale.toString(value, 'f', _precision);
}

QString ShadStatisticPrivate::displayText() const
{
    if (_isCountdownMode)
    {
        return _countdownText;
    }
    return formatValue(_displayValue);
}

QString ShadStatisticPrivate::countdownText(qint64 remainingSeconds) const
{
    const bool hasDays = _countdownFormat.contains(QLatin1Char('D'));
    const qint64 days = remainingSeconds / 86400;
    const qint64 hours = hasDays ? (remainingSeconds / 3600) % 24 : remainingSeconds / 3600;
    const qint64 minutes = (remainingSeconds / 60) % 60;
    const qint64 seconds = remainingSeconds % 60;
    QString result;
    int index = 0;
    while (index < _countdownFormat.size())
    {
        const QChar current = _countdownFormat.at(index);
        if (_countdownFormat.mid(index, 2) == QLatin1String("HH"))
        {
            result += QString::number(hours).rightJustified(2, QLatin1Char('0'));
            index += 2;
        }
        else if (_countdownFormat.mid(index, 2) == QLatin1String("mm"))
        {
            result += QString::number(minutes).rightJustified(2, QLatin1Char('0'));
            index += 2;
        }
        else if (_countdownFormat.mid(index, 2) == QLatin1String("ss"))
        {
            result += QString::number(seconds).rightJustified(2, QLatin1Char('0'));
            index += 2;
        }
        else if (current == QLatin1Char('D'))
        {
            result += QString::number(days);
            index += 1;
        }
        else
        {
            result += current;
            index += 1;
        }
    }
    return result;
}

QColor ShadStatisticPrivate::upColor() const
{
    return shadTheme->isDark() ? QColor(0x4a, 0xde, 0x80) : QColor(0x16, 0xa3, 0x4a);
}

QColor ShadStatisticPrivate::valueColor() const
{
    switch (_trend)
    {
        case ShadStatisticType::Up:
            return upColor();
        case ShadStatisticType::Down:
            return shadTheme->getColor(ShadThemeType::Destructive);
        case ShadStatisticType::None:
            break;
    }
    return shadTheme->getColor(ShadThemeType::Foreground);
}

ShadIconType::IconName ShadStatisticPrivate::trendIcon() const
{
    switch (_trend)
    {
        case ShadStatisticType::Up:
            return ShadIconType::ArrowUp;
        case ShadStatisticType::Down:
            return ShadIconType::ArrowDown;
        case ShadStatisticType::None:
            break;
    }
    return ShadIconType::None;
}

int ShadStatisticPrivate::trendIconSize() const
{
    return qMax(kIconSize, qRound(_valueFontSize * 0.75));
}

ShadStatisticPrivate::RowLayout ShadStatisticPrivate::computeRows(const QString& text) const
{
    RowLayout layout;
    const QFont titleTextFont = titleFont();
    const QFont valueTextFont = valueFont();
    const QFont affixTextFont = affixFont();
    const QFontMetricsF valueMetrics(valueTextFont);
    const QFontMetricsF affixMetrics(affixTextFont);
    const bool hasTitle = !_title.isEmpty() || _titleIcon != ShadIconType::None;

    layout.titleHeight = hasTitle ? qMax<qreal>(kTitleLineHeight, QFontMetricsF(titleTextFont).height()) : 0.0;
    qreal width = 0.0;
    if (hasTitle)
    {
        qreal x = 0.0;
        if (_titleIcon != ShadIconType::None)
        {
            layout.titleIconRect = QRectF(0.0, (layout.titleHeight - kIconSize) / 2.0, kIconSize, kIconSize);
            x = kIconSize;
            if (!_title.isEmpty())
            {
                x += kTitleIconGap;
            }
        }
        const qreal titleWidth = _title.isEmpty() ? 0.0 : ShadStyleHelper::textWidth(titleTextFont, _title);
        layout.titleTextRect = QRectF(x, 0.0, titleWidth, layout.titleHeight);
        width = x + titleWidth;
    }

    const qreal rowTop = hasTitle ? layout.titleHeight + kRowGap : 0.0;
    layout.valueHeight = valueMetrics.height();
    layout.baseline = rowTop + valueMetrics.ascent();
    qreal x = 0.0;
    if (trendIcon() != ShadIconType::None)
    {
        const int iconSize = trendIconSize();
        layout.trendIconRect = QRectF(x, rowTop + (layout.valueHeight - iconSize) / 2.0, iconSize, iconSize);
        x += iconSize + kSegmentGap;
    }
    if (_prefixIcon != ShadIconType::None)
    {
        layout.prefixIconRect = QRectF(x, rowTop + (layout.valueHeight - kIconSize) / 2.0, kIconSize, kIconSize);
        x += kIconSize + kSegmentGap;
    }
    if (!_prefix.isEmpty())
    {
        const qreal prefixWidth = ShadStyleHelper::textWidth(affixTextFont, _prefix);
        layout.prefixRect = QRectF(x, layout.baseline - affixMetrics.ascent(), prefixWidth, affixMetrics.height());
        x += prefixWidth + kSegmentGap;
    }
    const qreal valueWidth = ShadStyleHelper::textWidth(valueTextFont, text);
    layout.valueRect = QRectF(x, rowTop, valueWidth, layout.valueHeight);
    x += valueWidth;
    if (!_suffix.isEmpty())
    {
        x += kSegmentGap;
        const qreal suffixWidth = ShadStyleHelper::textWidth(affixTextFont, _suffix);
        layout.suffixRect = QRectF(x, layout.baseline - affixMetrics.ascent(), suffixWidth, affixMetrics.height());
        x += suffixWidth;
    }
    layout.width = qMax(width, x);
    layout.height = rowTop + layout.valueHeight;
    return layout;
}

QSize ShadStatisticPrivate::computeSize() const
{
    const QString text = _isCountdownMode ? _countdownText : formatValue(_value);
    const RowLayout layout = computeRows(text);
    return QSize(qCeil(layout.width), qCeil(layout.height));
}

void ShadStatisticPrivate::refresh()
{
    q_ptr->updateGeometry();
    q_ptr->update();
}

void ShadStatisticPrivate::startValueAnimation(double target)
{
    _valueAnimation->stop();
    if (_animationDuration <= 0 || _displayValue == target)
    {
        _displayValue = target;
        q_ptr->update();
        return;
    }
    _valueAnimation->setDuration(_animationDuration);
    _valueAnimation->setStartValue(_displayValue);
    _valueAnimation->setEndValue(target);
    _valueAnimation->start();
}

void ShadStatisticPrivate::stopCountdown()
{
    _countdownTimer->stop();
    _isCountdownActive = false;
}

void ShadStatisticPrivate::tickCountdown()
{
    if (!_isCountdownActive)
    {
        return;
    }
    const qint64 remainingMs = _countdownTarget.toMSecsSinceEpoch() - QDateTime::currentMSecsSinceEpoch();
    if (remainingMs <= 0)
    {
        stopCountdown();
        _countdownText = countdownText(0);
        refresh();
        Q_EMIT q_ptr->finished();
        return;
    }
    _countdownText = countdownText((remainingMs + 999) / 1000);
    refresh();
    scheduleCountdown();
}

void ShadStatisticPrivate::scheduleCountdown()
{
    const qint64 remainingMs = _countdownTarget.toMSecsSinceEpoch() - QDateTime::currentMSecsSinceEpoch();
    qint64 interval = remainingMs % 1000;
    if (interval <= 0)
    {
        interval = 1000;
    }
    _countdownTimer->start(static_cast<int>(interval));
}
