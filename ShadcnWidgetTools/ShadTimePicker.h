#ifndef SHADTIMEPICKER_H
#define SHADTIMEPICKER_H

#include <QSize>
#include <QString>
#include <QTime>
#include <QWidget>

#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

// 时间选择：触发框与 ShadDatePicker 相同（outline 样式、muted 占位文字、可选的时钟图标与下拉箭头），
// 点击后在下方弹出面板：时、分、秒三列可滚动选择（秒可隐藏），底部"确定"按钮写入时间并关闭；按 Esc 或点击外部关闭时不保存修改
// 面板的列顺序固定为"时 分 秒"从左到右，RTL 下不镜像；键盘：上下方向键改变当前列的值，左右方向键切换列，回车确定
class ShadTimePickerPrivate;

class SHADCN_EXPORT ShadTimePicker : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTimePicker)
    Q_PROPERTY(QTime time READ getTime WRITE setTime NOTIFY timeChanged)
    Q_PROPERTY(bool secondsVisible READ isSecondsVisible WRITE setSecondsVisible)
    Q_PROPERTY(QString placeholderText READ getPlaceholderText WRITE setPlaceholderText)
    Q_PROPERTY(QString displayFormat READ getDisplayFormat WRITE setDisplayFormat)
    Q_PROPERTY(bool clockIconVisible READ isClockIconVisible WRITE setClockIconVisible)
    Q_PROPERTY(bool chevronVisible READ isChevronVisible WRITE setChevronVisible)

    public:
        explicit ShadTimePicker(QWidget* parent = nullptr);
        ~ShadTimePicker() override;

        // 当前时间，传无效时间清空
        void setTime(const QTime& time);
        QTime getTime() const;
        void clear();

        // 是否显示秒这一列，默认 true；隐藏后选择的时间秒为 0
        void setSecondsVisible(bool visible);
        bool isSecondsVisible() const;

        // 未选择时的占位文字，默认"选择时间"
        void setPlaceholderText(const QString& text);
        QString getPlaceholderText() const;

        // QTime::toString 的格式字符串；为空时显示秒为 "HH:mm:ss"，不显示秒为 "HH:mm"
        void setDisplayFormat(const QString& format);
        QString getDisplayFormat() const;
        // 触发框当前显示的文字，未选择时为空
        QString getDisplayText() const;

        // 起始侧的时钟图标，默认不显示
        void setClockIconVisible(bool visible);
        bool isClockIconVisible() const;
        // 结束侧的下拉箭头，默认不显示
        void setChevronVisible(bool visible);
        bool isChevronVisible() const;

        void showPopup();
        void hidePopup();
        bool isPopupVisible() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void timeChanged(const QTime& time);

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

#endif // SHADTIMEPICKER_H
