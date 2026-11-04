#include "ShadBadgePrivate.h"

#include <QAbstractAnimation>
#include <QEasingCurve>
#include <QObject>
#include <QVariant>
#include <QVariantAnimation>

#include "ShadBadge.h"
#include "ShadStyleHelper.h"

ShadBadgePrivate::ShadBadgePrivate()
{
}

ShadBadgePrivate::~ShadBadgePrivate()
{
}

void ShadBadgePrivate::updateSpin()
{
    Q_Q(ShadBadge);
    const bool shouldSpin = _shadIcon == ShadIconType::LoaderCircle && q->isVisible();
    if (shouldSpin)
    {
        if (_spinAnimation == nullptr)
        {
            _spinAnimation = new QVariantAnimation(q);
            _spinAnimation->setStartValue(0.0);
            _spinAnimation->setEndValue(360.0);
            _spinAnimation->setDuration(ShadStyleHelper::kSpinDurationMs);
            _spinAnimation->setLoopCount(-1);
            _spinAnimation->setEasingCurve(QEasingCurve::Linear);
            QObject::connect(_spinAnimation, &QVariantAnimation::valueChanged, q, [this](const QVariant& value)
            {
                _spinAngle = value.toReal();
                q_ptr->update();
            });
        }
        if (_spinAnimation->state() != QAbstractAnimation::Running)
        {
            _spinAnimation->start();
        }
        return;
    }
    if (_spinAnimation != nullptr && _spinAnimation->state() != QAbstractAnimation::Stopped)
    {
        _spinAnimation->stop();
    }
    if (_spinAngle != 0.0)
    {
        _spinAngle = 0.0;
        q->update();
    }
}
