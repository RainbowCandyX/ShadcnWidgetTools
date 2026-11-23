#include "ShadCarouselDotsPrivate.h"

#include <QObject>
#include <QVariantAnimation>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <utility>

#include "ShadCarouselDots.h"

ShadCarouselDotsPrivate::ShadCarouselDotsPrivate()
{
}

ShadCarouselDotsPrivate::~ShadCarouselDotsPrivate()
{
}

int ShadCarouselDotsPrivate::dotCount() const
{
    return _carousel.isNull() ? 0 : _carousel->snapCount();
}

int ShadCarouselDotsPrivate::currentIndex() const
{
    return _carousel.isNull() ? -1 : _carousel->currentIndex();
}

qreal ShadCarouselDotsPrivate::weight(int index) const
{
    return qMax<qreal>(0.0, 1.0 - qAbs(index - _position));
}

qreal ShadCarouselDotsPrivate::contentWidth(int count) const
{
    const int dots = qMax(1, count);
    return dots * _dotSize + kActiveExtent + (dots - 1) * _spacing;
}

QRectF ShadCarouselDotsPrivate::contentRect() const
{
    Q_Q(const ShadCarouselDots);
    const qreal width = contentWidth(dotCount());
    return QRectF((q->width() - width) / 2.0, (q->height() - _dotSize) / 2.0, width, _dotSize);
}

QRectF ShadCarouselDotsPrivate::dotRect(int index) const
{
    Q_Q(const ShadCarouselDots);
    const QRectF content = contentRect();
    qreal x = content.left();
    for (int current = 0; current < index; ++current)
    {
        x += _dotSize + kActiveExtent * weight(current) + _spacing;
    }
    const QRectF logicalRect(x, content.top(), _dotSize + kActiveExtent * weight(index), _dotSize);
    return ShadStyleHelper::visualRect(q->layoutDirection(), QRectF(q->rect()), logicalRect);
}

int ShadCarouselDotsPrivate::dotIndexAt(const QPointF& pos) const
{
    Q_Q(const ShadCarouselDots);
    const int count = dotCount();
    if (count <= 1)
    {
        return -1;
    }
    const qreal halfSpacing = _spacing / 2.0;
    for (int index = 0; index < count; ++index)
    {
        const QRectF rect = dotRect(index);
        if (QRectF(rect.left() - halfSpacing, 0.0, rect.width() + _spacing, q->height()).contains(pos))
        {
            return index;
        }
    }
    return -1;
}

void ShadCarouselDotsPrivate::bind(ShadCarousel* carousel)
{
    Q_Q(ShadCarouselDots);
    _carousel = carousel;
    if (carousel == nullptr)
    {
        return;
    }
    _connections.append(QObject::connect(carousel, &ShadCarousel::currentChanged, q, [this]() { syncPosition(true); }));
    _connections.append(QObject::connect(carousel, &ShadCarousel::snapCountChanged, q, [this, q]()
    {
        syncPosition(false);
        q->updateGeometry();
    }));
    _connections.append(QObject::connect(carousel, &QObject::destroyed, q, [q]()
    {
        q->updateGeometry();
        q->update();
    }));
}

void ShadCarouselDotsPrivate::unbind()
{
    for (const QMetaObject::Connection& connection : std::as_const(_connections))
    {
        QObject::disconnect(connection);
    }
    _connections.clear();
    _carousel = nullptr;
}

void ShadCarouselDotsPrivate::syncPosition(bool animated)
{
    Q_Q(ShadCarouselDots);
    const qreal target = qMax(0, currentIndex());
    _animation->stop();
    if (!animated || !q->isVisible())
    {
        _position = target;
        q->update();
        return;
    }
    _animation->setStartValue(_position);
    _animation->setEndValue(target);
    _animation->start();
}

void ShadCarouselDotsPrivate::setHoveredIndex(int index)
{
    Q_Q(ShadCarouselDots);
    if (_hoveredIndex == index)
    {
        return;
    }
    _hoveredIndex = index;
    q->setCursor(index >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
    q->update();
}
