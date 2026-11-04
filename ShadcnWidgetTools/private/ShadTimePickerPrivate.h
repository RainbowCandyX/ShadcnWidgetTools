#ifndef SHADTIMEPICKERPRIVATE_H
#define SHADTIMEPICKERPRIVATE_H

#include <QString>
#include <QTime>

#include "ShadProperty.h"

class ShadPopover;
class ShadTimePanel;
class ShadTimePicker;
class ShadTransition;

class ShadTimePickerPrivate
{
    SHAD_D_CREATE(ShadTimePicker)

    public:
        ShadTimePickerPrivate();
        ~ShadTimePickerPrivate();

        QString effectiveFormat() const;
        QString displayText() const;

        ShadPopover* _popover{nullptr};
        ShadTimePanel* _panel{nullptr};
        ShadTransition* _hoverTransition{nullptr};
        QTime _time;
        QString _placeholderText;
        QString _displayFormat;
        bool _isSecondsVisible{true};
        bool _isClockIconVisible{false};
        bool _isChevronVisible{false};
        bool _isFocusVisible{false};
};

#endif // SHADTIMEPICKERPRIVATE_H
