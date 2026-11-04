#include "ShadDrawerPrivate.h"

#include <QApplication>
#include <QEvent>
#include <QList>
#include <QMetaObject>
#include <QMouseEvent>
#include <QObject>
#include <QPainterPath>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <algorithm>

#include "ShadDrawer.h"
#include "ShadDrawerOverlay.h"
#include "ShadFrameAnimation.h"
#include "ShadLabel.h"
#include "ShadStyleHelper.h"
#include "ShadWindowCorner.h"

quint64 ShadDrawerPrivate::s_openSerial = 0;

ShadDrawerPrivate::ShadDrawerPrivate()
{
}

ShadDrawerPrivate::~ShadDrawerPrivate()
{
}

bool ShadDrawerPrivate::isVertical() const
{
    return _direction == ShadDrawerType::Up || _direction == ShadDrawerType::Down;
}

bool ShadDrawerPrivate::hasSnapPoints() const
{
    return isVertical() && !_snapPoints.isEmpty();
}

QWidget* ShadDrawerPrivate::resolveHost() const
{
    Q_Q(const ShadDrawer);
    QWidget* anchor = !_anchor.isNull() ? _anchor.data() : q->parentWidget();
    return anchor != nullptr ? anchor->window() : nullptr;
}

QList<ShadDrawer*> ShadDrawerPrivate::visibleStack(QWidget* host)
{
    QList<ShadDrawer*> drawers;
    if (host == nullptr)
    {
        return drawers;
    }
    for (ShadDrawer* drawer : host->findChildren<ShadDrawer*>(QString(), Qt::FindDirectChildrenOnly))
    {
        if (!drawer->isHidden())
        {
            drawers.append(drawer);
        }
    }
    std::sort(drawers.begin(), drawers.end(), [](ShadDrawer* left, ShadDrawer* right) { return left->d_func()->_openSerial < right->d_func()->_openSerial; });
    return drawers;
}

ShadDrawerOverlay* ShadDrawerPrivate::findOverlay(QWidget* host)
{
    return host != nullptr ? host->findChild<ShadDrawerOverlay*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
}

void ShadDrawerPrivate::restack(QWidget* host)
{
    if (host == nullptr)
    {
        return;
    }
    if (ShadDrawerOverlay* overlay = findOverlay(host))
    {
        overlay->raise();
    }
    for (ShadDrawer* drawer : visibleStack(host))
    {
        drawer->raise();
    }
    for (ShadWindowCorner* corner : host->findChildren<ShadWindowCorner*>(QString(), Qt::FindDirectChildrenOnly))
    {
        corner->raise();
    }
}

bool ShadDrawerPrivate::isTopmost() const
{
    Q_Q(const ShadDrawer);
    const QList<ShadDrawer*> stack = visibleStack(_host);
    for (auto it = stack.crbegin(); it != stack.crend(); ++it)
    {
        if ((*it)->d_func()->_isOpen)
        {
            return *it == q;
        }
    }
    return false;
}

void ShadDrawerPrivate::attachHost(QWidget* host)
{
    Q_Q(ShadDrawer);
    if (q->parentWidget() != host)
    {
        q->setParent(host);
    }
    _host = host;
    qApp->installEventFilter(this);
}

void ShadDrawerPrivate::detachHost()
{
    Q_Q(ShadDrawer);
    qApp->removeEventFilter(this);
    if (_overlayConnection)
    {
        QObject::disconnect(_overlayConnection);
        _overlayConnection = QMetaObject::Connection();
    }
    if (_host.isNull())
    {
        return;
    }
    bool hasModal = false;
    for (ShadDrawer* drawer : visibleStack(_host))
    {
        if (drawer != q && drawer->d_func()->_isModal)
        {
            hasModal = true;
            break;
        }
    }
    ShadDrawerOverlay* overlay = findOverlay(_host);
    if (!hasModal && overlay != nullptr)
    {
        // 处于遮罩信号中，延迟删除
        overlay->hide();
        overlay->setParent(nullptr);
        overlay->deleteLater();
    }
    _host = nullptr;
}

