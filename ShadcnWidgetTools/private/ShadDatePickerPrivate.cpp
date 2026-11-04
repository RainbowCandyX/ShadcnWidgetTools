#include "ShadDatePickerPrivate.h"

#include <QChar>

#include "ShadCalendar.h"
#include "ShadDatePicker.h"
#include "ShadPopover.h"

namespace
{
    // 去掉星期后两端残留的分隔符：空白、逗号、阿拉伯逗号、全角逗号
    bool isFormatSeparator(QChar character)
    {
        return character.isSpace() || character == QLatin1Char(',') || character == QChar(0x060C) || character == QChar(0xFF0C);
    }
} // namespace

ShadDatePickerPrivate::ShadDatePickerPrivate()
{
}

ShadDatePickerPrivate::~ShadDatePickerPrivate()
{
}

QString ShadDatePickerPrivate::defaultFormat(const QLocale& locale)
{
    QString format = locale.dateFormat(QLocale::LongFormat);
    format.remove(QStringLiteral("dddd"));
    format.remove(QStringLiteral("ddd"));
    while (!format.isEmpty() && isFormatSeparator(format.front()))
    {
        format.remove(0, 1);
    }
    while (!format.isEmpty() && isFormatSeparator(format.back()))
    {
        format.chop(1);
    }
    return format;
}

QString ShadDatePickerPrivate::formatDate(const QDate& date) const
{
    Q_Q(const ShadDatePicker);
    if (!date.isValid())
    {
        return QString();
    }
    const QLocale locale = q->locale();
    return locale.toString(date, _displayFormat.isEmpty() ? defaultFormat(locale) : _displayFormat);
}

QString ShadDatePickerPrivate::displayText() const
{
    if (_mode == ShadDatePickerType::SingleDate)
    {
        return formatDate(_date);
    }
    if (!_rangeStart.isValid())
    {
        return QString();
    }
    if (!_rangeEnd.isValid())
    {
        return formatDate(_rangeStart);
    }
    return QStringLiteral("%1 - %2").arg(formatDate(_rangeStart), formatDate(_rangeEnd));
}

void ShadDatePickerPrivate::onCalendarDateClicked()
{
    Q_Q(ShadDatePicker);
    if (_mode == ShadDatePickerType::SingleDate)
    {
        const QDate date = _calendar->getSelectedDate();
        if (_date != date)
        {
            _date = date;
            q->updateGeometry();
            q->update();
            Q_EMIT q->dateChanged(date);
        }
        if (_isCloseOnSelect)
        {
            q->hidePopup();
        }
        return;
    }
    _rangeStart = _calendar->getRangeStart();
    _rangeEnd = _calendar->getRangeEnd();
    q->updateGeometry();
    q->update();
    Q_EMIT q->rangeChanged(_rangeStart, _rangeEnd);
    if (_isCloseOnSelect && _rangeEnd.isValid())
    {
        q->hidePopup();
    }
}

void ShadDatePickerPrivate::syncCalendar()
{
    const ShadCalendarType::SelectionMode mode = _mode == ShadDatePickerType::DateRange ? ShadCalendarType::Range : ShadCalendarType::Single;
    _calendar->setSelectionMode(mode);
    if (_mode == ShadDatePickerType::SingleDate)
    {
        _calendar->setSelectedDate(_date);
    }
    else
    {
        _calendar->setSelectedRange(_rangeStart, _rangeEnd);
    }
}
