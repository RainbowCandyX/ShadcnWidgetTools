#ifndef SHADICON_H
#define SHADICON_H

#include <QColor>
#include <QFont>
#include <QIcon>
#include <QPixmap>
#include <QRectF>
#include <QString>

#include "ShadIconType.h"
#include "ShadProperty.h"

class QPainter;

// Lucide 图标字体工具类，shadcn 默认图标尺寸为 16px
class SHADCN_EXPORT ShadIcon
{
    public:
        ShadIcon() = delete;

        // 首次调用时加载内嵌字体，加载失败返回空字符串
        static QString fontFamily();
        static QFont font(int pixelSize = 16);
        static QString glyph(ShadIconType::IconName icon);

        static void paint(QPainter* painter, const QRectF& rect, ShadIconType::IconName icon, const QColor& color, int pixelSize = 16);
        static QPixmap pixmap(ShadIconType::IconName icon, const QColor& color, int pixelSize = 16, qreal devicePixelRatio = 1.0);
        static QIcon icon(ShadIconType::IconName icon, const QColor& color, int pixelSize = 16);
};

#endif // SHADICON_H
