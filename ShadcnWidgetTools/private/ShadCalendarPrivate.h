#ifndef SHADCALENDARPRIVATE_H
#define SHADCALENDARPRIVATE_H

#include <QDate>
#include <QList>
#include <QLocale>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadProperty.h"

class ShadButton;
class ShadCalendar;
class ShadComboBox;

class ShadCalendarPrivate
{
    SHAD_D_CREATE(ShadCalendar)

    public:
        ShadCalendarPrivate();
        ~ShadCalendarPrivate();

        // p-3，--cell-size: 32px，月份之间 gap-4，标题与网格 gap-4，周与周之间 mt-2
        static constexpr int kPadding = 12;
        static constexpr int kCellSize = 32;
        static constexpr int kMonthGap = 16;
        static constexpr int kCaptionHeight = 32;
        static constexpr int kCaptionGap = 16;
        static constexpr int kWeekdayHeight = 20;
        static constexpr int kWeekGap = 8;
        static constexpr int kDaysPerWeek = 7;
        // 下拉标题中月份与年份之间 gap-1.5
        static constexpr int kDropdownGap = 6;
        // 星期名 text-[0.8rem]，日期与标题 text-sm
        static constexpr int kWeekdayFontSize = 13;
        static constexpr int kTextFontSize = 14;

        // 第 index 个显示的月份（该月 1 日）
        QDate monthAt(int index) const;
        // 某月网格第一格的日期（该周的第一天，可能属于上个月）
        QDate gridStart(const QDate& month) const;
        // 某月需要的周数（4 到 6）
        int weekCount(const QDate& month) const;
        int maximumWeekCount() const;
        int monthWidth() const;

        // 以下矩形均为 LTR 逻辑坐标，绘制与命中测试时按布局方向镜像
        int monthLeft(int index) const;
        int weekdayTop() const;
        int weekTop(int week) const;
        QRect cellRect(int monthIndex, int week, int column) const;
        QRect visualRect(const QRect& logicalRect) const;

        // 鼠标位置下的日期，monthIndex 返回所在的月份序号；不在任何日期格上时返回无效日期
        QDate dateAt(const QPoint& pos, int* monthIndex) const;
        // 日期在显示的月份中作为本月日期时的位置，找不到时返回 false
        bool locateDate(const QDate& date, int* monthIndex, int* week, int* column) const;
        bool isVisible(const QDate& date) const;
        bool isSelectable(const QDate& date) const;
        // 第 column 列对应的星期
        int dayOfWeekAt(int column) const;

        // 用户选择日期：按模式更新选择，发出 selectionChanged 与 dateClicked
        void selectDate(const QDate& date);
        // 日期不在显示的月份中时切换月份使其可见
        void ensureVisible(const QDate& date);
        // 键盘焦点移到 date（限制在可选范围内）并保证可见
        void moveFocus(const QDate& date);
        // 获得焦点时的初始焦点日期：可见的选中日期、可见的今天，否则为当前月第一个可选日期
        QDate initialFocusDate() const;
        QDate clampDate(const QDate& date) const;
        QDate firstNavigableMonth() const;
        QDate lastNavigableMonth() const;

        bool isDropdownActive() const;
        // 重新填充月份与年份下拉框（locale 或日期范围变化时）
        void rebuildDropdowns();
        // 下拉框选中项与当前月份同步，不触发月份切换
        void syncDropdowns();
        // 上一月 / 下一月按钮的启用状态与箭头方向
        void updateNavigation();
        // 放置导航按钮与下拉框
        void layoutChildren();
        // 尺寸、子组件与重绘一起更新
        void refresh();

        QLocale numberLocale() const;
        QString captionText(const QDate& month) const;
        QString weekdayText(int dayOfWeek) const;
        QString dayText(const QDate& date) const;

        ShadButton* _previousButton{nullptr};
        ShadButton* _nextButton{nullptr};
        ShadComboBox* _monthCombo{nullptr};
        ShadComboBox* _yearCombo{nullptr};
        ShadCalendarType::SelectionMode _mode{ShadCalendarType::Single};
        ShadCalendarType::CaptionLayout _captionLayout{ShadCalendarType::Label};
        QDate _selectedDate;
        QList<QDate> _selectedDates;
        QDate _rangeStart;
        QDate _rangeEnd;
        QDate _currentMonth;
        int _monthCount{1};
        QDate _minimumDate{1900, 1, 1};
        QDate _maximumDate{2100, 12, 31};
        Qt::DayOfWeek _firstDayOfWeek{Qt::Monday};
        bool _isFirstDayExplicit{false};
        QDate _focusDate;
        QDate _hoverDate;
        bool _isFocusVisible{false};
        bool _isSyncingDropdowns{false};
};

#endif // SHADCALENDARPRIVATE_H
