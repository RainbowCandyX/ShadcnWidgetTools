#ifndef SHADCAROUSELDOTSPRIVATE_H
#define SHADCAROUSELDOTSPRIVATE_H

#include <QList>
#include <QMetaObject>
#include <QPointF>
#include <QPointer>
#include <QRectF>
#include <QtGlobal>

#include "ShadCarousel.h"
#include "ShadProperty.h"
#include "ShadStyleHelper.h"

class QVariantAnimation;
class ShadCarouselDots;

class ShadCarouselDotsPrivate
{
    SHAD_D_CREATE(ShadCarouselDots)

    public:
        ShadCarouselDotsPrivate();
        ~ShadCarouselDotsPrivate();

        static constexpr int kDefaultDotSize = 8;
        static constexpr int kDefaultSpacing = 8;
        static constexpr int kActiveExtent = 12;
        static constexpr int kPadding = ShadStyleHelper::kFocusRingWidth + 3;
        static constexpr int kAnimationDurationMs = 200;

        int dotCount() const;
        int currentIndex() const;
        qreal weight(int index) const;
        qreal contentWidth(int count) const;
        QRectF contentRect() const;
        QRectF dotRect(int index) const;
        int dotIndexAt(const QPointF& pos) const;

        void bind(ShadCarousel* carousel);
        void unbind();
        void syncPosition(bool animated);
        void setHoveredIndex(int index);

        QPointer<ShadCarousel> _carousel;
        QList<QMetaObject::Connection> _connections;
        QVariantAnimation* _animation{nullptr};
        qreal _position{0.0};
        int _dotSize{kDefaultDotSize};
        int _spacing{kDefaultSpacing};
        int _hoveredIndex{-1};
        bool _isFocusVisible{false};
};

#endif // SHADCAROUSELDOTSPRIVATE_H
