#ifndef SHADMENUBARTRIGGER_H
#define SHADMENUBARTRIGGER_H

#include <QAbstractButton>
#include <QSize>
#include <QString>

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

// ShadMenubar 的菜单触发项
class ShadMenubarTrigger : public QAbstractButton
{
    Q_OBJECT

    public:
        explicit ShadMenubarTrigger(const QString& text, QWidget* parent = nullptr);
        ~ShadMenubarTrigger() override;

        void setOpen(bool open);
        bool isOpen() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void triggerPressed();

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        // 导航键交给 ShadMenubar
        void keyPressEvent(QKeyEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        bool _isOpen{false};
        bool _isHovered{false};
        bool _isFocusVisible{false};
};

#endif // SHADMENUBARTRIGGER_H
