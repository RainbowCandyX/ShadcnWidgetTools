#include "ShadImageCropperPrivate.h"

#include <QImage>
#include <QPixmap>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <cmath>

#include "ShadImageCropper.h"

namespace
{
    qreal clampValue(qreal value, qreal lower, qreal upper)
    {
        return qMax(lower, qMin(value, upper));
    }
} // namespace

ShadImageCropperPrivate::ShadImageCropperPrivate()
{
}

ShadImageCropperPrivate::~ShadImageCropperPrivate()
{
}

QRectF ShadImageCropperPrivate::contentRect() const
{
    Q_Q(const ShadImageCropper);
    return QRectF(q->rect()).adjusted(kInset, kInset, -kInset, -kInset);
}

QRectF ShadImageCropperPrivate::displayRect() const
{
    if (_transformed.isNull())
    {
        return QRectF();
    }
    const QRectF available = contentRect().adjusted(kPadding, kPadding, -kPadding, -kPadding);
    if (available.width() <= 0.0 || available.height() <= 0.0)
    {
        return QRectF();
    }
    const qreal scale = qMin(available.width() / _transformed.width(), available.height() / _transformed.height());
    const qreal width = qMax(1.0, std::round(_transformed.width() * scale));
    const qreal height = qMax(1.0, std::round(_transformed.height() * scale));
    const qreal left = std::round(available.left() + (available.width() - width) / 2.0);
    const qreal top = std::round(available.top() + (available.height() - height) / 2.0);
    return QRectF(left, top, width, height);
}

qreal ShadImageCropperPrivate::displayScale() const
{
    const QRectF display = displayRect();
    if (display.isEmpty() || _transformed.isNull())
    {
        return 1.0;
    }
    return display.width() / _transformed.width();
}

QRectF ShadImageCropperPrivate::cropDisplayRect() const
{
    const QRectF display = displayRect();
    const qreal scale = displayScale();
    return QRectF(display.left() + _cropRect.left() * scale, display.top() + _cropRect.top() * scale, _cropRect.width() * scale, _cropRect.height() * scale);
}

qreal ShadImageCropperPrivate::effectiveRatio() const
{
    return _shape == ShadImageCropperType::Circle ? 1.0 : _aspectRatio;
}

qreal ShadImageCropperPrivate::minWidthPixels() const
{
    const qreal minimum = qMax<qreal>(kMinImagePixels, kMinDisplayPixels / displayScale());
    return qMin<qreal>(minimum, _transformed.width());
}

qreal ShadImageCropperPrivate::minHeightPixels() const
{
    const qreal minimum = qMax<qreal>(kMinImagePixels, kMinDisplayPixels / displayScale());
    return qMin<qreal>(minimum, _transformed.height());
}

QRectF ShadImageCropperPrivate::defaultCropRect() const
{
    if (_transformed.isNull())
    {
        return QRectF();
    }
    const qreal imageWidth = _transformed.width();
    const qreal imageHeight = _transformed.height();
    const qreal ratio = effectiveRatio();
    qreal width = imageWidth;
    qreal height = imageHeight;
    if (ratio > 0.0)
    {
        if (imageWidth / imageHeight > ratio)
        {
            width = imageHeight * ratio;
        }
        else
        {
            height = imageWidth / ratio;
        }
    }
    width *= 0.8;
    height *= 0.8;
    return QRectF((imageWidth - width) / 2.0, (imageHeight - height) / 2.0, width, height);
}

QRectF ShadImageCropperPrivate::clampToImage(const QRectF& rect) const
{
    const qreal imageWidth = _transformed.width();
    const qreal imageHeight = _transformed.height();
    const qreal width = qMin(qMax(rect.width(), minWidthPixels()), imageWidth);
    const qreal height = qMin(qMax(rect.height(), minHeightPixels()), imageHeight);
    const qreal left = clampValue(rect.left(), 0.0, imageWidth - width);
    const qreal top = clampValue(rect.top(), 0.0, imageHeight - height);
    return QRectF(left, top, width, height);
}