void ShadDrawerPrivate::updateHeader()
{
    const bool hasTitle = !_titleLabel->text().isEmpty();
    const bool hasDescription = !_descriptionLabel->text().isEmpty();
    _titleLabel->setVisible(hasTitle);
    _descriptionLabel->setVisible(hasDescription);
    _header->setVisible(hasTitle || hasDescription);
}

void ShadDrawerPrivate::updateHeaderStyle()
{
    const bool isWide = !_host.isNull() && _host->width() >= kWideBreakpoint;
    const Qt::Alignment alignment = (isVertical() && !isWide ? Qt::AlignHCenter : Qt::AlignLeft) | Qt::AlignTop;
    _titleLabel->setAlignment(alignment);
    _descriptionLabel->setAlignment(alignment);
    _headerLayout->setSpacing(isWide ? kHeaderGap : kNarrowHeaderGap);
}

void ShadDrawerPrivate::updateExtent()
{
    if (_host.isNull())
    {
        return;
    }
    const int hostWidth = _host->width();
    const int hostHeight = _host->height();
    if (!isVertical())
    {
        if (_preferredSize > 0)
        {
            _extent = qMin(_preferredSize, hostWidth);
        }
        else
        {
            _extent = hostWidth >= kSideBreakpoint ? kSideWidth : hostWidth * kSideRatio;
        }
        return;
    }
    if (hasSnapPoints())
    {
        _extent = hostHeight;
        return;
    }
    const qreal maximum = qMax(0, hostHeight - kVerticalMargin);
    if (_preferredSize > 0)
    {
        _extent = qMin<qreal>(_preferredSize, maximum);
        return;
    }
    int bodyHeight = _bodyLayout->hasHeightForWidth() ? _bodyLayout->totalHeightForWidth(hostWidth) : _bodyLayout->totalSizeHint().height();
    bodyHeight = qMax(bodyHeight, _bodyLayout->totalMinimumSize().height());
    const int chrome = kBorderWidth + (_isHandleVisible ? kHandleArea : 0);
    _extent = qMin<qreal>(bodyHeight + chrome, maximum);
}

qreal ShadDrawerPrivate::closedOffset() const
{
    return _extent + kClosedGap;
}

qreal ShadDrawerPrivate::snapOffset(qreal snapPoint) const
{
    const qreal hostHeight = _host.isNull() ? 0.0 : _host->height();
    const qreal visible = snapPoint <= 1.0 ? snapPoint * hostHeight : snapPoint;
    return hostHeight - qBound(0.0, visible, hostHeight);
}

qreal ShadDrawerPrivate::restingOffset() const
{
    return hasSnapPoints() ? snapOffset(_snapPoint) : 0.0;
}

qreal ShadDrawerPrivate::minimumOffset() const
{
    if (!hasSnapPoints())
    {
        return 0.0;
    }
    qreal minimum = closedOffset();
    for (qreal point : _snapPoints)
    {
        minimum = qMin(minimum, snapOffset(point));
    }
    return minimum;
}

qreal ShadDrawerPrivate::openness() const
{
    const qreal closed = closedOffset();
    return closed > 0.0 ? qBound(0.0, 1.0 - _offset / closed, 1.0) : 0.0;
}

qreal ShadDrawerPrivate::overlayLevel() const
{
    if (_isOpen)
    {
        const qreal minimum = hasSnapPoints() ? kSnapOverlayMinLevel * _appear : 0.0;
        return qMax(minimum, openness());
    }
    const qreal closed = closedOffset();
    const qreal span = closed - _closeStartOffset;
    const qreal fraction = span > 0.0 ? qBound(0.0, (closed - _offset) / span, 1.0) : 0.0;
    return _closeStartLevel * fraction;
}

