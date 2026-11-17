#include "ShadAnchorPrivate.h"

#include <QAbstractAnimation>
#include <QAbstractScrollArea>
#include <QObject>
#include <QScrollBar>
#include <QVariantAnimation>
#include <QtGlobal>

#include <limits>
#include <utility>

#include "ShadAnchor.h"
#include "ShadStyleHelper.h"

ShadAnchorPrivate::ShadAnchorPrivate()
{
}

ShadAnchorPrivate::~ShadAnchorPrivate()
{
}

QFont ShadAnchorPrivate::textFont() const
{
    Q_Q(const ShadAnchor);
    return q->font();
}

QRectF ShadAnchorPrivate::rowRect(int index) const
{
    Q_Q(const ShadAnchor);
    const int margin = ShadStyleHelper::kFocusRingWidth;
    return QRectF(margin, margin + index * kRowHeight, q->width() - 2 * margin, kRowHeight);
}

QRectF ShadAnchorPrivate::textRect(int index) const
{
    Q_Q(const ShadAnchor);
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const qreal left = margin + kRailWidth + kTextPadding + qMax(0, _links.at(index).level) * kLevelIndent;
    const qreal right = q->width() - margin - kEndPadding;
    const QRectF row = rowRect(index);
    return QRectF(left, row.top(), qMax<qreal>(0.0, right - left), row.height());
}

int ShadAnchorPrivate::indexAt(const QPoint& pos) const
{
    Q_Q(const ShadAnchor);
    const int margin = ShadStyleHelper::kFocusRingWidth;
    if (pos.x() < 0 || pos.x() >= q->width() || pos.y() < margin)
    {
        return -1;
    }
    const int index = (pos.y() - margin) / kRowHeight;
    return index < _links.size() ? index : -1;
}

bool ShadAnchorPrivate::targetTop(int index, int* top) const
{
    if (_scrollArea.isNull() || index < 0 || index >= _links.size())
    {
        return false;
    }
    const QWidget* target = _links.at(index).target.data();
    const QWidget* viewport = _scrollArea->viewport();
    if (target == nullptr || !viewport->isAncestorOf(target) || !target->isVisibleTo(viewport))
    {
        return false;
    }
    *top = target->mapTo(viewport, QPoint(0, 0)).y();
    return true;
}

int ShadAnchorPrivate::computeSpyIndex() const
{
    if (_scrollArea.isNull() || _links.isEmpty())
    {
        return -1;
    }
    const QScrollBar* scrollBar = _scrollArea->verticalScrollBar();
    const bool isAtBottom = scrollBar->maximum() > scrollBar->minimum() && scrollBar->value() >= scrollBar->maximum();
    const int viewportHeight = _scrollArea->viewport()->height();
    const int threshold = _offset + 1;
    int passedIndex = -1;
    int passedTop = std::numeric_limits<int>::min();
    int firstIndex = -1;
    int firstTop = std::numeric_limits<int>::max();
    int bottomIndex = -1;
    int bottomTop = std::numeric_limits<int>::min();
    for (int index = 0; index < _links.size(); ++index)
    {
        int top = 0;
        if (!targetTop(index, &top))
        {
            continue;
        }
        if (top < firstTop)
        {
            firstTop = top;
            firstIndex = index;
        }
        if (top <= threshold && top > passedTop)
        {
            passedTop = top;
            passedIndex = index;
        }
        if (isAtBottom && top < viewportHeight && top > bottomTop)
        {
            bottomTop = top;
            bottomIndex = index;
        }
    }
    if (bottomIndex >= 0 && bottomTop > passedTop)
    {
        return bottomIndex;
    }
    return passedIndex >= 0 ? passedIndex : firstIndex;
}

void ShadAnchorPrivate::updateFromScroll()
{
    if (_scrollAnimation->state() == QAbstractAnimation::Running)
    {
        return;
    }
    setCurrent(computeSpyIndex());
}

void ShadAnchorPrivate::setCurrent(int index)
{
    Q_Q(ShadAnchor);
    if (_currentIndex == index)
    {
        return;
    }
    const int previousIndex = _currentIndex;
    _currentIndex = index;
    _indicatorAnimation->stop();
    if (index >= 0)
    {
        if (previousIndex < 0 || !q->isVisible())
        {
            _indicatorPosition = index;
        }
        else
        {
            _indicatorAnimation->setStartValue(_indicatorPosition);
            _indicatorAnimation->setEndValue(qreal(index));
            _indicatorAnimation->start();
        }
    }
    q->update();
    Q_EMIT q->currentIndexChanged(index);
}

void ShadAnchorPrivate::scrollToLink(int index)
{
    int top = 0;
    if (!targetTop(index, &top))
    {
        return;
    }
    QScrollBar* scrollBar = _scrollArea->verticalScrollBar();
    const int value = qBound(scrollBar->minimum(), scrollBar->value() + top - _offset, scrollBar->maximum());
    _scrollAnimation->stop();
    if (value == scrollBar->value())
    {
        return;
    }
    if (_durationMs <= 0 || !_scrollArea->isVisible())
    {
        _isAnimatingScroll = true;
        scrollBar->setValue(value);
        _isAnimatingScroll = false;
        return;
    }
    _scrollAnimation->setStartValue(scrollBar->value());
    _scrollAnimation->setEndValue(value);
    _scrollAnimation->setDuration(_durationMs);
    _scrollAnimation->start();
}

void ShadAnchorPrivate::activate(int index)
{
    Q_Q(ShadAnchor);
    if (index < 0 || index >= _links.size())
    {
        return;
    }
    _focusIndex = index;
    setCurrent(index);
    scrollToLink(index);
    Q_EMIT q->linkClicked(index);
}

void ShadAnchorPrivate::connectScrollArea()
{
    Q_Q(ShadAnchor);
    if (_scrollArea.isNull())
    {
        return;
    }
    _scrollArea->viewport()->installEventFilter(q);
    QScrollBar* scrollBar = _scrollArea->verticalScrollBar();
    _scrollConnections.append(QObject::connect(scrollBar, &QScrollBar::valueChanged, q, [this]()
    {
        if (_isAnimatingScroll)
        {
            return;
        }
        _scrollAnimation->stop();
        updateFromScroll();
    }));
    _scrollConnections.append(QObject::connect(scrollBar, &QScrollBar::rangeChanged, q, [this]() { updateFromScroll(); }));
}

void ShadAnchorPrivate::disconnectScrollArea()
{
    Q_Q(ShadAnchor);
    for (const QMetaObject::Connection& connection : std::as_const(_scrollConnections))
    {
        QObject::disconnect(connection);
    }
    _scrollConnections.clear();
    _scrollAnimation->stop();
    if (!_scrollArea.isNull())
    {
        _scrollArea->viewport()->removeEventFilter(q);
    }
}