QRectF ShadImageCropperPrivate::fitAspect(const QRectF& rect) const
{
    const qreal ratio = effectiveRatio();
    if (ratio <= 0.0)
    {
        return clampToImage(rect);
    }
    const qreal imageWidth = _transformed.width();
    const qreal imageHeight = _transformed.height();
    const QPointF center = rect.center();
    qreal width = std::sqrt(qMax(1.0, rect.width() * rect.height()) * ratio);
    qreal height = width / ratio;
    width = qMax(width, minWidthPixels());
    height = width / ratio;
    height = qMax(height, minHeightPixels());
    width = height * ratio;
    if (width > imageWidth)
    {
        width = imageWidth;
        height = width / ratio;
    }
    if (height > imageHeight)
    {
        height = imageHeight;
        width = height * ratio;
    }
    const qreal left = clampValue(center.x() - width / 2.0, 0.0, imageWidth - width);
    const qreal top = clampValue(center.y() - height / 2.0, 0.0, imageHeight - height);
    return QRectF(left, top, width, height);
}

QRect ShadImageCropperPrivate::roundedCropRect() const
{
    if (_transformed.isNull() || _cropRect.isEmpty())
    {
        return QRect();
    }
    const int left = qRound(_cropRect.left());
    const int top = qRound(_cropRect.top());
    const int right = qRound(_cropRect.right());
    const int bottom = qRound(_cropRect.bottom());
    const QRect rounded(QPoint(left, top), QSize(qMax(1, right - left), qMax(1, bottom - top)));
    return rounded.intersected(QRect(QPoint(0, 0), _transformed.size()));
}

ShadImageCropperPrivate::Handle ShadImageCropperPrivate::hitTest(const QPointF& position) const
{
    if (_transformed.isNull())
    {
        return None;
    }
    const QRectF crop = cropDisplayRect();
    const qreal half = kHandleHitSize;
    const struct
    {
        QPointF point;
        Handle handle;
    } corners[] = {
        {crop.topLeft(), TopLeft},
        {crop.topRight(), TopRight},
        {crop.bottomRight(), BottomRight},
        {crop.bottomLeft(), BottomLeft},
    };
    for (const auto& corner : corners)
    {
        if (QRectF(corner.point.x() - half, corner.point.y() - half, half * 2.0, half * 2.0).contains(position))
        {
            return corner.handle;
        }
    }
    const qreal band = 6.0;
    const bool isInsideHorizontal = position.x() >= crop.left() && position.x() <= crop.right();
    const bool isInsideVertical = position.y() >= crop.top() && position.y() <= crop.bottom();
    if (isInsideHorizontal && qAbs(position.y() - crop.top()) <= band)
    {
        return Top;
    }
    if (isInsideHorizontal && qAbs(position.y() - crop.bottom()) <= band)
    {
        return Bottom;
    }
    if (isInsideVertical && qAbs(position.x() - crop.left()) <= band)
    {
        return Left;
    }
    if (isInsideVertical && qAbs(position.x() - crop.right()) <= band)
    {
        return Right;
    }
    if (crop.contains(position))
    {
        return Move;
    }
    return None;
}

