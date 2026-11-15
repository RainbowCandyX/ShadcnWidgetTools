#include "ShadMarqueePrivate.h"

#include <QAbstractAnimation>
#include <QVariantAnimation>

#include "ShadMarquee.h"
#include "ShadStyleHelper.h"

ShadMarqueePrivate::ShadMarqueePrivate()
{
}

ShadMarqueePrivate::~ShadMarqueePrivate()
{
}

void ShadMarqueePrivate::updateTextWidth()
{
    Q_Q(ShadMarquee);
    _textWidth = _text.isEmpty() ? 0 : ShadStyleHelper::textWidth(q->font(), _text);
    configureAnimation();
}

void ShadMarqueePrivate::configureAnimation()
{
    Q_Q(ShadMarquee);
    _animation->stop();
    _offset = 0.0;
    const int period = _textWidth + _gap;
    if (period > 0 && _speed > 0)
    {
        _animation->setEndValue(qreal(period));
        _animation->setDuration(qMax(1, qRound(period * 1000.0 / _speed)));
    }
    updateAnimation();
    q->update();
}

void ShadMarqueePrivate::updateAnimation()
{
    Q_Q(ShadMarquee);
    if (!isScrollable())
    {
        _animation->stop();
        if (_offset != 0.0)
        {
            _offset = 0.0;
            q->update();
        }
        return;
    }
    // 暂停而不是停止，恢复后继续
    if (!q->isVisible() || (_isPauseOnHover && _isHovered))
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

bool ShadMarqueePrivate::isScrollable() const
{
    Q_Q(const ShadMarquee);
    if (!_isRunning || _speed <= 0 || _textWidth <= 0)
    {
        return false;
    }
    return !_isOverflowOnly || _textWidth > q->width();
}
