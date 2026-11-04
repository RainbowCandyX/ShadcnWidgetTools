#include "ShadScreenshotAnnotation.h"

#include <QChar>
#include <QFont>
#include <QFontMetricsF>
#include <QLineF>
#include <QMap>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

namespace
{
    struct SizeRange
    {
        int minimum;
        int maximum;
        int value;
    };

    constexpr SizeRange kStrokeRange{1, 20, 4};
    constexpr SizeRange kMosaicRange{8, 60, 24};
    constexpr SizeRange kTextRange{12, 48, 18};
    constexpr qreal kMinShapeLength = 2.0;
    constexpr qreal kArrowHeadBase = 8.0;
    constexpr qreal kArrowHeadScale = 3.0;
    constexpr qreal kArrowHeadWidthRatio = 0.5;
    constexpr qreal kDotOffset = 0.01;

    QPainterPath strokePath(const QList<QPointF>& points)
    {
        QPainterPath path;
        if (points.isEmpty())
        {
            return path;
        }
        path.moveTo(points.first());
        if (points.size() == 1)
        {
            path.lineTo(points.first() + QPointF(kDotOffset, 0.0));
            return path;
        }
        for (int i = 1; i < points.size(); ++i)
        {
            path.lineTo(points.at(i));
        }
        return path;
    }

    SizeRange sizeRange(ShadScreenshotAnnotation::Tool tool)
    {
        switch (tool)
        {
            case ShadScreenshotAnnotation::Tool::Mosaic:
                return kMosaicRange;
            case ShadScreenshotAnnotation::Tool::Text:
                return kTextRange;
            case ShadScreenshotAnnotation::Tool::None:
            case ShadScreenshotAnnotation::Tool::Rectangle:
            case ShadScreenshotAnnotation::Tool::Ellipse:
            case ShadScreenshotAnnotation::Tool::Arrow:
            case ShadScreenshotAnnotation::Tool::Pen:
                break;
        }
        return kStrokeRange;
    }
}

int ShadScreenshotAnnotation::minimumSize(Tool tool)
{
    return sizeRange(tool).minimum;
}

int ShadScreenshotAnnotation::maximumSize(Tool tool)
{
    return sizeRange(tool).maximum;
}

int ShadScreenshotAnnotation::defaultSize(Tool tool)
{
    return sizeRange(tool).value;
}

QFont ShadScreenshotAnnotation::textFont(int pixelSize)
{
    QFont font;
    font.setPixelSize(pixelSize);
    return font;
}

bool ShadScreenshotAnnotation::isEmpty() const
{
    switch (tool)
    {
        case Tool::None:
            return true;
        case Tool::Rectangle:
        case Tool::Ellipse:
        case Tool::Arrow:
            return points.size() < 2 || QLineF(points.first(), points.last()).length() < kMinShapeLength;
        case Tool::Pen:
        case Tool::Mosaic:
            return points.isEmpty();
        case Tool::Text:
            return points.isEmpty() || text.trimmed().isEmpty();
    }
    return true;
}

void ShadScreenshotAnnotation::paint(QPainter* painter, const QMap<int, QPixmap>& mosaics, const QRectF& screenRect) const
{
    if (isEmpty())
    {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, size, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    switch (tool)
    {
        case Tool::Rectangle:
            painter->setPen(QPen(color, size, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
            painter->drawRect(QRectF(points.first(), points.last()).normalized());
            break;
        case Tool::Ellipse:
            painter->drawEllipse(QRectF(points.first(), points.last()).normalized());
            break;
        case Tool::Arrow:
            {
                const QLineF line(points.first(), points.last());
                const qreal headLength = qMin(line.length(), kArrowHeadBase + size * kArrowHeadScale);
                const QPointF direction = (line.p2() - line.p1()) / line.length();
                const QPointF normal(-direction.y(), direction.x());
                const QPointF base = line.p2() - direction * headLength;
                const qreal halfWidth = headLength * kArrowHeadWidthRatio;
                painter->drawLine(line.p1(), base);
                painter->setPen(Qt::NoPen);
                painter->setBrush(color);
                painter->drawPolygon(QPolygonF({line.p2(), base + normal * halfWidth, base - normal * halfWidth}));
                break;
            }
        case Tool::Pen:
            painter->drawPath(strokePath(points));
            break;
        case Tool::Mosaic:
            {
                const QPixmap mosaic = mosaics.value(blockSize);
                if (mosaic.isNull())
                {
                    break;
                }
                QPainterPathStroker stroker;
                stroker.setWidth(size);
                stroker.setCapStyle(Qt::RoundCap);
                stroker.setJoinStyle(Qt::RoundJoin);
                painter->setClipPath(stroker.createStroke(strokePath(points)), Qt::IntersectClip);
                painter->drawPixmap(screenRect, mosaic, QRectF(mosaic.rect()));
                break;
            }
        case Tool::Text:
            {
                const QFont font = textFont(size);
                const QFontMetricsF metrics(font);
                painter->setFont(font);
                const QStringList lines = text.split(QLatin1Char('\n'));
                for (int i = 0; i < lines.size(); ++i)
                {
                    painter->drawText(QPointF(points.first().x(), points.first().y() + metrics.ascent() + i * metrics.height()), lines.at(i));
                }
                break;
            }
        case Tool::None:
            break;
    }
    painter->restore();
}
