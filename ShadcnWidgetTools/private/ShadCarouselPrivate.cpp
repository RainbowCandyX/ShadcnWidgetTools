#include "ShadCarouselPrivate.h"

#include <QAbstractSlider>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QLineEdit>
#include <QList>
#include <QMouseEvent>
#include <QObject>
#include <QPlainTextEdit>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>
#include <QtMath>

#include <cmath>

#include "ShadButton.h"
#include "ShadCarousel.h"
#include "ShadFrameAnimation.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"

namespace
{
    // 视口还没有尺寸时用于计算停靠点的名义幻灯片长度；停靠点数量只与比例有关，尺寸确定后会重新计算
    constexpr qreal kNominalItemExtent = 100.0;
    // 取消子组件点击时发送的松开位置，远离任何组件
    constexpr qreal kCancelPosition = -100000.0;
} // namespace

ShadCarouselPrivate::ShadCarouselPrivate()
{
}

ShadCarouselPrivate::~ShadCarouselPrivate()
{
}

int ShadCarouselPrivate::navigationMargin() const
{
    return _isNavigationVisible ? kButtonOffset + ShadStyleHelper::kFocusRingWidth : 0;
}

QRect ShadCarouselPrivate::viewportRect(const QRect& bounds) const
{
    const int margin = navigationMargin();
    if (_orientation == Qt::Horizontal)
    {
        return bounds.adjusted(margin, 0, -margin, 0);
    }
    return bounds.adjusted(0, margin, 0, -margin);
}

qreal ShadCarouselPrivate::viewportExtent() const
{
    const qreal length = _orientation == Qt::Horizontal ? _viewport->width() : _viewport->height();
    if (length > 0)
    {
        return length;
    }
    return _itemsPerView * kNominalItemExtent + (_itemsPerView - 1) * _spacing;
}

qreal ShadCarouselPrivate::slotExtent(qreal viewportLength) const
{
    return (viewportLength + _spacing) / _itemsPerView;
}

bool ShadCarouselPrivate::isLoopActive() const
{
    return _isLoop && _items.size() > _itemsPerView;
}

qreal ShadCarouselPrivate::loopPeriod() const
{
    return _items.size() * slotExtent(viewportExtent());
}

qreal ShadCarouselPrivate::maxOffset() const
{
    const qreal length = viewportExtent();
    const qreal trackLength = _items.size() * slotExtent(length) - _spacing;
    return qMax<qreal>(0.0, trackLength - length);
}

qreal ShadCarouselPrivate::wrapDelta(qreal delta, Direction direction) const
{
    const qreal period = loopPeriod();
    if (period <= 0.0)
    {
        return delta;
    }
    qreal wrapped = std::fmod(delta, period);
    switch (direction)
    {
        case Forward:
            {
                if (wrapped <= kSnapTolerance)
                {
                    wrapped += period;
                }
                break;
            }
        case Backward:
            {
                if (wrapped >= -kSnapTolerance)
                {
                    wrapped -= period;
                }
                break;
            }
        case Nearest:
            {
                if (wrapped > period / 2.0)
                {
                    wrapped -= period;
                }
                else if (wrapped <= -period / 2.0)
                {
                    wrapped += period;
                }
                break;
            }
    }
    return wrapped;
}

int ShadCarouselPrivate::nearestSnap(qreal offset) const
{
    const bool isLoop = isLoopActive();
    int nearest = -1;
    qreal nearestDistance = 0.0;
    for (int index = 0; index < _snaps.size(); ++index)
    {
        const qreal delta = _snaps.at(index) - offset;
        const qreal distance = qAbs(isLoop ? wrapDelta(delta, Nearest) : delta);
        if (nearest < 0 || distance < nearestDistance)
        {
            nearest = index;
            nearestDistance = distance;
        }
    }
    return nearest;
}

qreal ShadCarouselPrivate::snapTarget(int index, qreal offset, Direction direction) const
{
    if (!isLoopActive())
    {
        return _snaps.at(index);
    }
    return offset + wrapDelta(_snaps.at(index) - offset, direction);
}