QRectF ShadImageCropperPrivate::resized(Handle handle, qreal dx, qreal dy) const
{
    const qreal imageWidth = _transformed.width();
    const qreal imageHeight = _transformed.height();
    const bool hasLeft = handle == TopLeft || handle == Left || handle == BottomLeft;
    const bool hasRight = handle == TopRight || handle == Right || handle == BottomRight;
    const bool hasTop = handle == TopLeft || handle == Top || handle == TopRight;
    const bool hasBottom = handle == BottomLeft || handle == Bottom || handle == BottomRight;
    const qreal minWidth = minWidthPixels();
    const qreal minHeight = minHeightPixels();
    const qreal ratio = effectiveRatio();
    const QRectF start = _pressRect;
    if (ratio <= 0.0)
    {
        qreal left = start.left();
        qreal right = start.right();
        qreal top = start.top();
        qreal bottom = start.bottom();
        if (hasLeft)
        {
            left = clampValue(start.left() + dx, 0.0, right - minWidth);
        }
        if (hasRight)
        {
            right = clampValue(start.right() + dx, left + minWidth, imageWidth);
        }
        if (hasTop)
        {
            top = clampValue(start.top() + dy, 0.0, bottom - minHeight);
        }
        if (hasBottom)
        {
            bottom = clampValue(start.bottom() + dy, top + minHeight, imageHeight);
        }
        return QRectF(QPointF(left, top), QPointF(right, bottom));
    }
    const qreal lowerWidth = qMax(minWidth, minHeight * ratio);
    if ((hasLeft || hasRight) && (hasTop || hasBottom))
    {
        const qreal anchorX = hasLeft ? start.right() : start.left();
        const qreal anchorY = hasTop ? start.bottom() : start.top();
        const qreal movingX = (hasLeft ? start.left() : start.right()) + dx;
        const qreal movingY = (hasTop ? start.top() : start.bottom()) + dy;
        const qreal wantedWidth = hasLeft ? anchorX - movingX : movingX - anchorX;
        const qreal wantedHeight = hasTop ? anchorY - movingY : movingY - anchorY;
        const qreal availableWidth = hasLeft ? anchorX : imageWidth - anchorX;
        const qreal availableHeight = hasTop ? anchorY : imageHeight - anchorY;
        const qreal maxWidth = qMin(availableWidth, availableHeight * ratio);
        const qreal width = qMin(qMax(qMax(wantedWidth, wantedHeight * ratio), qMin(lowerWidth, maxWidth)), maxWidth);
        const qreal height = width / ratio;
        return QRectF(hasLeft ? anchorX - width : anchorX, hasTop ? anchorY - height : anchorY, width, height);
    }
    if (hasLeft || hasRight)
    {
        const qreal anchorX = hasLeft ? start.right() : start.left();
        const qreal centerY = start.center().y();
        const qreal movingX = (hasLeft ? start.left() : start.right()) + dx;
        const qreal wantedWidth = hasLeft ? anchorX - movingX : movingX - anchorX;
        const qreal availableWidth = hasLeft ? anchorX : imageWidth - anchorX;
        const qreal maxWidth = qMin(availableWidth, 2.0 * qMin(centerY, imageHeight - centerY) * ratio);
        const qreal width = qMin(qMax(wantedWidth, qMin(lowerWidth, maxWidth)), maxWidth);
        const qreal height = width / ratio;
        return QRectF(hasLeft ? anchorX - width : anchorX, centerY - height / 2.0, width, height);
    }
    const qreal anchorY = hasTop ? start.bottom() : start.top();
    const qreal centerX = start.center().x();
    const qreal movingY = (hasTop ? start.top() : start.bottom()) + dy;
    const qreal wantedHeight = hasTop ? anchorY - movingY : movingY - anchorY;
    const qreal availableHeight = hasTop ? anchorY : imageHeight - anchorY;
    const qreal lowerHeight = lowerWidth / ratio;
    const qreal maxHeight = qMin(availableHeight, 2.0 * qMin(centerX, imageWidth - centerX) / ratio);
    const qreal height = qMin(qMax(wantedHeight, qMin(lowerHeight, maxHeight)), maxHeight);
    const qreal width = height * ratio;
    return QRectF(centerX - width / 2.0, hasTop ? anchorY - height : anchorY, width, height);
}

void ShadImageCropperPrivate::setCropRectF(const QRectF& rect)
{
    _cropRect = rect;
}

void ShadImageCropperPrivate::emitCropRectIfChanged()
{
    Q_Q(ShadImageCropper);
    const QRect rounded = roundedCropRect();
    if (rounded == _lastEmittedRect)
    {
        return;
    }
    _lastEmittedRect = rounded;
    Q_EMIT q->cropRectChanged(rounded);
}

void ShadImageCropperPrivate::applyTransform(const QImage& image, const QRectF& rect)
{
    _transformed = image;
    _cropRect = rect;
    _isCacheValid = false;
}

void ShadImageCropperPrivate::ensureCache()
{
    Q_Q(ShadImageCropper);
    const QRectF display = displayRect();
    if (display.isEmpty() || _transformed.isNull())
    {
        _cache = QPixmap();
        _isCacheValid = false;
        return;
    }
    const qreal ratio = q->devicePixelRatioF();
    const QSize deviceSize(qMax(1, qRound(display.width() * ratio)), qMax(1, qRound(display.height() * ratio)));
    if (_isCacheValid && _cacheSize == deviceSize && qFuzzyCompare(_cacheRatio, ratio))
    {
        return;
    }
    _cache = QPixmap::fromImage(_transformed.scaled(deviceSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    _cache.setDevicePixelRatio(ratio);
    _cacheSize = deviceSize;
    _cacheRatio = ratio;
    _isCacheValid = true;
}
