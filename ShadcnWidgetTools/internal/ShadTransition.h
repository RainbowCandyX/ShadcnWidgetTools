#ifndef SHADTRANSITION_H
#define SHADTRANSITION_H

#include <QObject>
#include <QtGlobal>

#include <chrono>

class QEasingCurve;
class QVariantAnimation;
class QWidget;

// 0 到 1 之间的状态过渡动画，数值变化时自动重绘目标组件
// 默认 150ms + cubic-bezier(0.4, 0, 0.2, 1)，与 Tailwind transition 默认值一致
class ShadTransition : public QObject
{
    Q_OBJECT

    public:
        explicit ShadTransition(QWidget* target, int durationMs = 150);
        ~ShadTransition() override;

        void animateTo(qreal endValue);
        void jumpTo(qreal value);
        qreal value() const;
        void setScaledByDistance(bool scaled);
        void setEasingCurve(const QEasingCurve& easingCurve);
        void setRapidChangeInterval(int intervalMs);

    private:
        QWidget* _target{nullptr};
        QVariantAnimation* _animation{nullptr};
        int _durationMs{150};
        bool _isScaledByDistance{true};
        int _rapidChangeIntervalMs{0};
        std::chrono::steady_clock::time_point _lastAnimateTime;
        qreal _value{0.0};
        qreal _endValue{0.0};
};

#endif // SHADTRANSITION_H
