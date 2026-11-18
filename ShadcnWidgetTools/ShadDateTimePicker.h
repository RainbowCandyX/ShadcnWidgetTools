#ifndef SHADDATETIMEPICKER_H
#define SHADDATETIMEPICKER_H

#include <QDateTime>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class ShadCalendar;

class ShadDateTimePickerPrivate;

class SHADCN_EXPORT ShadDateTimePicker : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadDateTimePicker)
    Q_PROPERTY(QDateTime dateTime READ getDateTime WRITE setDateTime NOTIFY dateTimeChanged USER true)
    Q_PROPERTY(bool secondsVisible READ isSecondsVisible WRITE setSecondsVisible)
    Q_PROPERTY(bool nowButtonVisible READ isNowButtonVisible WRITE setNowButtonVisible)
    Q_PROPERTY(QString placeholderText READ getPlaceholderText WRITE setPlaceholderText)
    Q_PROPERTY(QString displayFormat READ getDisplayFormat WRITE setDisplayFormat)
    Q_PROPERTY(bool calendarIconVisible READ isCalendarIconVisible WRITE setCalendarIconVisible)
    Q_PROPERTY(bool chevronVisible READ isChevronVisible WRITE setChevronVisible)

    public:
        explicit ShadDateTimePicker(QWidget* parent = nullptr);
        ~ShadDateTimePicker() override;

        void setDateTime(const QDateTime& dateTime);
        QDateTime getDateTime() const;

        void setSecondsVisible(bool visible);
        bool isSecondsVisible() const;

        void setNowButtonVisible(bool visible);
        bool isNowButtonVisible() const;

        void setPlaceholderText(const QString& text);
        QString getPlaceholderText() const;

        void setDisplayFormat(const QString& format);
        QString getDisplayFormat() const;
        QString getDisplayText() const;

        void setCalendarIconVisible(bool visible);
        bool isCalendarIconVisible() const;
        void setChevronVisible(bool visible);
        bool isChevronVisible() const;

        ShadCalendar* calendar() const;

        bool isPopupVisible() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    public Q_SLOTS:
        void clear();
        void showPopup();
        void hidePopup();

    Q_SIGNALS:
        void dateTimeChanged(const QDateTime& dateTime);

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

#endif // SHADDATETIMEPICKER_H
