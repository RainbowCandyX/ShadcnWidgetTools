#include "ShadWatermarkPrivate.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QRectF>
#include <QStringList>
#include <QTransform>
#include <QtCore/qnamespace.h>

#include <cmath>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadWatermark.h"

ShadWatermarkPrivate::ShadWatermarkPrivate()
{
}

ShadWatermarkPrivate::~ShadWatermarkPrivate()
{
}

void ShadWatermarkPrivate::invalidate()
{
    _cache = QPixmap();
    _cacheRatio = 0.0;
    q_ptr->update();
}

QSize ShadWatermarkPrivate::imageDisplaySize() const
{
    if (_imageSize.isValid() && !_imageSize.isEmpty())
    {
        return _imageSize;
    }
    if (_image.width() > kMaxImageWidth)
    {
        const int height = qMax(1, qRound(static_cast<qreal>(_image.height()) * kMaxImageWidth / _image.width()));
        return QSize(kMaxImageWidth, height);
    }
    return _image.size();
}

QSize ShadWatermarkPrivate::contentSize() const
{
    if (!_image.isNull())
    {
        return imageDisplaySize();
    }
    QFont font = q_ptr->font();
    font.setPixelSize(qMax(1, _fontPixelSize));
    const QFontMetrics metrics(font);
    const QStringList lines = _text.split(QLatin1Char('\n'));
    int width = 0;
    for (const QString& line : lines)
    {
        width = qMax(width, metrics.horizontalAdvance(line));
    }
    return QSize(width, metrics.height() * static_cast<int>(lines.size()));
}

QSize ShadWatermarkPrivate::unitSize() const
{
    const QSize content = contentSize();
    const QRectF bounds = QTransform().rotate(_rotation).mapRect(QRectF(0.0, 0.0, content.width(), content.height()));
    const int width = static_cast<int>(std::ceil(bounds.width())) + qMax(0, _gap.width());
    const int height = static_cast<int>(std::ceil(bounds.height())) + qMax(0, _gap.height());
    return QSize(qMax(1, width), qMax(1, height));
}

QColor ShadWatermarkPrivate::effectiveColor() const
{
    if (_hasColor)
    {
        return _color;
    }
    return ShadStyleHelper::withAlpha(shadTheme->getColor(ShadThemeType::Foreground), 0.15);
}

QPoint ShadWatermarkPrivate::effectiveOffset() const
{
    if (_hasOffset)
    {
        return _offset;
    }
    return QPoint(_gap.width() / 2, _gap.height() / 2);
}

void ShadWatermarkPrivate::ensureCache(qreal devicePixelRatio)
{
    if (!_cache.isNull() && qFuzzyCompare(_cacheRatio, devicePixelRatio))
    {
        return;
    }
    const QSize unit = unitSize();
    const QSize content = contentSize();
    QPixmap pixmap(unit * devicePixelRatio);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::transparent);
    if (content.isEmpty())
    {
        _cache = pixmap;
        _cacheRatio = devicePixelRatio;
        return;
    }
    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.translate(unit.width() / 2.0, unit.height() / 2.0);
    painter.rotate(_rotation);
    const QRectF contentRect(-content.width() / 2.0, -content.height() / 2.0, content.width(), content.height());
    if (!_image.isNull())
    {
        painter.setOpacity(_opacity);
        painter.drawImage(contentRect, _image);
    }
    else
    {
        QFont font = q_ptr->font();
        font.setPixelSize(qMax(1, _fontPixelSize));
        painter.setFont(font);
        painter.setPen(effectiveColor());
        const QStringList lines = _text.split(QLatin1Char('\n'));
        const qreal lineHeight = static_cast<qreal>(QFontMetrics(font).height());
        for (int index = 0; index < lines.size(); ++index)
        {
            const QRectF lineRect(contentRect.left(), contentRect.top() + lineHeight * index, contentRect.width(), lineHeight);
            painter.drawText(lineRect, Qt::AlignCenter, lines.at(index));
        }
    }
    painter.end();
    _cache = pixmap;
    _cacheRatio = devicePixelRatio;
}