int ShadCarouselPrivate::itemMainHint(const QWidget* item) const
{
    const QSize hint = item->sizeHint().expandedTo(item->minimumSizeHint());
    if (_orientation == Qt::Horizontal)
    {
        return qBound(item->minimumWidth(), hint.width(), item->maximumWidth());
    }
    return qBound(item->minimumHeight(), hint.height(), item->maximumHeight());
}

int ShadCarouselPrivate::itemCrossHint(const QWidget* item, int mainExtent) const
{
    if (_orientation == Qt::Horizontal)
    {
        const int height = item->hasHeightForWidth() ? item->heightForWidth(mainExtent) : item->sizeHint().expandedTo(item->minimumSizeHint()).height();
        return qBound(item->minimumHeight(), height, item->maximumHeight());
    }
    const int width = item->sizeHint().expandedTo(item->minimumSizeHint()).width();
    return qBound(item->minimumWidth(), width, item->maximumWidth());
}

int ShadCarouselPrivate::contentCrossHint(int width) const
{
    const qreal length = qMax(0, width - 2 * navigationMargin());
    const int itemExtent = qMax(0, qFloor(slotExtent(length) - _spacing));
    int cross = 0;
    for (const QWidget* item : _items)
    {
        cross = qMax(cross, itemCrossHint(item, itemExtent));
    }
    if (_isNavigationVisible)
    {
        cross = qMax(cross, _prevButton->sizeHint().height());
    }
    return cross;
}

int ShadCarouselPrivate::contentMainHintVertical() const
{
    int itemExtent = 0;
    for (const QWidget* item : _items)
    {
        itemExtent = qMax(itemExtent, itemMainHint(item));
    }
    return _itemsPerView * itemExtent + (_itemsPerView - 1) * _spacing;
}

bool ShadCarouselPrivate::updateSnaps()
{
    const QList<qreal> oldSnaps = _snaps;
    _snaps.clear();
    const int itemCount = int(_items.size());
    if (itemCount > 0)
    {
        const qreal length = viewportExtent();
        const qreal slot = slotExtent(length);
        const qreal itemExtent = slot - _spacing;
        qreal alignShift = 0.0;
        if (_align == ShadCarouselType::Center)
        {
            alignShift = (itemExtent - length) / 2.0;
        }
        else if (_align == ShadCarouselType::End)
        {
            alignShift = itemExtent - length;
        }
        if (isLoopActive())
        {
            for (int index = 0; index < itemCount; ++index)
            {
                _snaps.append(index * slot + alignShift);
            }
        }
        else
        {
            // containScroll="trimSnaps"：停靠点限制在内容范围内，首尾重合的停靠点只保留一个
            const qreal maximum = maxOffset();
            if (maximum <= kSnapTolerance)
            {
                _snaps.append(0.0);
            }
            else
            {
                QList<qreal> contained;
                for (int index = 0; index < itemCount; ++index)
                {
                    contained.append(qBound<qreal>(0.0, index * slot + alignShift, maximum));
                }
                int startEdge = 0;
                for (int index = 0; index < contained.size(); ++index)
                {
                    if (contained.at(index) <= kSnapTolerance)
                    {
                        startEdge = index;
                    }
                }
                int endEdge = int(contained.size()) - 1;
                for (int index = 0; index < contained.size(); ++index)
                {
                    if (contained.at(index) >= maximum - kSnapTolerance)
                    {
                        endEdge = index;
                        break;
                    }
                }
                for (int index = startEdge; index <= endEdge; ++index)
                {
                    _snaps.append(contained.at(index));
                }
            }
        }
    }
    if (_snaps.isEmpty())
    {
        setCurrent(-1);
    }
    else if (_currentIndex < 0)
    {
        setCurrent(0);
    }
    else if (_currentIndex >= _snaps.size())
    {
        setCurrent(int(_snaps.size()) - 1);
    }
    if (_snaps.size() != oldSnaps.size())
    {
        Q_Q(ShadCarousel);
        Q_EMIT q->snapCountChanged(int(_snaps.size()));
    }
    return _snaps != oldSnaps;
}

