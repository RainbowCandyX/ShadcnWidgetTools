#ifndef SHADDIALOGOVERLAY_H
#define SHADDIALOGOVERLAY_H

#include <QWidget>
#include <QtGlobal>

class QEvent;
class QObject;
class QPaintEvent;

// ShadDialog 模态打开时盖在父窗口上的遮罩，不接收鼠标事件
class ShadDialogOverlay : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadDialogOverlay(QWidget* host);
        ~ShadDialogOverlay() override;

        void setProgress(qreal progress);
        // 窗口圆角除外
        void bringToFront();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        QWidget* _host{nullptr};
        qreal _progress{0.0};
};

#endif // SHADDIALOGOVERLAY_H
