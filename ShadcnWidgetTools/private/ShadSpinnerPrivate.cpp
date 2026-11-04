#include "ShadSpinnerPrivate.h"

#include <QAbstractAnimation>
#include <QVariantAnimation>

#include "ShadSpinner.h"
#include "ShadTheme.h"

ShadSpinnerPrivate::ShadSpinnerPrivate()
{
}

ShadSpinnerPrivate::~ShadSpinnerPrivate()
{
}

void ShadSpinnerPrivate::updateAnimation()
{
    Q_Q(ShadSpinner);
    const bool isActive = _isRunning && _icon != ShadIconType::None;
    if (!isActive)
    {
        _animation->stop();
        if (_angle != 0.0)
        {
            _angle = 0.0;
            q->update();
        }
        return;
    }
    // 隐藏时暂停而不是停止，重新显示后从当前角度继续，避免跳回起点
    if (!q->isVisible())
    {
        if (_animation->state() == QAbstractAnimation::Running)
        {
            _animation->pause();
        }
        return;
    }
    if (_animation->state() == QAbstractAnimation::Paused)
    {
        _animation->resume();
    }
    else if (_animation->state() == QAbstractAnimation::Stopped)
    {
        _animation->start();
    }
}

QColor ShadSpinnerPrivate::iconColor() const
{
    return _color.isValid() ? _color : shadTheme->getColor(_colorRole);
}