void ShadCarouselPrivate::updateLayout()
{
    Q_Q(ShadCarousel);
    const QRect bounds = q->rect();
    _viewport->setGeometry(viewportRect(bounds));
    if (_isNavigationVisible)
    {
        const QSize buttonSize = _prevButton->sizeHint();
        QRect prevRect;
        QRect nextRect;
        if (_orientation == Qt::Horizontal)
        {
            const int y = (bounds.height() - buttonSize.height()) / 2;
            prevRect = ShadStyleHelper::visualRect(q->layoutDirection(), bounds, QRect(QPoint(0, y), buttonSize));
            nextRect = ShadStyleHelper::visualRect(q->layoutDirection(), bounds, QRect(QPoint(bounds.width() - buttonSize.width(), y), buttonSize));
        }
        else
        {
            const int x = (bounds.width() - buttonSize.width()) / 2;
            prevRect = QRect(QPoint(x, 0), buttonSize);
            nextRect = QRect(QPoint(x, bounds.height() - buttonSize.height()), buttonSize);
        }
        _prevButton->setGeometry(prevRect);
        _nextButton->setGeometry(nextRect);
    }
    if (!_isNavigationVisible)
    {
        ShadStyleHelper::clearFocusWithin(_prevButton);
        ShadStyleHelper::clearFocusWithin(_nextButton);
    }
    _prevButton->setVisible(_isNavigationVisible);
    _nextButton->setVisible(_isNavigationVisible);
    const bool isSnapsChanged = updateSnaps();
    // 停靠点变化后正在播放的动画目标已失效，直接停到当前停靠点；停靠点不变时（例如幻灯片只是更新了尺寸提示）不打断动画
    // 按下但还没有开始拖动时保持位置，松开后由 settle 继续停靠
    if (!_isPointerDown && (isSnapsChanged || !_animation->isRunning()))
    {
        stopAnimation();
        _offset = _currentIndex >= 0 ? _snaps.at(_currentIndex) : 0.0;
    }
    layoutItems();
    updateButtons();
}

void ShadCarouselPrivate::layoutItems()
{
    Q_Q(ShadCarousel);
    const int itemCount = int(_items.size());
    const qreal length = _orientation == Qt::Horizontal ? _viewport->width() : _viewport->height();
    if (itemCount == 0 || length <= 0)
    {
        return;
    }
    const qreal slot = slotExtent(length);
    const qreal itemExtent = slot - _spacing;
    const int cross = _orientation == Qt::Horizontal ? _viewport->height() : _viewport->width();
    const bool isLoop = isLoopActive();
    const qreal period = itemCount * slot;
    for (int index = 0; index < itemCount; ++index)
    {
        qreal position = index * slot - _offset;
        if (isLoop)
        {
            // 位置按一圈取模放到 [-slot, period - slot) 内，视口两侧都能接上首尾的幻灯片
            position = std::fmod(position + slot, period);
            if (position < 0.0)
            {
                position += period;
            }
            position -= slot;
        }
        const int start = qRound(position);
        const int end = qRound(position + itemExtent);
        QRect geometry;
        if (_orientation == Qt::Horizontal)
        {
            geometry = ShadStyleHelper::visualRect(q->layoutDirection(), _viewport->rect(), QRect(start, 0, end - start, cross));
        }
        else
        {
            geometry = QRect(0, start, cross, end - start);
        }
        _items.at(index)->setGeometry(geometry);
    }
}

void ShadCarouselPrivate::setOffset(qreal offset)
{
    _offset = offset;
    layoutItems();
}

void ShadCarouselPrivate::updateButtons()
{
    Q_Q(ShadCarousel);
    if (_orientation == Qt::Horizontal)
    {
        // 图标指向切换方向：RTL 下上一张在右侧，箭头朝右
        const bool isRightToLeft = q->layoutDirection() == Qt::RightToLeft;
        _prevButton->setShadIcon(isRightToLeft ? ShadIconType::ChevronRight : ShadIconType::ChevronLeft);
        _nextButton->setShadIcon(isRightToLeft ? ShadIconType::ChevronLeft : ShadIconType::ChevronRight);
    }
    else
    {
        // 对应纵向时按钮的 rotate-90
        _prevButton->setShadIcon(ShadIconType::ChevronUp);
        _nextButton->setShadIcon(ShadIconType::ChevronDown);
    }
    ShadStyleHelper::setNavigationButtonsEnabled(_prevButton, q->canScrollPrev(), _nextButton, q->canScrollNext());
}

