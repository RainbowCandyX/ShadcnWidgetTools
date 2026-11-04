#ifndef SHADDATEPICKER_H
#define SHADDATEPICKER_H

#include <QDate>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class ShadCalendar;

// 日期选择，对应 shadcn Date Picker：outline 按钮样式的触发框，点击后在下方弹出 ShadCalendar（PopoverContent p-0，与触发框起始侧对齐）
// 未选择时显示 muted 色的占位文字；可在起始侧显示日历图标、在结束侧显示下拉箭头
// 选中的日期按 displayFormat 显示，默认使用 locale 的长日期格式并去掉星期；范围显示为"开始 - 结束"
// closeOnSelect 默认开启：单日模式选择后关闭，范围模式选择结束日期后关闭；calendar() 可设置日期范围、下拉标题、月份数等
// 键盘：Tab 聚焦，空格 / 回车 / 向下键打开，浮层中按 Esc 关闭
class ShadDatePickerPrivate;

class SHADCN_EXPORT ShadDatePicker : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadDatePicker)
    Q_PROPERTY(ShadDatePickerType::Mode mode READ getMode WRITE setMode)
    Q_PROPERTY(QDate date READ getDate WRITE setDate NOTIFY dateChanged)
    Q_PROPERTY(QString placeholderText READ getPlaceholderText WRITE setPlaceholderText)
    Q_PROPERTY(QString displayFormat READ getDisplayFormat WRITE setDisplayFormat)
    Q_PROPERTY(bool calendarIconVisible READ isCalendarIconVisible WRITE setCalendarIconVisible)
    Q_PROPERTY(bool chevronVisible READ isChevronVisible WRITE setChevronVisible)
    Q_PROPERTY(bool closeOnSelect READ isCloseOnSelect WRITE setCloseOnSelect)

    public:
        explicit ShadDatePicker(QWidget* parent = nullptr);
        explicit ShadDatePicker(ShadDatePickerType::Mode mode, QWidget* parent = nullptr);
        ~ShadDatePicker() override;

        // 单日或范围，默认 SingleDate；切换模式会清空已选日期
        void setMode(ShadDatePickerType::Mode mode);
        ShadDatePickerType::Mode getMode() const;

        // 单日模式的日期，传无效日期清空
        void setDate(const QDate& date);
        QDate getDate() const;
        // 范围模式的起止日期，start 晚于 end 时自动交换；只有起始日期时 end 传无效日期
        void setDateRange(const QDate& start, const QDate& end);
        QDate getRangeStart() const;
        QDate getRangeEnd() const;
        void clear();

        // 未选择时的占位文字，默认"选择日期"
        void setPlaceholderText(const QString& text);
        QString getPlaceholderText() const;

        // QDate::toString 的格式字符串，例如 "yyyy-MM-dd"；为空时使用 locale 的长日期格式并去掉星期
        void setDisplayFormat(const QString& format);
        QString getDisplayFormat() const;
        // 触发框当前显示的文字，未选择时为空
        QString getDisplayText() const;

        // 起始侧的日历图标，默认不显示
        void setCalendarIconVisible(bool visible);
        bool isCalendarIconVisible() const;
        // 结束侧的下拉箭头，默认不显示
        void setChevronVisible(bool visible);
        bool isChevronVisible() const;

        // 选择后是否自动关闭浮层，默认 true
        void setCloseOnSelect(bool closeOnSelect);
        bool isCloseOnSelect() const;

        // 浮层中的日历，可设置最小 / 最大日期、标题样式、月份数等；选择模式由 setMode 决定，不要直接修改
        ShadCalendar* calendar() const;

        void showPopup();
        void hidePopup();
        bool isPopupVisible() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void dateChanged(const QDate& date);
        void rangeChanged(const QDate& start, const QDate& end);

    protected:
        bool event(QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADDATEPICKER_H
