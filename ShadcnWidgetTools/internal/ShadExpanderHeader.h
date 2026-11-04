#ifndef SHADEXPANDERHEADER_H
#define SHADEXPANDERHEADER_H

#include <QWidget>
#include <QtGlobal>

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QHBoxLayout;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

// ShadExpander 的标题行
class ShadExpanderHeader : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadExpanderHeader(QWidget* parent = nullptr);
        ~ShadExpanderHeader() override;

        QHBoxLayout* contentLayout() const;
        void setProgress(qreal progress);

    Q_SIGNALS:
        void clicked();

    protected:
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        QHBoxLayout* _layout{nullptr};
        qreal _progress{0.0};
        bool _isHovered{false};
        bool _isPressed{false};
        bool _isFocusVisible{false};
};

#endif // SHADEXPANDERHEADER_H