void ShadCarouselPrivate::setCurrent(int index)
{
    if (_currentIndex == index)
    {
        return;
    }
    Q_Q(ShadCarousel);
    _currentIndex = index;
    updateButtons();
    Q_EMIT q->currentChanged(index);
}

void ShadCarouselPrivate::scrollToSnap(int index, Direction direction, bool animated)
{
    if (_snaps.isEmpty())
    {
        return;
    }
    const int snapCount = int(_snaps.size());
    if (isLoopActive())
    {
        index = ((index % snapCount) + snapCount) % snapCount;
    }
    else
    {
        index = qBound(0, index, snapCount - 1);
    }
    // 拖动过程中通过代码切换时放弃本次拖动
    if (_isPointerDown)
    {
        resetPointer();
    }
    // 动画进行中再次切换时从上一次的目标位置出发计算（与 Embla 一致）：从当前位置出发时，按住方向键连续切换会让位置越来越落后，
    // 循环模式下落后接近一圈时向前查找会绕到更近的一圈，目标往回缩，表现为停顿
    const qreal base = _animation->isRunning() ? _animation->getEndValue() : _offset;
    const qreal target = snapTarget(index, base, direction);
    setCurrent(index);
    if (animated)
    {
        animateTo(target);
        return;
    }
    stopAnimation();
    setOffset(target);
}

void ShadCarouselPrivate::animateTo(qreal target)
{
    if (qAbs(target - _offset) < kSnapTolerance)
    {
        stopAnimation();
        setOffset(target);
        return;
    }
    _animation->setStartValue(_offset);
    _animation->setEndValue(target);
    _animation->start();
}

void ShadCarouselPrivate::stopAnimation()
{
    _animation->stop();
}

void ShadCarouselPrivate::settle()
{
    if (_currentIndex >= 0 && _currentIndex < _snaps.size())
    {
        animateTo(snapTarget(_currentIndex, _offset, Nearest));
    }
}

int ShadCarouselPrivate::keyStep(int key) const
{
    Q_Q(const ShadCarousel);
    if (_orientation == Qt::Vertical)
    {
        if (key == Qt::Key_Up)
        {
            return -1;
        }
        return key == Qt::Key_Down ? 1 : 0;
    }
    const int forward = q->layoutDirection() == Qt::RightToLeft ? -1 : 1;
    if (key == Qt::Key_Left)
    {
        return -forward;
    }
    return key == Qt::Key_Right ? forward : 0;
}

void ShadCarouselPrivate::restartAutoplay()
{
    Q_Q(ShadCarousel);
    _autoplayTimer->stop();
    if (_autoplayInterval > 0 && q->isVisible() && !_isHovered && !_isPointerDown && _snaps.size() > 1)
    {
        _autoplayTimer->start(_autoplayInterval);
    }
}

void ShadCarouselPrivate::autoplayStep()
{
    Q_Q(ShadCarousel);
    if (q->canScrollNext())
    {
        scrollToSnap(_currentIndex + 1, Forward, true);
    }
    else
    {
        // 与 Autoplay 插件一致：不能继续向后时回到第一个停靠点
        scrollToSnap(0, Nearest, true);
    }
    restartAutoplay();
}

void ShadCarouselPrivate::installFilter(QObject* object)
{
    Q_Q(ShadCarousel);
    if (object == nullptr || !object->isWidgetType())
    {
        return;
    }
    // 先移除再安装，避免重复安装后同一事件被处理多次
    object->removeEventFilter(q);
    object->installEventFilter(q);
    const QObjectList children = object->children();
    for (QObject* child : children)
    {
        installFilter(child);
    }
}

