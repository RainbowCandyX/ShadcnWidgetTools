#ifndef SHADDATEPICKERPRIVATE_H
#define SHADDATEPICKERPRIVATE_H

#include <QDate>
#include <QLocale>
#include <QString>

#include "ShadDef.h"
#include "ShadProperty.h"

class ShadCalendar;
class ShadDatePicker;
class ShadPopover;
class ShadTransition;

class ShadDatePickerPrivate
{
    SHAD_D_CREATE(ShadDatePicker)

    public:
        ShadDatePickerPrivate();
        ~ShadDatePickerPrivate();

        // 计算理想宽度时使用的示例日期，取月名与日期较长的一天
        static constexpr int kSampleYear = 2025;
        static constexpr int kSampleMonth = 9;
        static constexpr int kSampleDay = 28;

        // locale 的长日期格式去掉星期，例如 "MMMM d, yyyy"、"yyyy年M月d日"
        static QString defaultFormat(const QLocale& locale);
        QString formatDate(const QDate& date) const;
        QString displayText() const;
        // 日历按钮点击日期后读取日历的选择并按设置关闭浮层
        void onCalendarDateClicked();
        // 把当前日期同步到日历，不触发日历的用户选择信号
        void syncCalendar();

        ShadPopover* _popover{nullptr};
        ShadCalendar* _calendar{nullptr};
        ShadTransition* _hoverTransition{nullptr};
        ShadDatePickerType::Mode _mode{ShadDatePickerType::SingleDate};
        QDate _date;
        QDate _rangeStart;
        QDate _rangeEnd;
        QString _placeholderText;
        QString _displayFormat;
        bool _isCalendarIconVisible{false};
        bool _isChevronVisible{false};
        bool _isCloseOnSelect{true};
        bool _isFocusVisible{false};
};

#endif // SHADDATEPICKERPRIVATE_H
