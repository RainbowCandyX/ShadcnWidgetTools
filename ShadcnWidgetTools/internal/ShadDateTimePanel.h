#ifndef SHADDATETIMEPANEL_H
#define SHADDATETIMEPANEL_H

#include <QDateTime>
#include <QWidget>

class QEvent;
class QPaintEvent;
class ShadButton;
class ShadCalendar;
class ShadTimePanel;

class ShadDateTimePanel : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadDateTimePanel(QWidget* parent = nullptr);
        ~ShadDateTimePanel() override;

        ShadCalendar* calendar() const;

        void setSecondsVisible(bool visible);
        void setNowButtonVisible(bool visible);

        void setDateTime(const QDateTime& dateTime);
        QDateTime dateTime() const;

        void scrollToSelection();

    Q_SIGNALS:
        void confirmed(const QDateTime& dateTime);

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void retranslate();
        void confirm();
        void updateConfirmButton();

        QWidget* _body{nullptr};
        QWidget* _footer{nullptr};
        ShadCalendar* _calendar{nullptr};
        ShadTimePanel* _timePanel{nullptr};
        ShadButton* _nowButton{nullptr};
        ShadButton* _confirmButton{nullptr};
};

#endif // SHADDATETIMEPANEL_H