void ShadCarouselPrivate::removeFilter(QObject* object)
{
    Q_Q(ShadCarousel);
    if (object == nullptr || !object->isWidgetType())
    {
        return;
    }
    object->removeEventFilter(q);
    const QObjectList children = object->children();
    for (QObject* child : children)
    {
        removeFilter(child);
    }
}

bool ShadCarouselPrivate::isDragExcluded(QWidget* widget) const
{
    for (QWidget* current = widget; current != nullptr && current != _viewport; current = current->parentWidget())
    {
        if (qobject_cast<QLineEdit*>(current) != nullptr || qobject_cast<QTextEdit*>(current) != nullptr || qobject_cast<QPlainTextEdit*>(current) != nullptr || qobject_cast<QAbstractSpinBox*>(current) != nullptr || qobject_cast<QComboBox*>(current) != nullptr || qobject_cast<QAbstractSlider*>(current) != nullptr)
        {
            return true;
        }
    }
    return false;
}

bool ShadCarouselPrivate::handleMouseEvent(QWidget* watched, QMouseEvent* event)
{
    const QPoint globalPos = event->globalPosition().toPoint();
    switch (event->type())
    {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
            {
                if (event->button() != Qt::LeftButton)
                {
                    return _isDragging;
                }
                if (_isDragging || _snaps.isEmpty() || isDragExcluded(watched))
                {
                    return false;
                }
                // 子组件不处理按下时事件会继续交给父组件，这里会再次收到；最后收到的是接受按下的组件，取消点击时发给它
                _isPointerDown = true;
                _pressWidget = watched;
                _pressGlobalPos = globalPos;
                // 动画中按下会停在当前位置（对应 Embla 按下时停止滚动）
                stopAnimation();
                _pressOffset = _offset;
                _pressSnap = nearestSnap(_offset);
                _autoplayTimer->stop();
                return false;
            }
        case QEvent::MouseMove:
            {
                if (!_isPointerDown)
                {
                    return false;
                }
                if (!event->buttons().testFlag(Qt::LeftButton))
                {
                    // 没有收到松开事件（例如松开时被其他窗口截获），结束本次跟踪
                    const bool wasDragging = _isDragging;
                    if (wasDragging)
                    {
                        endDrag();
                    }
                    else
                    {
                        resetPointer();
                        settle();
                        restartAutoplay();
                    }
                    return wasDragging;
                }
                const qreal mainDelta = logicalAxisPosition(globalPos) - logicalAxisPosition(_pressGlobalPos);
                if (!_isDragging)
                {
                    const qreal crossDelta = logicalCrossPosition(globalPos) - logicalCrossPosition(_pressGlobalPos);
                    if (qMax(qAbs(mainDelta), qAbs(crossDelta)) < kDragThreshold)
                    {
                        return false;
                    }
                    if (qAbs(mainDelta) < qAbs(crossDelta))
                    {
                        // 主要沿交叉方向移动，不是切换手势，交给子组件
                        resetPointer();
                        settle();
                        restartAutoplay();
                        return false;
                    }
                    beginDrag(globalPos);
                }
                qreal offset = _pressOffset - mainDelta;
                if (!isLoopActive())
                {
                    const qreal maximum = maxOffset();
                    if (offset < 0.0)
                    {
                        offset *= kEdgeResistance;
                    }
                    else if (offset > maximum)
                    {
                        offset = maximum + (offset - maximum) * kEdgeResistance;
                    }
                }
                setOffset(offset);
                addSample(globalPos);
                return true;
            }
        case QEvent::MouseButtonRelease:
            {
                if (event->button() != Qt::LeftButton || !_isPointerDown)
                {
                    return _isDragging;
                }
                if (_isDragging)
                {
                    addSample(globalPos);
                    endDrag();
                    return true;
                }
                resetPointer();
                settle();
                restartAutoplay();
                return false;
            }
        default:
            {
                return false;
            }
    }
}

qreal ShadCarouselPrivate::logicalAxisPosition(const QPoint& globalPos) const
{
    Q_Q(const ShadCarousel);
    if (_orientation == Qt::Vertical)
    {
        return globalPos.y();
    }
    return q->layoutDirection() == Qt::RightToLeft ? -globalPos.x() : globalPos.x();
}

