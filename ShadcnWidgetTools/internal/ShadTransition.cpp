#include "ShadTransition.h"

#include <QEasingCurve>
#include <QPointF>
#include <QVariant>
#include <QVariantAnimation>
#include <QWidget>
#include <QtMath>

#include <chrono>

ShadTransition::ShadTransition(QWidget* target, int durationMs)
    : QObject(target), _target(target), _durationMs(qMax(0, durationMs))
{
    _animation = new QVariantAnimation(this);
    QEasingCurve easingCurve(QEasingCurve::BezierSpline);
    easingCurve.addCubicBezierSegment(QPointF(0.4, 0.0), QPointF(0.2, 1.0), QPointF(1.0, 1.0));
    _animation->setEasingCurve(easingCurve);
    connect(_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        _value = value.toReal();
        if (_target != nullptr)
        {
            _target->update();
        }
    });
}

ShadTransition::~ShadTransition()
{
}

void ShadTransition::animateTo(qreal endValue)
{
    if (qFuzzyCompare(_endValue, endValue) && (_animation->state() == QAbstractAnimation::Running || qFuzzyCompare(_value, endValue)))
    {
        return;
    }
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    const bool isRapidChange = _rapidChangeIntervalMs > 0 && now - _lastAnimateTime < std::chrono::milliseconds(_rapidChangeIntervalMs);
    _lastAnimateTime = now;
    _endValue = endValue;
    _animation->stop();
    const qreal distance = qAbs(endValue - _value);
    if (isRapidChange || _durationMs == 0 || distance <= 0.0 || _target == nullptr || !_target->isVisible())
    {
        jumpTo(endValue);
        return;
    }
    // 中途反向时按剩余距离缩短时长，保证速度一致
    _animation->setDuration(_isScaledByDistance ? qMax(1, qRound(_durationMs * qMin<qreal>(1.0, distance))) : _durationMs);
    _animation->setStartValue(_value);
    _animation->setEndValue(endValue);
    _animation->start();
}

void ShadTransition::jumpTo(qreal value)
{
    _animation->stop();
    _value = value;
    _endValue = value;
    if (_target != nullptr)
    {
        _target->update();
    }
}

qreal ShadTransition::value() const
{
    return _value;
}

void ShadTransition::setScaledByDistance(bool scaled)
{
    _isScaledByDistance = scaled;
}

void ShadTransition::setEasingCurve(const QEasingCurve& easingCurve)
{
    _animation->setEasingCurve(easingCurve);
}

void ShadTransition::setRapidChangeInterval(int intervalMs)
{
    _rapidChangeIntervalMs = qMax(0, intervalMs);
}
