#include "ShadSignaturePadPrivate.h"

#include <QCursor>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QSize>
#include <QtMath>
#include <QtCore/qnamespace.h>

#include "ShadSignaturePad.h"
#include "ShadTheme.h"

ShadSignaturePadPrivate::ShadSignaturePadPrivate()
{
}

ShadSignaturePadPrivate::~ShadSignaturePadPrivate()
{
}

QColor ShadSignaturePadPrivate::effectivePenColor() const
{
    return _hasPenColor ? _penColor : shadTheme->getColor(ShadThemeType::Foreground);
}

void ShadSignaturePadPrivate::invalidateCache()
{
    _isCacheDirty = true;
}

bool ShadSignaturePadPrivate::ensureCache()
{
    Q_Q(ShadSignaturePad);
    const qreal ratio = q->devicePixelRatioF();
    const QSize pixelSize(qCeil(q->width() * ratio), qCeil(q->height() * ratio));
    if (pixelSize.isEmpty())
    {
        return false;
    }
    if (!_isCacheDirty && _cache.size() == pixelSize && qFuzzyCompare(_cache.devicePixelRatio(), ratio))
    {
        return true;
    }
    _cache = QImage(pixelSize, QImage::Format_ARGB32_Premultiplied);
    _cache.setDevicePixelRatio(ratio);
    _cache.fill(Qt::transparent);
    QPainter painter(&_cache);
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (int i = 0; i < _strokes.size(); ++i)
    {
        const bool isActive = _isDrawing && i == _strokes.size() - 1;
        paintStroke(&painter, _strokes.at(i), !isActive);
    }
    _isCacheDirty = false;
    return true;
}

void ShadSignaturePadPrivate::drawSegment(QPainter* painter, const Stroke& stroke, int index) const
{
    const QVector<StrokePoint>& points = stroke.points;
    if (index < 0 || index + 1 >= points.size())
    {
        return;
    }
    const QPointF end = (points.at(index).pos + points.at(index + 1).pos) / 2.0;
    QPainterPath path;
    if (index == 0)
    {
        path.moveTo(points.at(0).pos);
        path.lineTo(end);
    }
    else
    {
        path.moveTo((points.at(index - 1).pos + points.at(index).pos) / 2.0);
        path.quadTo(points.at(index).pos, end);
    }
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(effectivePenColor(), points.at(index).width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawPath(path);
}

void ShadSignaturePadPrivate::drawTail(QPainter* painter, const Stroke& stroke) const
{
    const QVector<StrokePoint>& points = stroke.points;
    if (points.isEmpty())
    {
        return;
    }
    const StrokePoint& lastPoint = points.last();
    if (points.size() == 1)
    {
        painter->setPen(Qt::NoPen);
        painter->setBrush(effectivePenColor());
        painter->drawEllipse(lastPoint.pos, lastPoint.width / 2.0, lastPoint.width / 2.0);
        return;
    }
    const QPointF start = (points.at(points.size() - 2).pos + lastPoint.pos) / 2.0;
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(effectivePenColor(), lastPoint.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawLine(start, lastPoint.pos);
}

void ShadSignaturePadPrivate::paintStroke(QPainter* painter, const Stroke& stroke, bool includeTail) const
{
    for (int i = 0; i + 1 < stroke.points.size(); ++i)
    {
        drawSegment(painter, stroke, i);
    }
    if (includeTail)
    {
        drawTail(painter, stroke);
    }
}

QRectF ShadSignaturePadPrivate::strokesBounds() const
{
    bool isFirst = true;
    QRectF bounds;
    for (const Stroke& stroke : _strokes)
    {
        for (const StrokePoint& point : stroke.points)
        {
            const qreal half = point.width / 2.0;
            const QRectF pointRect(point.pos.x() - half, point.pos.y() - half, point.width, point.width);
            bounds = isFirst ? pointRect : bounds.united(pointRect);
            isFirst = false;
        }
    }
    return bounds;
}

qreal ShadSignaturePadPrivate::targetWidth(qreal distance, qint64 duration, qreal pressure) const
{
    if (pressure > 0.0)
    {
        return _minPenWidth + qBound<qreal>(0.0, pressure, 1.0) * (_maxPenWidth - _minPenWidth);
    }
    const qreal speed = distance / static_cast<qreal>(qMax<qint64>(1, duration));
    const qreal ratio = qBound<qreal>(0.0, speed / kSpeedLimit, 1.0);
    return _maxPenWidth - ratio * (_maxPenWidth - _minPenWidth);
}

void ShadSignaturePadPrivate::beginStroke(const QPointF& pos, qint64 timestamp, qreal pressure)
{
    Q_Q(ShadSignaturePad);
    Stroke stroke;
    StrokePoint point;
    point.pos = pos;
    point.width = pressure > 0.0 ? targetWidth(0.0, 1, pressure) : (_minPenWidth + _maxPenWidth) / 2.0;
    stroke.points.append(point);
    _strokes.append(stroke);
    _isDrawing = true;
    _lastTimestamp = timestamp;
    Q_EMIT q->strokeStarted();
    q->update();
}

void ShadSignaturePadPrivate::appendPoint(const QPointF& pos, qint64 timestamp, qreal pressure)
{
    Q_Q(ShadSignaturePad);
    if (!_isDrawing || _strokes.isEmpty())
    {
        return;
    }
    Stroke& stroke = _strokes.last();
    const StrokePoint previous = stroke.points.last();
    const qreal distance = QLineF(previous.pos, pos).length();
    if (distance < kMinMoveDistance)
    {
        return;
    }
    StrokePoint point;
    point.pos = pos;
    point.width = kKeepWidthRatio * previous.width + (1.0 - kKeepWidthRatio) * targetWidth(distance, timestamp - _lastTimestamp, pressure);
    _lastTimestamp = timestamp;
    stroke.points.append(point);
    const int count = stroke.points.size();
    if (ensureCache())
    {
        QPainter painter(&_cache);
        painter.setRenderHint(QPainter::Antialiasing, true);
        drawSegment(&painter, stroke, count - 2);
    }
    QRectF dirty = QRectF(stroke.points.at(count - 1).pos, QSizeF(0.0, 0.0));
    for (int i = qMax(0, count - 3); i < count; ++i)
    {
        dirty = dirty.united(QRectF(stroke.points.at(i).pos, QSizeF(0.0, 0.0)));
    }
    const qreal margin = _maxPenWidth + 2.0;
    q->update(dirty.adjusted(-margin, -margin, margin, margin).toAlignedRect());
}

void ShadSignaturePadPrivate::endStroke()
{
    Q_Q(ShadSignaturePad);
    if (!_isDrawing)
    {
        return;
    }
    _isDrawing = false;
    _isTabletStroke = false;
    if (!_strokes.isEmpty() && ensureCache())
    {
        QPainter painter(&_cache);
        painter.setRenderHint(QPainter::Antialiasing, true);
        drawTail(&painter, _strokes.last());
    }
    q->update();
    Q_EMIT q->strokeFinished();
    Q_EMIT q->changed();
}

bool ShadSignaturePadPrivate::canDraw() const
{
    Q_Q(const ShadSignaturePad);
    return q->isEnabled() && !_isReadOnly;
}

void ShadSignaturePadPrivate::refreshCursor()
{
    Q_Q(ShadSignaturePad);
    if (canDraw())
    {
        q->setCursor(Qt::CrossCursor);
    }
    else
    {
        q->unsetCursor();
    }
}