void ShadDrawerPrivate::place()
{
    Q_Q(ShadDrawer);
    if (_host.isNull())
    {
        return;
    }
    const qreal hostWidth = _host->width();
    const qreal hostHeight = _host->height();
    const qreal level = _stackLevel;
    const qreal amount = qMin<qreal>(level, 1.0);
    const qreal scale = qBound(0.0, 1.0 - kStackStep * level, 1.0);
    const qreal peek = level * kStackPeek;
    const qreal height = _extent + (_stackExtent - _extent) * amount;
    QRectF panel;
    switch (_direction)
    {
        case ShadDrawerType::Up:
            {
                const qreal top = -_offset + peek + (1.0 - scale) * _stackExtent;
                panel = QRectF((hostWidth - hostWidth * scale) / 2.0, top, hostWidth * scale, height * scale);
                break;
            }
        case ShadDrawerType::Right:
            {
                const qreal right = hostWidth + _offset - peek - (1.0 - scale) * _extent;
                panel = QRectF(right - _extent * scale, (hostHeight - hostHeight * scale) / 2.0, _extent * scale, hostHeight * scale);
                break;
            }
        case ShadDrawerType::Down:
            {
                const qreal bottom = hostHeight + _offset - peek - (1.0 - scale) * _stackExtent;
                panel = QRectF((hostWidth - hostWidth * scale) / 2.0, bottom - height * scale, hostWidth * scale, height * scale);
                break;
            }
        case ShadDrawerType::Left:
            {
                const qreal left = -_offset + peek + (1.0 - scale) * _extent;
                panel = QRectF(left, (hostHeight - hostHeight * scale) / 2.0, _extent * scale, hostHeight * scale);
                break;
            }
    }
    _panelScale = scale;
    const QRect panelRect(qRound(panel.left()), qRound(panel.top()), qRound(panel.width()), qRound(panel.height()));
    QRect widgetRect = panelRect;
    if (level <= 0.0)
    {
        switch (_direction)
        {
            case ShadDrawerType::Up:
                widgetRect.setTop(widgetRect.top() - kBleed);
                break;
            case ShadDrawerType::Right:
                widgetRect.setRight(widgetRect.right() + kBleed);
                break;
            case ShadDrawerType::Down:
                widgetRect.setBottom(widgetRect.bottom() + kBleed);
                break;
            case ShadDrawerType::Left:
                widgetRect.setLeft(widgetRect.left() - kBleed);
                break;
        }
    }
    if (q->geometry() != widgetRect)
    {
        q->setGeometry(widgetRect);
    }
    _panelRect = panelRect.translated(-widgetRect.topLeft());
    if (level <= 0.0)
    {
        QRect bodyRect = _panelRect;
        const int inner = kBorderWidth + (_isHandleVisible ? kHandleArea : 0);
        switch (_direction)
        {
            case ShadDrawerType::Up:
                bodyRect.setBottom(bodyRect.bottom() - inner);
                break;
            case ShadDrawerType::Right:
                bodyRect.setLeft(bodyRect.left() + inner);
                break;
            case ShadDrawerType::Down:
                bodyRect.setTop(bodyRect.top() + inner);
                break;
            case ShadDrawerType::Left:
                bodyRect.setRight(bodyRect.right() - inner);
                break;
        }
        if (_body->geometry() != bodyRect)
        {
            _body->setGeometry(bodyRect);
        }
    }
    q->update();
}

void ShadDrawerPrivate::refresh()
{
    place();
    if (!_host.isNull())
    {
        syncStack(_host);
        syncOverlay(_host);
    }
}

void ShadDrawerPrivate::syncStack(QWidget* host)
{
    const QList<ShadDrawer*> stack = visibleStack(host);
    if (stack.isEmpty())
    {
        return;
    }
    const qreal frontExtent = stack.last()->d_func()->_extent;
    for (int index = 0; index < stack.size(); ++index)
    {
        qreal level = 0.0;
        for (int above = index + 1; above < stack.size(); ++above)
        {
            level += stack.at(above)->d_func()->openness();
        }
        stack.at(index)->d_func()->setStack(level, frontExtent);
    }
}

