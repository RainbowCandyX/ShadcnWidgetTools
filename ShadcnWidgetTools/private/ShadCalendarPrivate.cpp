#include "ShadCalendarPrivate.h"

#include <QSignalBlocker>
#include <QSize>
#include <QVariant>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadCalendar.h"
#include "ShadComboBox.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"

ShadCalendarPrivate::ShadCalendarPrivate()
{
}

ShadCalendarPrivate::~ShadCalendarPrivate()
{
}

QDate ShadCalendarPrivate::monthAt(int index) const
{
    return _currentMonth.addMonths(index);
}

QDate ShadCalendarPrivate::gridStart(const QDate& month) const
{
    const int offset = (month.dayOfWeek() - static_cast<int>(_firstDayOfWeek) + kDaysPerWeek) % kDaysPerWeek;
    return month.addDays(-offset);
}

int ShadCalendarPrivate::weekCount(const QDate& month) const
{
    const int offset = static_cast<int>(gridStart(month).daysTo(month));
    return (offset + month.daysInMonth() + kDaysPerWeek - 1) / kDaysPerWeek;
}

int ShadCalendarPrivate::maximumWeekCount() const
{
    int count = 0;
    for (int index = 0; index < _monthCount; ++index)
    {
        count = qMax(count, weekCount(monthAt(index)));
    }
    return count;
}

int ShadCalendarPrivate::monthWidth() const
{
    return kDaysPerWeek * kCellSize;
}

int ShadCalendarPrivate::monthLeft(int index) const
{
    return kPadding + index * (monthWidth() + kMonthGap);
}

int ShadCalendarPrivate::weekdayTop() const
{
    return kPadding + kCaptionHeight + kCaptionGap;
}

int ShadCalendarPrivate::weekTop(int week) const
{
    return weekdayTop() + kWeekdayHeight + kWeekGap + week * (kCellSize + kWeekGap);
}

QRect ShadCalendarPrivate::cellRect(int monthIndex, int week, int column) const
{
    return QRect(monthLeft(monthIndex) + column * kCellSize, weekTop(week), kCellSize, kCellSize);
}

QRect ShadCalendarPrivate::visualRect(const QRect& logicalRect) const
{
    Q_Q(const ShadCalendar);
    return ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), logicalRect);
}

QDate ShadCalendarPrivate::dateAt(const QPoint& pos, int* monthIndex) const
{
    Q_Q(const ShadCalendar);
    // 命中测试在逻辑坐标中进行，RTL 时先把横坐标镜像回去
    const int x = q->layoutDirection() == Qt::RightToLeft ? q->width() - 1 - pos.x() : pos.x();
    const int y = pos.y();
    for (int index = 0; index < _monthCount; ++index)
    {
        const int left = monthLeft(index);
        if (x < left || x >= left + monthWidth())
        {
            continue;
        }
        const QDate month = monthAt(index);
        for (int week = 0; week < weekCount(month); ++week)
        {
            const int top = weekTop(week);
            if (y < top || y >= top + kCellSize)
            {
                continue;
            }
            const int column = (x - left) / kCellSize;
            if (monthIndex != nullptr)
            {
                *monthIndex = index;
            }
            return gridStart(month).addDays(week * kDaysPerWeek + column);
        }
    }
    return QDate();
}

bool ShadCalendarPrivate::locateDate(const QDate& date, int* monthIndex, int* week, int* column) const
{
    if (!date.isValid())
    {
        return false;
    }
    for (int index = 0; index < _monthCount; ++index)
    {
        const QDate month = monthAt(index);
        if (date.year() != month.year() || date.month() != month.month())
        {
            continue;
        }
        const int offset = static_cast<int>(gridStart(month).daysTo(date));
        *monthIndex = index;
        *week = offset / kDaysPerWeek;
        *column = offset % kDaysPerWeek;
        return true;
    }
    return false;
}

bool ShadCalendarPrivate::isVisible(const QDate& date) const
{
    if (!date.isValid())
    {
        return false;
    }
    const QDate first = monthAt(0);
    const QDate last = monthAt(_monthCount - 1);
    return date >= first && date <= last.addDays(last.daysInMonth() - 1);
}

bool ShadCalendarPrivate::isSelectable(const QDate& date) const
{
    return date.isValid() && date >= _minimumDate && date <= _maximumDate;
}

int ShadCalendarPrivate::dayOfWeekAt(int column) const
{
    return (static_cast<int>(_firstDayOfWeek) - 1 + column) % kDaysPerWeek + 1;
}

