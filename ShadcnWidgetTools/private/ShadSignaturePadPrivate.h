#ifndef SHADSIGNATUREPADPRIVATE_H
#define SHADSIGNATUREPADPRIVATE_H

#include <QColor>
#include <QImage>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "ShadProperty.h"

class QPainter;
class ShadSignaturePad;

class ShadSignaturePadPrivate
{
    SHAD_D_CREATE(ShadSignaturePad)

    public:
        struct StrokePoint
        {
            QPointF pos;
            qreal width{0.0};
        };

        struct Stroke
        {
            QVector<StrokePoint> points;
        };

        static constexpr qreal kSpeedLimit = 2.5;
        static constexpr qreal kKeepWidthRatio = 0.7;
        static constexpr qreal kMinMoveDistance = 1.0;
        static constexpr qreal kTrimMargin = 8.0;
        static constexpr qreal kBaselineInset = 16.0;
        static constexpr qreal kMarkerHalfSize = 4.0;

        ShadSignaturePadPrivate();
        ~ShadSignaturePadPrivate();

        QColor effectivePenColor() const;
        void invalidateCache();
        bool ensureCache();
        void drawSegment(QPainter* painter, const Stroke& stroke, int index) const;
        void drawTail(QPainter* painter, const Stroke& stroke) const;
        void paintStroke(QPainter* painter, const Stroke& stroke, bool includeTail) const;
        QRectF strokesBounds() const;
        qreal targetWidth(qreal distance, qint64 duration, qreal pressure) const;
        void beginStroke(const QPointF& pos, qint64 timestamp, qreal pressure);
        void appendPoint(const QPointF& pos, qint64 timestamp, qreal pressure);
        void endStroke();
        bool canDraw() const;
        void refreshCursor();

        QVector<Stroke> _strokes;
        QImage _cache;
        bool _isCacheDirty{true};
        bool _isDrawing{false};
        bool _isTabletStroke{false};
        qint64 _lastTimestamp{0};
        QColor _penColor;
        bool _hasPenColor{false};
        qreal _minPenWidth{1.0};
        qreal _maxPenWidth{3.5};
        QString _placeholderText;
        bool _hasCustomPlaceholder{false};
        bool _isReadOnly{false};
        bool _isFocusVisible{false};
};

#endif