qreal ShadCarouselPrivate::logicalCrossPosition(const QPoint& globalPos) const
{
    return _orientation == Qt::Vertical ? globalPos.x() : globalPos.y();
}

void ShadCarouselPrivate::addSample(const QPoint& globalPos)
{
    const qint64 now = _dragClock.elapsed();
    _samples.append({now, logicalAxisPosition(globalPos)});
    while (_samples.size() > 2 && now - _samples.first().time > kVelocityWindowMs)
    {
        _samples.removeFirst();
    }
}

void ShadCarouselPrivate::beginDrag(const QPoint& globalPos)
{
    _isDragging = true;
    _dragClock.start();
    _samples.clear();
    addSample(globalPos);
    // 拖动开始后取消子组件的点击：发送一次落在组件之外的松开事件，按钮收到后恢复未按下状态且不触发 clicked
    if (!_pressWidget.isNull() && _pressWidget.data() != _viewport)
    {
        const QPointF outside(kCancelPosition, kCancelPosition);
        QMouseEvent release(QEvent::MouseButtonRelease, outside, outside, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        _isSendingCancel = true;
        QCoreApplication::sendEvent(_pressWidget.data(), &release);
        _isSendingCancel = false;
    }
}

void ShadCarouselPrivate::endDrag()
{
    // 松开前最近一段时间内的平均速度（像素/毫秒，指针向结束侧移动为正）；指针停住超过统计时长时视为 0
    qreal velocity = 0.0;
    if (_samples.size() >= 2 && _dragClock.elapsed() - _samples.last().time <= kVelocityWindowMs)
    {
        const DragSample& first = _samples.first();
        const DragSample& last = _samples.last();
        if (last.time > first.time)
        {
            velocity = (last.position - first.position) / static_cast<qreal>(last.time - first.time);
        }
    }
    const qreal pressOffset = _pressOffset;
    resetPointer();
    if (_snaps.isEmpty())
    {
        restartAutoplay();
        return;
    }
    const int snapCount = int(_snaps.size());
    // 拖动过程中尺寸变化导致停靠点重新计算时，按下时记录的停靠点可能已经无效
    const int pressSnap = _pressSnap >= 0 && _pressSnap < snapCount ? _pressSnap : nearestSnap(_offset);
    const qreal projected = _offset - velocity * kFlickProjectionMs;
    // 最多移动一个停靠点（对应 Embla 的 skipSnaps=false）：在按下时的停靠点与前后两个停靠点中取离预估位置最近的
    int candidates[3];
    qreal positions[3];
    candidates[0] = pressSnap;
    if (isLoopActive())
    {
        const qreal startPosition = pressOffset + wrapDelta(_snaps.at(pressSnap) - pressOffset, Nearest);
        candidates[1] = (pressSnap + 1) % snapCount;
        candidates[2] = (pressSnap - 1 + snapCount) % snapCount;
        positions[0] = startPosition;
        positions[1] = startPosition + wrapDelta(_snaps.at(candidates[1]) - _snaps.at(pressSnap), Forward);
        positions[2] = startPosition + wrapDelta(_snaps.at(candidates[2]) - _snaps.at(pressSnap), Backward);
    }
    else
    {
        candidates[1] = qMin(pressSnap + 1, snapCount - 1);
        candidates[2] = qMax(pressSnap - 1, 0);
        for (int index = 0; index < 3; ++index)
        {
            positions[index] = _snaps.at(candidates[index]);
        }
    }
    int chosen = 0;
    for (int index = 1; index < 3; ++index)
    {
        if (qAbs(positions[index] - projected) < qAbs(positions[chosen] - projected))
        {
            chosen = index;
        }
    }
    setCurrent(candidates[chosen]);
    animateTo(positions[chosen]);
    restartAutoplay();
}

void ShadCarouselPrivate::resetPointer()
{
    _isPointerDown = false;
    _isDragging = false;
    _pressWidget = nullptr;
    _samples.clear();
}
