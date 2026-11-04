#include "ShadTimePickerPrivate.h"

#include "ShadTimePicker.h"

ShadTimePickerPrivate::ShadTimePickerPrivate()
{
}

ShadTimePickerPrivate::~ShadTimePickerPrivate()
{
}

QString ShadTimePickerPrivate::effectiveFormat() const
{
    if (!_displayFormat.isEmpty())
    {
        return _displayFormat;
    }
    return _isSecondsVisible ? QStringLiteral("HH:mm:ss") : QStringLiteral("HH:mm");
}

QString ShadTimePickerPrivate::displayText() const
{
    return _time.isValid() ? _time.toString(effectiveFormat()) : QString();
}
