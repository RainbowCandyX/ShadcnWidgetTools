#ifndef SHADWATERMARKPRIVATE_H
#define SHADWATERMARKPRIVATE_H

#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QtGlobal>

#include "ShadProperty.h"

class QWidget;
class ShadWatermark;

class ShadWatermarkPrivate
{
    SHAD_D_CREATE(ShadWatermark)

    public:
        ShadWatermarkPrivate();
        ~ShadWatermarkPrivate();

        static constexpr int kMaxImageWidth = 120;

        void invalidate();
        QSize contentSize() const;
        QSize imageDisplaySize() const;
        QSize unitSize() const;
        QColor effectiveColor() const;
        QPoint effectiveOffset() const;
        void ensureCache(qreal devicePixelRatio);

        QWidget* _target{nullptr};
        QString _text;
        QImage _image;
        QSize _imageSize;
        qreal _rotation{-22.0};
        QSize _gap{100, 100};
        QPoint _offset;
        bool _hasOffset{false};
        int _fontPixelSize{16};
        QColor _color;
        bool _hasColor{false};
        qreal _opacity{1.0};
        QPixmap _cache;
        qreal _cacheRatio{0.0};
};

#endif
