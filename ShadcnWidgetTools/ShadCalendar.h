#ifndef SHADCALENDAR_H
#define SHADCALENDAR_H

#include <QDate>
#include <QList>
#include <QSize>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;

// 日历，对应 shadcn Calendar（react-day-picker）：p-3，每格 32×32（--cell-size），圆角 rounded-md
// 选择模式：单选（选中为 bg-primary）、多选（点击切换）、范围（起止为 bg-primary，中间为 bg-muted，每行连成一条）
// 今天为 bg-muted；不属于当月的日期显示为 muted 色，点击后选中并切换到该月；超出最小 / 最大日期的日期为 opacity-50 且不可选择
// 标题为年月文字，左右两端为上一月 / 下一月按钮；Dropdown 标题用下拉框切换月份与年份（只在显示单个月时生效）
// 月名、星期名与日期数字使用组件的 locale()，每周第一天默认跟随 locale
// 键盘：方向键移动一天 / 一周，PageUp / PageDown 切换月份（加 Shift 切换年份），Home / End 移到本周第一天 / 最后一天，回车或空格选择
// RTL 下网格从右向左排列，左右方向键对调，上一月 / 下一月按钮的箭头水平翻转
// 背景透明，放在卡片或浮层中时与容器背景一致
class ShadCalendarPrivate;

class SHADCN_EXPORT ShadCalendar : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadCalendar)
    Q_PROPERTY(ShadCalendarType::SelectionMode selectionMode READ getSelectionMode WRITE setSelectionMode)
    Q_PROPERTY(ShadCalendarType::CaptionLayout captionLayout READ getCaptionLayout WRITE setCaptionLayout)
    Q_PROPERTY(QDate selectedDate READ getSelectedDate WRITE setSelectedDate NOTIFY selectionChanged)
    Q_PROPERTY(QDate currentMonth READ getCurrentMonth WRITE setCurrentMonth NOTIFY currentMonthChanged)
    Q_PROPERTY(int monthCount READ getMonthCount WRITE setMonthCount)
    Q_PROPERTY(QDate minimumDate READ getMinimumDate WRITE setMinimumDate)
    Q_PROPERTY(QDate maximumDate READ getMaximumDate WRITE setMaximumDate)
    Q_PROPERTY(Qt::DayOfWeek firstDayOfWeek READ getFirstDayOfWeek WRITE setFirstDayOfWeek)

    public:
        explicit ShadCalendar(QWidget* parent = nullptr);
        ~ShadCalendar() override;

        // 选择模式，默认 Single；切换模式会清空当前选择
        void setSelectionMode(ShadCalendarType::SelectionMode mode);
        ShadCalendarType::SelectionMode getSelectionMode() const;

        // 单选模式的选中日期；多选模式下返回最早的一个，范围模式下返回起始日期；传无效日期清空
        void setSelectedDate(const QDate& date);
        QDate getSelectedDate() const;
        // 多选模式的选中日期，按日期升序
        void setSelectedDates(const QList<QDate>& dates);
        QList<QDate> getSelectedDates() const;
        // 范围模式的起止日期；只有起始日期时结束日期为无效日期；start 晚于 end 时自动交换
        void setSelectedRange(const QDate& start, const QDate& end);
        QDate getRangeStart() const;
        QDate getRangeEnd() const;
        void clearSelection();

        // 显示的第一个月，只取年月；超出最小 / 最大日期所在月份时限制在范围内
        void setCurrentMonth(const QDate& month);
        QDate getCurrentMonth() const;

        // 并排显示的月份数，默认 1，对应 numberOfMonths
        void setMonthCount(int count);
        int getMonthCount() const;

        // 标题样式，默认 Label；Dropdown 只在显示单个月时生效
        void setCaptionLayout(ShadCalendarType::CaptionLayout layout);
        ShadCalendarType::CaptionLayout getCaptionLayout() const;

        // 可选择的日期范围，默认 1900-01-01 到 2100-12-31；下拉标题的年份列表也取这个范围
        void setMinimumDate(const QDate& date);
        QDate getMinimumDate() const;
        void setMaximumDate(const QDate& date);
        QDate getMaximumDate() const;

        // 每周第一天，默认跟随 locale；显式设置后不再随 locale 变化
        void setFirstDayOfWeek(Qt::DayOfWeek day);
        Qt::DayOfWeek getFirstDayOfWeek() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void selectionChanged();
        // 显示的第一个月变化，month 为该月 1 日
        void currentMonthChanged(const QDate& month);
        // 用户通过鼠标或键盘选择了日期（程序调用 setSelected* 时不发出），选择状态已更新
        void dateClicked(const QDate& date);

    protected:
        bool event(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADCALENDAR_H