void ShadDrawerPrivate::syncOverlay(QWidget* host)
{
    ShadDrawerOverlay* overlay = findOverlay(host);
    if (overlay == nullptr)
    {
        return;
    }
    qreal level = 0.0;
    // 由最下层的模态抽屉决定
    for (ShadDrawer* drawer : visibleStack(host))
    {
        if (drawer->d_func()->_isModal)
        {
            level = drawer->d_func()->overlayLevel();
            break;
        }
    }
    overlay->setLevel(level);
}

void ShadDrawerPrivate::setStack(qreal level, qreal frontExtent)
{
    Q_Q(ShadDrawer);
    const qreal stackLevel = level > 0.001 ? level : 0.0;
    if (qFuzzyCompare(stackLevel + 1.0, _stackLevel + 1.0) && qFuzzyCompare(frontExtent + 1.0, _stackExtent + 1.0))
    {
        return;
    }
    const bool wasStacked = _stackLevel > 0.0;
    const bool isStacked = stackLevel > 0.0;
    if (isStacked && !wasStacked)
    {
        _snapshotRect = QRectF(QPointF(_body->geometry().topLeft() - _panelRect.topLeft()), QSizeF(_body->size()));
        _bodySnapshot = ShadStyleHelper::grabContent(_body);
        _body->hide();
        q->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }
    else if (!isStacked && wasStacked)
    {
        _bodySnapshot = QPixmap();
        _body->show();
        q->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    }
    _stackLevel = stackLevel;
    _stackExtent = frontExtent;
    place();
}

void ShadDrawerPrivate::animateTo(qreal offset, int duration)
{
    _animationStart = _offset;
    _animationEnd = offset;
    _animation->setStartValue(_offset);
    _animation->setEndValue(offset);
    _animation->setDuration(duration);
    _animation->start();
}

void ShadDrawerPrivate::requestClose()
{
    Q_Q(ShadDrawer);
    if (!_isOpen)
    {
        return;
    }
    const QList<ShadDrawer*> stack = visibleStack(_host);
    const int index = static_cast<int>(stack.indexOf(q));
    for (int above = static_cast<int>(stack.size()) - 1; above > index; --above)
    {
        stack.at(above)->d_func()->requestClose();
    }
    _closeStartLevel = overlayLevel();
    _closeStartOffset = _offset;
    _isOpen = false;
    _isOpening = false;
    _isClosing = true;
    _isPressed = false;
    _isSwiping = false;
    const qreal closed = closedOffset();
    const qreal strength = closed > 0.0 ? qBound(kMinCloseStrength, (closed - _offset) / closed, 1.0) : 1.0;
    animateTo(closed, qRound(kCloseDurationMs * strength));
    Q_EMIT q->openChanged(false);
}

void ShadDrawerPrivate::finishClose()
{
    Q_Q(ShadDrawer);
    _isClosing = false;
    QWidget* focus = QApplication::focusWidget();
    const bool hadFocus = focus != nullptr && (focus == q || q->isAncestorOf(focus));
    q->hide();
    _stackLevel = 0.0;
    _bodySnapshot = QPixmap();
    _body->show();
    q->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    if (!_snapPoints.isEmpty())
    {
        setCurrentSnapPoint(_snapPoints.first());
    }
    QWidget* host = _host.data();
    detachHost();
    // 先恢复下层内容，焦点才能回去
    if (host != nullptr)
    {
        syncStack(host);
        syncOverlay(host);
    }
    if (hadFocus && !_focusBeforeOpen.isNull())
    {
        _focusBeforeOpen->setFocus(Qt::OtherFocusReason);
    }
    _focusBeforeOpen = nullptr;
    Q_EMIT q->openChangeComplete(false);
}