void ShadCalendarPrivate::selectDate(const QDate& date)
{
    Q_Q(ShadCalendar);
    switch (_mode)
    {
        case ShadCalendarType::Single:
            {
                _selectedDate = date;
                break;
            }
        case ShadCalendarType::Multiple:
            {
                // 点击切换选中状态，列表保持升序
                if (_selectedDates.contains(date))
                {
                    _selectedDates.removeAll(date);
                }
                else
                {
                    int position = 0;
                    while (position < _selectedDates.size() && _selectedDates.at(position) < date)
                    {
                        ++position;
                    }
                    _selectedDates.insert(position, date);
                }
                break;
            }
        case ShadCalendarType::Range:
            {
                // 没有起点或范围已完整时重新开始；早于起点时作为新的起点，否则作为终点
                if (!_rangeStart.isValid() || _rangeEnd.isValid())
                {
                    _rangeStart = date;
                    _rangeEnd = QDate();
                }
                else if (date < _rangeStart)
                {
                    _rangeStart = date;
                }
                else
                {
                    _rangeEnd = date;
                }
                break;
            }
    }
    q->update();
    Q_EMIT q->selectionChanged();
    Q_EMIT q->dateClicked(date);
}

void ShadCalendarPrivate::ensureVisible(const QDate& date)
{
    Q_Q(ShadCalendar);
    if (!date.isValid() || isVisible(date))
    {
        return;
    }
    const QDate month(date.year(), date.month(), 1);
    if (month < monthAt(0))
    {
        q->setCurrentMonth(month);
    }
    else
    {
        q->setCurrentMonth(month.addMonths(-(_monthCount - 1)));
    }
}

void ShadCalendarPrivate::moveFocus(const QDate& date)
{
    Q_Q(ShadCalendar);
    _focusDate = clampDate(date);
    ensureVisible(_focusDate);
    q->update();
}

QDate ShadCalendarPrivate::initialFocusDate() const
{
    QDate selected;
    switch (_mode)
    {
        case ShadCalendarType::Single:
            selected = _selectedDate;
            break;
        case ShadCalendarType::Multiple:
            selected = _selectedDates.isEmpty() ? QDate() : _selectedDates.first();
            break;
        case ShadCalendarType::Range:
            selected = _rangeStart;
            break;
    }
    if (isVisible(selected))
    {
        return selected;
    }
    const QDate today = QDate::currentDate();
    if (isVisible(today) && isSelectable(today))
    {
        return today;
    }
    return clampDate(monthAt(0));
}

QDate ShadCalendarPrivate::clampDate(const QDate& date) const
{
    if (date < _minimumDate)
    {
        return _minimumDate;
    }
    if (date > _maximumDate)
    {
        return _maximumDate;
    }
    return date;
}

QDate ShadCalendarPrivate::firstNavigableMonth() const
{
    return QDate(_minimumDate.year(), _minimumDate.month(), 1);
}

QDate ShadCalendarPrivate::lastNavigableMonth() const
{
    // 并排显示多个月时，最后一个月不能超过最大日期所在的月份
    const QDate lastMonth = QDate(_maximumDate.year(), _maximumDate.month(), 1).addMonths(-(_monthCount - 1));
    return qMax(lastMonth, firstNavigableMonth());
}

bool ShadCalendarPrivate::isDropdownActive() const
{
    return _captionLayout == ShadCalendarType::Dropdown && _monthCount == 1;
}

void ShadCalendarPrivate::rebuildDropdowns()
{
    Q_Q(ShadCalendar);
    const QSignalBlocker monthBlocker(_monthCombo);
    const QSignalBlocker yearBlocker(_yearCombo);
    const QLocale locale = q->locale();
    const QLocale digits = numberLocale();
    _monthCombo->clear();
    for (int month = 1; month <= 12; ++month)
    {
        // 对应 formatMonthDropdown 的 month: "short"
        _monthCombo->addItem(locale.standaloneMonthName(month, QLocale::ShortFormat), month);
    }
    _yearCombo->clear();
    for (int year = _minimumDate.year(); year <= _maximumDate.year(); ++year)
    {
        _yearCombo->addItem(digits.toString(year), year);
    }
    syncDropdowns();
}

void ShadCalendarPrivate::syncDropdowns()
{
    const QSignalBlocker monthBlocker(_monthCombo);
    const QSignalBlocker yearBlocker(_yearCombo);
    _monthCombo->setCurrentIndex(_currentMonth.month() - 1);
    _yearCombo->setCurrentIndex(_currentMonth.year() - _minimumDate.year());
}

