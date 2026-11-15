#include "ShadSkeletonPrivate.h"

#include <QAbstractAnimation>
#include <QCoreApplication>
#include <QEasingCurve>
#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QSet>
#include <QVariant>
#include <QVariantAnimation>
#include <QWidget>

#include <utility>

#include "ShadSkeleton.h"

namespace
{
    constexpr int kPulseDurationMs = 2000;
    constexpr qreal kPulseMinOpacity = 0.5;

    // 全部骨架屏共用一个动画，保持同步
    class SkeletonPulse : public QObject
    {
        public:
            // 随应用对象一起销毁
            static SkeletonPulse* instance()
            {
                static QPointer<SkeletonPulse> pulse;
                if (pulse.isNull())
                {
                    pulse = new SkeletonPulse(QCoreApplication::instance());
                }
                return pulse.data();
            }

            void add(QWidget* widget)
            {
                _widgets.insert(widget);
                if (_animation->state() != QAbstractAnimation::Running)
                {
                    _animation->start();
                }
            }

            void remove(QWidget* widget)
            {
                _widgets.remove(widget);
                if (_widgets.isEmpty())
                {
                    _animation->stop();
                    _opacity = 1.0;
                }
            }

            qreal opacity() const
            {
                return _opacity;
            }

        private:
            explicit SkeletonPulse(QObject* parent)
                : QObject(parent)
            {
                _curve.addCubicBezierSegment(QPointF(0.4, 0.0), QPointF(0.6, 1.0), QPointF(1.0, 1.0));
                _animation = new QVariantAnimation(this);
                _animation->setStartValue(0.0);
                _animation->setEndValue(1.0);
                _animation->setDuration(kPulseDurationMs);
                _animation->setLoopCount(-1);
                _animation->setEasingCurve(QEasingCurve::Linear);
                QObject::connect(_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
                {
                    const qreal progress = value.toReal();
                    const qreal half = progress < 0.5 ? progress * 2.0 : (1.0 - progress) * 2.0;
                    _opacity = 1.0 - (1.0 - kPulseMinOpacity) * _curve.valueForProgress(half);
                    for (QWidget* widget : std::as_const(_widgets))
                    {
                        widget->update();
                    }
                });
            }

            QVariantAnimation* _animation{nullptr};
            QEasingCurve _curve{QEasingCurve::BezierSpline};
            QSet<QWidget*> _widgets;
            qreal _opacity{1.0};
    };
} // namespace

ShadSkeletonPrivate::ShadSkeletonPrivate()
{
}

ShadSkeletonPrivate::~ShadSkeletonPrivate()
{
}

void ShadSkeletonPrivate::updateAnimation()
{
    Q_Q(ShadSkeleton);
    const bool isPulsing = _isAnimated && q->isVisible();
    if (_isPulsing == isPulsing)
    {
        return;
    }
    _isPulsing = isPulsing;
    if (isPulsing)
    {
        SkeletonPulse::instance()->add(q);
    }
    else
    {
        SkeletonPulse::instance()->remove(q);
    }
    q->update();
}

qreal ShadSkeletonPrivate::pulseOpacity() const
{
    return _isPulsing ? SkeletonPulse::instance()->opacity() : 1.0;
}