void ShadDrawerPrivate::settle()
{
    if (_clock.elapsed() - _lastTime > kVelocityTimeoutMs)
    {
        _velocity = 0.0;
    }
    if (!hasSnapPoints())
    {
        if (_offset > _extent * kDismissRatio || _velocity > kDismissVelocity)
        {
            requestClose();
            return;
        }
        animateTo(0.0, kOpenDurationMs);
        return;
    }
    struct Candidate
    {
        qreal offset{0.0};
        qreal point{0.0};
        bool isClosed{false};
    };
    QList<Candidate> candidates;
    for (qreal point : _snapPoints)
    {
        candidates.append({snapOffset(point), point, false});
    }
    candidates.append({closedOffset(), 0.0, true});
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) { return left.offset < right.offset; });
    const auto nearest = [&candidates](qreal offset, int lower, int upper)
    {
        int best = lower;
        for (int index = lower; index <= upper; ++index)
        {
            if (qAbs(candidates.at(index).offset - offset) < qAbs(candidates.at(best).offset - offset))
            {
                best = index;
            }
        }
        return best;
    };
    const int last = static_cast<int>(candidates.size()) - 1;
    int lower = 0;
    int upper = last;
    if (_isSequentialSnap)
    {
        const int start = nearest(_pressOffset, 0, last);
        lower = qMax(0, start - 1);
        upper = qMin(last, start + 1);
    }
    const Candidate& target = candidates.at(nearest(_offset + _velocity * kProjectionMs, lower, upper));
    if (target.isClosed)
    {
        requestClose();
        return;
    }
    setCurrentSnapPoint(target.point);
    animateTo(target.offset, kOpenDurationMs);
}

void ShadDrawerPrivate::setCurrentSnapPoint(qreal snapPoint)
{
    Q_Q(ShadDrawer);
    if (qFuzzyCompare(snapPoint, _snapPoint))
    {
        return;
    }
    _snapPoint = snapPoint;
    Q_EMIT q->snapPointChanged(snapPoint);
}

void ShadDrawerPrivate::focusFirstChild()
{
    Q_Q(ShadDrawer);
    const QList<QWidget*> widgets = focusableChildren();
    if (!widgets.isEmpty())
    {
        widgets.first()->setFocus(Qt::PopupFocusReason);
        return;
    }
    // 用于响应 Esc
    q->setFocus(Qt::PopupFocusReason);
}

QList<QWidget*> ShadDrawerPrivate::focusableChildren() const
{
    Q_Q(const ShadDrawer);
    QList<QWidget*> widgets;
    QWidget* candidate = q->nextInFocusChain();
    while (candidate != nullptr && candidate != q)
    {
        const bool isTabFocus = (candidate->focusPolicy() & Qt::TabFocus) == Qt::TabFocus;
        if (q->isAncestorOf(candidate) && candidate->isVisibleTo(q) && candidate->isEnabled() && isTabFocus)
        {
            widgets.append(candidate);
        }
        candidate = candidate->nextInFocusChain();
    }
    return widgets;
}

QPainterPath ShadDrawerPrivate::panelPath(const QRectF& panel, qreal radius) const
{
    const qreal corner = qMin(radius, qMin(panel.width(), panel.height()) / 2.0);
    QRectF extended = panel;
    switch (_direction)
    {
        case ShadDrawerType::Up:
            extended.setTop(panel.top() - corner);
            break;
        case ShadDrawerType::Right:
            extended.setRight(panel.right() + corner);
            break;
        case ShadDrawerType::Down:
            extended.setBottom(panel.bottom() + corner);
            break;
        case ShadDrawerType::Left:
            extended.setLeft(panel.left() - corner);
            break;
    }
    QPainterPath rounded;
    rounded.addRoundedRect(extended, corner, corner);
    QPainterPath box;
    box.addRect(panel);
    return rounded.intersected(box);
}

