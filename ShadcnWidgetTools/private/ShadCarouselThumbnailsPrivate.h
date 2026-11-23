#ifndef SHADCAROUSELTHUMBNAILSPRIVATE_H
#define SHADCAROUSELTHUMBNAILSPRIVATE_H

#include <QList>
#include <QMetaObject>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QRectF>
#include <QtGlobal>

#include "ShadCarousel.h"
#include "ShadProperty.h"

class QVariantAnimation;
class ShadButton;
class ShadCarouselThumbnails;

class ShadCarouselThumbnailsPrivate
{
    SHAD_D_CREATE(ShadCarouselThumbnails)

    public:
        ShadCarouselThumbnailsPrivate();
        ~ShadCarouselThumbnailsPrivate();

        static constexpr int kDefaultThumbnailSize = 64;
        static constexpr int kDefaultSpacing = 8;
        static constexpr int kRingWidth = 2;
        static constexpr int kPadding = 2 * kRingWidth;
        static constexpr int kNavigationGap = 8;
        static constexpr int kPlaceholderIconSize = 20;
        static constexpr int kScrollDurationMs = 200;
        static constexpr qreal kHoverOpacity = 0.85;
        static constexpr qreal kPressedScale = 0.95;

        int currentIndex() const;
        int navigationExtent() const;
        QRectF stripRect() const;
        qreal contentWidth() const;
        qreal maxScroll() const;
        QRectF logicalThumbnailRect(int index) const;
        QRectF thumbnailRect(int index) const;
        int thumbnailIndexAt(const QPointF& pos) const;

        void bind(ShadCarousel* carousel);
        void unbind();
        void layoutButtons();
        void updateButtons();
        void setScroll(qreal scroll);
        void ensureCurrentVisible(bool animated);
        void setHoveredIndex(int index);
        void refresh();

        QPointer<ShadCarousel> _carousel;
        QList<QMetaObject::Connection> _connections;
        QList<QPixmap> _pixmaps;
        ShadButton* _prevButton{nullptr};
        ShadButton* _nextButton{nullptr};
        QVariantAnimation* _scrollAnimation{nullptr};
        qreal _scroll{0.0};
        int _thumbnailSize{kDefaultThumbnailSize};
        int _spacing{kDefaultSpacing};
        int _hoveredIndex{-1};
        int _pressedIndex{-1};
        bool _isNavigationVisible{false};
        bool _isFocusVisible{false};
};

#endif // SHADCAROUSELTHUMBNAILSPRIVATE_H
