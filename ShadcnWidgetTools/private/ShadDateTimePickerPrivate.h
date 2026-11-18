#ifndef SHADDATETIMEPICKERPRIVATE_H
#define SHADDATETIMEPICKERPRIVATE_H

#include <QDateTime>
#include <QString>

#include "ShadProperty.h"

class ShadDateTimePanel;
class ShadDateTimePicker;
class ShadPopover;
class ShadTransition;

class ShadDateTimePickerPrivate
{
    SHAD_D_CREATE(ShadDateTimePicker)

    public:
        ShadDateTimePickerPrivate();
        ~ShadDateTimePickerPrivate();

        QString effectiveFormat() const;
        QString formatDateTime(const QDateTime& dateTime) const;
        QString displayText() const;
        QDateTime normalized(const QDateTime& dateTime) const;

        ShadPopover* _popover{nullptr};
        ShadDateTimePanel* _panel{nullptr};
        ShadTransition* _hoverTransition{nullptr};
        QDateTime _dateTime;
        QString _placeholderText;
        QString _displayFormat;
        bool _isSecondsVisible{true};
        bool _isNowButtonVisible{true};
        bool _isCalendarIconVisible{false};
        bool _isChevronVisible{false};
        bool _isFocusVisible{false};
};

#endif // SHADDATETIMEPICKERPRIVATE_H
