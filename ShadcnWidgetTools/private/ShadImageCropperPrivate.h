#ifndef SHADIMAGECROPPERPRIVATE_H
#define SHADIMAGECROPPERPRIVATE_H

#include <QImage>
#include <QPixmap>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>

#include "ShadDef.h"
#include "ShadProperty.h"

class ShadImageCropper;

class ShadImageCropperPrivate
{
    SHAD_D_CREATE(ShadImageCropper)

    public:
        enum Handle
        {
            None = 0,
            Move,
            TopLeft,
            Top,
            TopRight,
            Right,
            BottomRight,
            Bottom,
            BottomLeft,
            Left,
        };

        static constexpr int kPadding = 16;
        static constexpr int kInset = 3;
        static constexpr int kMinImagePixels = 20;
        static constexpr int kMinDisplayPixels = 24;
        static constexpr int kHandleHitSize = 10;
        static constexpr int kHandleLength = 16;
        static constexpr int kHandleThickness = 3;

        ShadImageCropperPrivate();
        ~ShadImageCropperPrivate();

        QRectF contentRect() const;
        QRectF displayRect() const;
        qreal displayScale() const;
        QRectF cropDisplayRect() const;
        qreal effectiveRatio() const;
        qreal minWidthPixels() const;
        qreal minHeightPixels() const;
        QRectF defaultCropRect() const;
        QRectF fitAspect(const QRectF& rect) const;
        QRectF clampToImage(const QRectF& rect) const;
        QRect roundedCropRect() const;
        Handle hitTest(const QPointF& position) const;
        QRectF resized(Handle handle, qreal dx, qreal dy) const;
        void setCropRectF(const QRectF& rect);
        void emitCropRectIfChanged();
        void applyTransform(const QImage& image, const QRectF& rect);
        void ensureCache();

        QImage _image;
        QImage _transformed;
        QRectF _cropRect;
        QRect _lastEmittedRect;
        qreal _aspectRatio{0.0};
        ShadImageCropperType::Shape _shape{ShadImageCropperType::Rectangle};
        bool _isFocusVisible{false};
        Handle _pressHandle{None};
        Handle _hoverHandle{None};
        QPointF _pressPosition;
        QRectF _pressRect;
        QPixmap _cache;
        QSize _cacheSize;
        qreal _cacheRatio{0.0};
        bool _isCacheValid{false};
};

#endif // SHADIMAGECROPPERPRIVATE_H
