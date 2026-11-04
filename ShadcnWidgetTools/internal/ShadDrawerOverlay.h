#ifndef SHADDRAWEROVERLAY_H
#define SHADDRAWEROVERLAY_H

#include <QPixmap>
#include <QPointer>
#include <QWidget>
#include <QtGlobal>

class QEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QWheelEvent;

// 模态 ShadDrawer 的遮罩，拦截鼠标
class ShadDrawerOverlay : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadDrawerOverlay(QWidget* host);
        ~ShadDrawerOverlay() override;

        void setLevel(qreal level);
        void refreshBackdrop();

    Q_SIGNALS:
        void pressed();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        QPointer<QWidget> _host;
        QPixmap _backdrop;
        qreal _level{0.0};
};

#endif // SHADDRAWEROVERLAY_H
