#include "ShadAvatarPrivate.h"

#include <QBrush>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QRgb>
#include <QtCore/qnamespace.h>

#include "ShadAvatar.h"
#include "ShadTheme.h"

ShadAvatarPrivate::ShadAvatarPrivate()
{
}

ShadAvatarPrivate::~ShadAvatarPrivate()
{
}

int ShadAvatarPrivate::diameter(ShadAvatarType::Size size)
{
    switch (size)
    {
        case ShadAvatarType::Small:
            return 24;
        case ShadAvatarType::Large:
            return 40;
        case ShadAvatarType::Medium:
            break;
    }
    return 32;
}

int ShadAvatarPrivate::fallbackPixelSize(ShadAvatarType::Size size)
{
    return size == ShadAvatarType::Small ? 12 : 14;
}

int ShadAvatarPrivate::iconSize(ShadAvatarType::Size size)
{
    switch (size)
    {
        case ShadAvatarType::Small:
            return 12;
        case ShadAvatarType::Large:
            return 20;
        case ShadAvatarType::Medium:
            break;
    }
    return 16;
}

int ShadAvatarPrivate::badgeDiameter(ShadAvatarType::Size size)
{
    switch (size)
    {
        case ShadAvatarType::Small:
            return 8;
        case ShadAvatarType::Large:
            return 12;
        case ShadAvatarType::Medium:
            break;
    }
    return 10;
}

void ShadAvatarPrivate::drawRing(QPainter* painter, const QRectF& rect, qreal radius)
{
    // 圆环中线位于外形外 1px，宽 2px
    const qreal half = kRingWidth / 2.0;
    painter->save();
    painter->setPen(QPen(shadTheme->getColor(ShadThemeType::Background), kRingWidth));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(rect.adjusted(-half, -half, half, half), radius + half, radius + half);
    painter->restore();
}

qreal ShadAvatarPrivate::cornerRadius(qreal side) const
{
    return _shape == ShadAvatarType::Circle ? side / 2.0 : shadTheme->getRadius(ShadThemeType::Large);
}

QPixmap ShadAvatarPrivate::shapedImage(qreal devicePixelRatio)
{
    const int side = qRound(diameter(_size) * devicePixelRatio);
    if (!_cache.isNull() && qFuzzyCompare(_cacheDevicePixelRatio, devicePixelRatio) && _cache.width() == side)
    {
        return _cache;
    }
    // object-cover：按短边缩放填满正方形，再从中间裁剪
    const QImage scaled = _image.toImage().scaled(side, side, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QImage cropped = scaled.copy((scaled.width() - side) / 2, (scaled.height() - side) / 2, side, side).convertToFormat(QImage::Format_ARGB32);
    if (_isGrayscale)
    {
        // 保留透明度的灰度转换，Format_Grayscale8 会丢失透明度
        for (int y = 0; y < cropped.height(); ++y)
        {
            QRgb* line = reinterpret_cast<QRgb*>(cropped.scanLine(y));
            for (int x = 0; x < cropped.width(); ++x)
            {
                const int gray = qGray(line[x]);
                line[x] = qRgba(gray, gray, gray, qAlpha(line[x]));
            }
        }
    }
    // 用图片作为画刷绘制，边缘比裁剪路径平滑；圆角按物理像素缩放
    QImage result(side, side, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    {
        const qreal radius = cornerRadius(diameter(_size)) * devicePixelRatio;
        QPainter painter(&result);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(cropped));
        painter.drawRoundedRect(QRectF(0, 0, side, side), radius, radius);
    }
    _cache = QPixmap::fromImage(result);
    _cache.setDevicePixelRatio(devicePixelRatio);
    _cacheDevicePixelRatio = devicePixelRatio;
    return _cache;
}

void ShadAvatarPrivate::invalidateCache()
{
    _cache = QPixmap();
    _cacheDevicePixelRatio = 0.0;
}
