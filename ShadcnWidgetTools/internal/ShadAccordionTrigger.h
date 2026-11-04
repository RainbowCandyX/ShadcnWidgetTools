#ifndef SHADACCORDIONTRIGGER_H
#define SHADACCORDIONTRIGGER_H

#include <QAbstractButton>
#include <QSize>

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QPaintEvent;

// ShadAccordion 每一项的标题行
class ShadAccordionTrigger : public QAbstractButton
{
    Q_OBJECT

    public:
        explicit ShadAccordionTrigger(QWidget* parent = nullptr);
        ~ShadAccordionTrigger() override;

        void setExpanded(bool expanded);
        bool isExpanded() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;
        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;

    protected:
        // 方向键与 Home / End 交给外层处理
        void keyPressEvent(QKeyEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        int textWidth(int width) const;
        int textHeight(int width) const;

        bool _isExpanded{false};
        bool _isHovered{false};
        bool _isFocusVisible{false};
};

#endif // SHADACCORDIONTRIGGER_H
