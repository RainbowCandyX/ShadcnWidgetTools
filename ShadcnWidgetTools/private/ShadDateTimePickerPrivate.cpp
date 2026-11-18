#include "ShadDateTimePickerPrivate.h"

#include <QDate>
#include <QLocale>
#include <QTime>

#include "ShadDatePickerPrivate.h"
#include "ShadDateTimePicker.h"

ShadDateTimePickerPrivate::ShadDateTimePickerPrivate()
{
}

ShadDateTimePickerPrivate::~ShadDateTimePickerPrivate()
{
}

QString ShadDateTimePickerPrivate::effectiveFormat() const
{
    Q_Q(const ShadDateTimePicker);
    if (!_displayFormat.isEmpty())
    {
        return _displayFormat;
    }
    return ShadDatePickerPrivate::defaultFormat(q->locale()) + (_isSecondsVisible ? QStringLiteral(" HH:mm:ss") : QStringLiteral(" HH:mm"));
}

QString ShadDateTimePickerPrivate::formatDateTime(const QDateTime& dateTime) const
{
    Q_Q(const ShadDateTimePicker);
    return dateTime.isValid() ? q->locale().toString(dateTime, effectiveFormat()) : QString();
}

QString ShadDateTimePickerPrivate::displayText() const
{
    return formatDateTime(_dateTime);
}

QDateTime ShadDateTimePickerPrivate::normalized(const QDateTime& dateTime) const
{
    if (!dateTime.isValid())
    {
        return QDateTime();
    }
    const QDateTime local = dateTime.toLocalTime();
    const QTime time = local.time();
    return QDateTime(local.date(), _isSecondsVisible ? QTime(time.hour(), time.minute(), time.second()) : QTime(time.hour(), time.minute()));
}