QPainterPath ShadDrawerPrivate::innerEdgePath(const QRectF& panel, qreal radius) const
{
    const QRectF edge = panel.adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal corner = qMax(0.0, qMin(radius, qMin(panel.width(), panel.height()) / 2.0) - 0.5);
    const qreal diameter = 2.0 * corner;
    QPainterPath path;
    switch (_direction)
    {
        case ShadDrawerType::Up:
            path.moveTo(edge.left(), edge.bottom() - corner);
            path.arcTo(QRectF(edge.left(), edge.bottom() - diameter, diameter, diameter), 180.0, 90.0);
            path.lineTo(edge.right() - corner, edge.bottom());
            path.arcTo(QRectF(edge.right() - diameter, edge.bottom() - diameter, diameter, diameter), 270.0, 90.0);
            break;
        case ShadDrawerType::Right:
            path.moveTo(edge.left() + corner, edge.top());
            path.arcTo(QRectF(edge.left(), edge.top(), diameter, diameter), 90.0, 90.0);
            path.lineTo(edge.left(), edge.bottom() - corner);
            path.arcTo(QRectF(edge.left(), edge.bottom() - diameter, diameter, diameter), 180.0, 90.0);
            break;
        case ShadDrawerType::Down:
            path.moveTo(edge.left(), edge.top() + corner);
            path.arcTo(QRectF(edge.left(), edge.top(), diameter, diameter), 180.0, -90.0);
            path.lineTo(edge.right() - corner, edge.top());
            path.arcTo(QRectF(edge.right() - diameter, edge.top(), diameter, diameter), 90.0, -90.0);
            break;
        case ShadDrawerType::Left:
            path.moveTo(edge.right() - corner, edge.top());
            path.arcTo(QRectF(edge.right() - diameter, edge.top(), diameter, diameter), 90.0, -90.0);
            path.lineTo(edge.right(), edge.bottom() - corner);
            path.arcTo(QRectF(edge.right() - diameter, edge.bottom() - diameter, diameter, diameter), 0.0, -90.0);
            break;
    }
    return path;
}

bool ShadDrawerPrivate::eventFilter(QObject* watched, QEvent* event)
{
    Q_Q(ShadDrawer);
    if (!_host.isNull() && watched == _host.data())
    {
        if (event->type() == QEvent::Resize)
        {
            updateHeaderStyle();
            updateExtent();
            if (!_isSwiping && !_isClosing)
            {
                if (_animation->isRunning())
                {
                    animateTo(restingOffset(), kOpenDurationMs);
                }
                else
                {
                    _offset = restingOffset();
                }
            }
            refresh();
        }
        else if (event->type() == QEvent::ChildAdded)
        {
            // 添加过程中不能调整顺序
            QMetaObject::invokeMethod(this, [this]() { restack(_host); }, Qt::QueuedConnection);
        }
        return QObject::eventFilter(watched, event);
    }
    if (watched == _body)
    {
        if (event->type() == QEvent::LayoutRequest && !q->isHidden() && _stackLevel <= 0.0)
        {
            updateExtent();
            refresh();
        }
        return QObject::eventFilter(watched, event);
    }
    const bool isPress = event->type() == QEvent::MouseButtonPress;
    const bool isFocusIn = event->type() == QEvent::FocusIn;
    if ((!isPress && !isFocusIn) || !_isOpen || !watched->isWidgetType())
    {
        return QObject::eventFilter(watched, event);
    }
    QWidget* widget = static_cast<QWidget*>(watched);
    if (widget->window() != _host || !isTopmost())
    {
        return QObject::eventFilter(watched, event);
    }
    if (isPress && !_isModal && _isPointerDismissal)
    {
        const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (!_panelRect.contains(q->mapFromGlobal(mouseEvent->globalPosition().toPoint())))
        {
            _outsideDismissClock.start();
            requestClose();
        }
    }
    else if (isFocusIn && _isModal && widget != q && !q->isAncestorOf(widget))
    {
        QMetaObject::invokeMethod(this, [this]()
        {
            if (_isOpen)
            {
                focusFirstChild();
            }
        }, Qt::QueuedConnection);
    }
    return QObject::eventFilter(watched, event);
}
