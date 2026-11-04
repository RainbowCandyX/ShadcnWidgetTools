#include "ShadFrameAnimation.h"

#include <QEvent>
#include <QTimer>
#include <QWidget>
#include <QWindow>
#include <QtGlobal>

namespace
{
    // 兜底定时器在动画时长之后再等待的时间
    constexpr int kFallbackMarginMs = 100;
} // namespace

ShadFrameAnimation::ShadFrameAnimation(QWidget* target)
    : QObject(target), _target(target)
{
    _fallbackTimer = new QTimer(this);
    _fallbackTimer->setSingleShot(true);
    connect(_fallbackTimer, &QTimer::timeout, this, [this]()
    {
        if (_isRunning)
        {
            advance(true);
        }
    });
}

ShadFrameAnimation::~ShadFrameAnimation()
{
    detach();
}

void ShadFrameAnimation::setDuration(int msecs)
{
    _duration = qMax(0, msecs);
}

int ShadFrameAnimation::getDuration() const
{
    return _duration;
}

void ShadFrameAnimation::setEasingCurve(const QEasingCurve& curve)
{
    _easingCurve = curve;
}

QEasingCurve ShadFrameAnimation::getEasingCurve() const
{
    return _easingCurve;
}

void ShadFrameAnimation::setStartValue(qreal value)
{
    _startValue = value;
}

qreal ShadFrameAnimation::getStartValue() const
{
    return _startValue;
}

void ShadFrameAnimation::setEndValue(qreal value)
{
    _endValue = value;
}

qreal ShadFrameAnimation::getEndValue() const
{
    return _endValue;
}

void ShadFrameAnimation::start()
{
    stop();
    QWidget* window = _target.isNull() ? nullptr : _target->window();
    QWindow* handle = window != nullptr ? window->windowHandle() : nullptr;
    if (handle == nullptr || !_target->isVisible() || _duration <= 0)
    {
        // 没有可显示的窗口就没有帧回调，直接到达终点
        _isRunning = true;
        advance(true);
        return;
    }
    _window = handle;
    _window->installEventFilter(this);
    _isRunning = true;
    _clock.start();
    _fallbackTimer->start(_duration + kFallbackMarginMs);
    _window->requestUpdate();
}

void ShadFrameAnimation::stop()
{
    _isRunning = false;
    detach();
}

bool ShadFrameAnimation::isRunning() const
{
    return _isRunning;
}

bool ShadFrameAnimation::eventFilter(QObject* watched, QEvent* event)
{
    // 在窗口处理 UpdateRequest（同步并绘制）之前推进，本帧画出的就是当前时间对应的数值
    if (_isRunning && watched == _window.data() && event->type() == QEvent::UpdateRequest)
    {
        advance(false);
    }
    return QObject::eventFilter(watched, event);
}

void ShadFrameAnimation::advance(bool forceEnd)
{
    const qreal progress = forceEnd || _duration <= 0 ? 1.0 : qMin<qreal>(1.0, static_cast<qreal>(_clock.elapsed()) / _duration);
    const bool isFinished = progress >= 1.0;
    const qreal value = isFinished ? _endValue : _startValue + (_endValue - _startValue) * _easingCurve.valueForProgress(progress);
    if (isFinished)
    {
        // 先结束再发出信号，槽函数中重新 start 不会被这里覆盖
        _isRunning = false;
        detach();
    }
    Q_EMIT valueChanged(value);
    if (isFinished)
    {
        Q_EMIT finished();
        return;
    }
    // 槽函数中可能调用了 stop 或 start，只在仍为本轮播放时请求下一帧
    if (_isRunning && !_window.isNull())
    {
        _window->requestUpdate();
    }
}

void ShadFrameAnimation::detach()
{
    _fallbackTimer->stop();
    if (!_window.isNull())
    {
        _window->removeEventFilter(this);
    }
    _window = nullptr;
}
