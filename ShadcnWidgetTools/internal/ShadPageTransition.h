#ifndef SHADPAGETRANSITION_H
#define SHADPAGETRANSITION_H

#include <QPixmap>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QWidget>
#include <QtGlobal>

class QPaintEvent;
class ShadFrameAnimation;

// 页面切换动画，覆盖在页面区域之上；配置来自 ShadTheme
class ShadPageTransition : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadPageTransition(QWidget* area);
        ~ShadPageTransition() override;

        // 页面可以已隐藏
        void capture(QWidget* page);
        // 切换后调用；direction 为正时新页面从结束侧进入
        void play(QWidget* page, int direction);
        void finish();

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        void applyProgress(qreal progress);

        ShadFrameAnimation* _animation{nullptr};
        QPixmap _snapshot;
        QRect _snapshotRect;
        QPointer<QWidget> _page;
        QPoint _pageBase;
        int _offset{0};
        qreal _progress{1.0};
};

#endif // SHADPAGETRANSITION_H
