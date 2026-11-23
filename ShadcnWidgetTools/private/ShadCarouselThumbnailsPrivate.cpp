#include "ShadCarouselThumbnailsPrivate.h"

#include <QObject>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QVariantAnimation>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <utility>

#include "ShadButton.h"
#include "ShadCarouselThumbnails.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"

ShadCarouselThumbnailsPrivate::ShadCarouselThumbnailsPrivate()
{
}

ShadCarouselThumbnailsPrivate::~ShadCarouselThumbnailsPrivate()
{
}

int ShadCarouselThumbnailsPrivate::currentIndex() const
{
    return _carousel.isNull() ? -1 : _carousel->currentIndex();
}

int ShadCarouselThumbnailsPrivate::navigationExtent() const
{
    return _isNavigationVisible ? _prevButton->sizeHint().width() + kNavigationGap : 0;
}

QRectF ShadCarouselThumbnailsPrivate::stripRect() const
{
    Q_Q(const ShadCarouselThumbnails);
    const int extent = navigationExtent();
    return QRectF(extent, 0.0, qMax(0, q->width() - 2 * extent), q->height());
}

qreal ShadCarouselThumbnailsPrivate::contentWidth() const
{
    const int count = int(_pixmaps.size());
    return count == 0 ? 0.0 : count * _thumbnailSize + (count - 1) * _spacing;
}

qreal ShadCarouselThumbnailsPrivate::maxScroll() const
{
    return qMax<qreal>(0.0, contentWidth() + 2 * kPadding - stripRect().width());
}

QRectF ShadCarouselThumbnailsPrivate::logicalThumbnailRect(int index) const
{
    Q_Q(const ShadCarouselThumbnails);
    const QRectF strip = stripRect();
    const qreal overflow = maxScroll();
    const qreal start = overflow > 0.0 ? strip.left() + kPadding - _scroll : strip.left() + (strip.width() - contentWidth()) / 2.0;
    return QRectF(start + index * (_thumbnailSize + _spacing), (q->height() - _thumbnailSize) / 2.0, _thumbnailSize, _thumbnailSize);
}

QRectF ShadCarouselThumbnailsPrivate::thumbnailRect(int index) const
{
    Q_Q(const ShadCarouselThumbnails);
    return ShadStyleHelper::visualRect(q->layoutDirection(), QRectF(q->rect()), logicalThumbnailRect(index));
}

int ShadCarouselThumbnailsPrivate::thumbnailIndexAt(const QPointF& pos) const
{
    Q_Q(const ShadCarouselThumbnails);
    if (!ShadStyleHelper::visualRect(q->layoutDirection(), QRectF(q->rect()), stripRect()).contains(pos))
    {
        return -1;
    }
    for (int index = 0; index < int(_pixmaps.size()); ++index)
    {
        if (thumbnailRect(index).contains(pos))
        {
            return index;
        }
    }
    return -1;
}

void ShadCarouselThumbnailsPrivate::bind(ShadCarousel* carousel)
{
    Q_Q(ShadCarouselThumbnails);
    _carousel = carousel;
    if (carousel == nullptr)
    {
        return;
    }
    _connections.append(QObject::connect(carousel, &ShadCarousel::currentChanged, q, [this, q]()
    {
        updateButtons();
        ensureCurrentVisible(true);
        q->update();
    }));
    _connections.append(QObject::connect(carousel, &ShadCarousel::snapCountChanged, q, [this]() { updateButtons(); }));
    _connections.append(QObject::connect(carousel, &QObject::destroyed, q, [this, q]()
    {
        updateButtons();
        q->update();
    }));
}

void ShadCarouselThumbnailsPrivate::unbind()
{
    for (const QMetaObject::Connection& connection : std::as_const(_connections))
    {
        QObject::disconnect(connection);
    }
    _connections.clear();
    _carousel = nullptr;
}

void ShadCarouselThumbnailsPrivate::layoutButtons()
{
    Q_Q(ShadCarouselThumbnails);
    const QSize buttonSize = _prevButton->sizeHint();
    const int y = (q->height() - buttonSize.height()) / 2;
    _prevButton->setGeometry(ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), QRect(QPoint(0, y), buttonSize)));
    _nextButton->setGeometry(ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), QRect(QPoint(q->width() - buttonSize.width(), y), buttonSize)));
    if (!_isNavigationVisible)
    {
        ShadStyleHelper::clearFocusWithin(_prevButton);
        ShadStyleHelper::clearFocusWithin(_nextButton);
    }
    _prevButton->setVisible(_isNavigationVisible);
    _nextButton->setVisible(_isNavigationVisible);
}

void ShadCarouselThumbnailsPrivate::updateButtons()
{
    Q_Q(ShadCarouselThumbnails);
    const bool isRightToLeft = q->layoutDirection() == Qt::RightToLeft;
    _prevButton->setShadIcon(isRightToLeft ? ShadIconType::ChevronRight : ShadIconType::ChevronLeft);
    _nextButton->setShadIcon(isRightToLeft ? ShadIconType::ChevronLeft : ShadIconType::ChevronRight);
    const bool canScrollPrev = !_carousel.isNull() && _carousel->canScrollPrev();
    const bool canScrollNext = !_carousel.isNull() && _carousel->canScrollNext();
    ShadStyleHelper::setNavigationButtonsEnabled(_prevButton, canScrollPrev, _nextButton, canScrollNext);
}

void ShadCarouselThumbnailsPrivate::setScroll(qreal scroll)
{
    Q_Q(ShadCarouselThumbnails);
    const qreal bounded = qBound<qreal>(0.0, scroll, maxScroll());
    if (qFuzzyCompare(_scroll + 1.0, bounded + 1.0))
    {
        return;
    }
    _scroll = bounded;
    q->update();
}

void ShadCarouselThumbnailsPrivate::ensureCurrentVisible(bool animated)
{
    Q_Q(ShadCarouselThumbnails);
    _scrollAnimation->stop();
    const int index = currentIndex();
    const qreal overflow = maxScroll();
    if (index < 0 || index >= _pixmaps.size() || overflow <= 0.0)
    {
        setScroll(qMin(_scroll, overflow));
        return;
    }
    const qreal center = kPadding + index * (_thumbnailSize + _spacing) + _thumbnailSize / 2.0;
    const qreal target = qBound<qreal>(0.0, center - stripRect().width() / 2.0, overflow);
    if (!animated || !q->isVisible())
    {
        setScroll(target);
        return;
    }
    _scrollAnimation->setStartValue(_scroll);
    _scrollAnimation->setEndValue(target);
    _scrollAnimation->start();
}

void ShadCarouselThumbnailsPrivate::setHoveredIndex(int index)
{
    Q_Q(ShadCarouselThumbnails);
    if (_hoveredIndex == index)
    {
        return;
    }
    _hoveredIndex = index;
    q->setCursor(index >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
    q->update();
}

void ShadCarouselThumbnailsPrivate::refresh()
{
    Q_Q(ShadCarouselThumbnails);
    _hoveredIndex = -1;
    _pressedIndex = -1;
    layoutButtons();
    updateButtons();
    ensureCurrentVisible(false);
    q->updateGeometry();
    q->update();
}
