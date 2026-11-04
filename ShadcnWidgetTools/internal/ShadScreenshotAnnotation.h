#ifndef SHADSCREENSHOTANNOTATION_H
#define SHADSCREENSHOTANNOTATION_H

#include <QColor>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QPointF>
#include <QString>

class QFont;
class QPainter;
class QRectF;

class ShadScreenshotAnnotation
{
    public:
        enum class Tool
        {
            None = 0,
            Rectangle,
            Ellipse,
            Arrow,
            Pen,
            Mosaic,
            Text,
        };

        static int minimumSize(Tool tool);
        static int maximumSize(Tool tool);
        static int defaultSize(Tool tool);
        static QFont textFont(int pixelSize);

        bool isEmpty() const;
        void paint(QPainter* painter, const QMap<int, QPixmap>& mosaics, const QRectF& screenRect) const;

        Tool tool{Tool::None};
        QColor color;
        int size{0};
        int blockSize{0};
        QList<QPointF> points;
        QString text;
};

#endif