void ShadCalendarPrivate::updateNavigation()
{
    Q_Q(ShadCalendar);
    ShadStyleHelper::setNavigationButtonsEnabled(_previousButton, _currentMonth > firstNavigableMonth(), _nextButton, _currentMonth < lastNavigableMonth());
    // RTL 时上一月按钮位于右侧，箭头朝右（对应 rtl 下的 rotate-180）
    const bool isRightToLeft = q->layoutDirection() == Qt::RightToLeft;
    _previousButton->setShadIcon(isRightToLeft ? ShadIconType::ChevronRight : ShadIconType::ChevronLeft);
    _nextButton->setShadIcon(isRightToLeft ? ShadIconType::ChevronLeft : ShadIconType::ChevronRight);
}

void ShadCalendarPrivate::layoutChildren()
{
    Q_Q(ShadCalendar);
    const int focusMargin = ShadStyleHelper::kFocusRingWidth;
    // 导航按钮位于整个月份区域的两端（absolute inset-x-0 top-0），按可视边缘对齐
    const QSize buttonSize = _previousButton->sizeHint();
    const QRect previousRect(kPadding - focusMargin, kPadding - focusMargin, buttonSize.width(), buttonSize.height());
    const QRect nextRect(q->width() - kPadding - kCellSize - focusMargin, kPadding - focusMargin, buttonSize.width(), buttonSize.height());
    _previousButton->setGeometry(visualRect(previousRect));
    _nextButton->setGeometry(visualRect(nextRect));

    const bool isDropdown = isDropdownActive();
    _monthCombo->setVisible(isDropdown);
    _yearCombo->setVisible(isDropdown);
    if (!isDropdown)
    {
        return;
    }
    // 月份与年份下拉框在标题行居中，扣除两者的焦点环边距后间距为 gap-1.5
    const QSize monthSize = _monthCombo->sizeHint();
    const QSize yearSize = _yearCombo->sizeHint();
    const int visualWidth = monthSize.width() + yearSize.width() - 4 * focusMargin + kDropdownGap;
    const int left = monthLeft(0) + (monthWidth() - visualWidth) / 2 - focusMargin;
    const QRect monthRect(left, kPadding + (kCaptionHeight - monthSize.height()) / 2, monthSize.width(), monthSize.height());
    const QRect yearRect(left + monthSize.width() - 2 * focusMargin + kDropdownGap, kPadding + (kCaptionHeight - yearSize.height()) / 2, yearSize.width(), yearSize.height());
    _monthCombo->setGeometry(visualRect(monthRect));
    _yearCombo->setGeometry(visualRect(yearRect));
}

void ShadCalendarPrivate::refresh()
{
    Q_Q(ShadCalendar);
    updateNavigation();
    q->updateGeometry();
    layoutChildren();
    q->update();
}

QLocale ShadCalendarPrivate::numberLocale() const
{
    Q_Q(const ShadCalendar);
    // 年份与日期不显示千位分隔符，数字仍使用 locale 的数字字符
    QLocale locale = q->locale();
    locale.setNumberOptions(locale.numberOptions() | QLocale::OmitGroupSeparator);
    return locale;
}

QString ShadCalendarPrivate::captionText(const QDate& month) const
{
    Q_Q(const ShadCalendar);
    const QLocale locale = q->locale();
    switch (locale.language())
    {
        case QLocale::Chinese:
        case QLocale::Japanese:
            return locale.toString(month, QStringLiteral("yyyy年M月"));
        case QLocale::Korean:
            return locale.toString(month, QStringLiteral("yyyy년 M월"));
        default:
            return locale.toString(month, QStringLiteral("MMMM yyyy"));
    }
}

QString ShadCalendarPrivate::weekdayText(int dayOfWeek) const
{
    Q_Q(const ShadCalendar);
    const QLocale locale = q->locale();
    // 中文与日文用单字（日 一 二），其他语言用缩写（Sun Mon）
    const bool isCjk = locale.language() == QLocale::Chinese || locale.language() == QLocale::Japanese;
    return locale.standaloneDayName(dayOfWeek, isCjk ? QLocale::NarrowFormat : QLocale::ShortFormat);
}

QString ShadCalendarPrivate::dayText(const QDate& date) const
{
    return numberLocale().toString(date.day());
}
