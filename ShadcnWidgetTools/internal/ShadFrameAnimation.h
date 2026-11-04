#ifndef SHADFRAMEANIMATION_H
#define SHADFRAMEANIMATION_H

#include <QEasingCurve>
#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QWindow>
#include <QtGlobal>

class QEvent;
class QTimer;
class QWidget;

// 跟随窗口帧回调推进的数值动画，组件库内部使用
// QVariantAnimation 由约 16ms 的定时器驱动，与屏幕刷新不同步，偶尔一次刷新之间推进两步，表现为某一帧步长翻倍；
// 这里通过 QWindow::requestUpdate 请求帧回调，在窗口的 UpdateRequest 中先按实际经过的时间计算数值，再让窗口正常绘制
// 目标组件不可见或没有原生窗口时直接到达终点；窗口长时间收不到帧回调（例如被最小化）时由兜底定时器结束动画
class ShadFrameAnimation : public QObject
{
    Q_OBJECT

    public:
        // target 用于查找所在窗口，同时作为父对象
        explicit ShadFrameAnimation(QWidget* target);
        ~ShadFrameAnimation() override;

        void setDuration(int msecs);
        int getDuration() const;
        void setEasingCurve(const QEasingCurve& curve);
        QEasingCurve getEasingCurve() const;
        void setStartValue(qreal value);
        qreal getStartValue() const;
        void setEndValue(qreal value);
        qreal getEndValue() const;

        // 从起始值开始播放；正在播放时重新开始
        void start();
        // 停止在当前数值，不发出 finished
        void stop();
        bool isRunning() const;

    Q_SIGNALS:
        void valueChanged(qreal value);
        void finished();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        // 按经过的时间推进一帧；forceEnd 为 true 时直接到达终点
        void advance(bool forceEnd);
        // 移除窗口事件过滤并停止兜底定时器
        void detach();

        QPointer<QWidget> _target;
        QPointer<QWindow> _window;
        QTimer* _fallbackTimer{nullptr};
        QElapsedTimer _clock;
        QEasingCurve _easingCurve{QEasingCurve::Linear};
        int _duration{250};
        qreal _startValue{0.0};
        qreal _endValue{1.0};
        bool _isRunning{false};
};

#endif // SHADFRAMEANIMATION_H
