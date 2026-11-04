#ifndef SHADRATINGPRIVATE_H
#define SHADRATINGPRIVATE_H

#include <QColor>
#include <QPainterPath>
#include <QPoint>
#include <QRectF>

#include "ShadProperty.h"

class ShadRating;

class ShadRatingPrivate
{
    SHAD_D_CREATE(ShadRating)

    public:
        ShadRatingPrivate();
        ~ShadRatingPrivate();

        // 第 index 颗星所在的区域（组件坐标），RTL 时镜像
        QRectF starRect(int index) const;
        // 光标所在的星星，不在任何星星上时返回 -1；星星之间的间距算作前一颗星，移动时不会闪回 0
        int starAt(const QPoint& pos) const;
        // Lucide star 图标的多边形（24 视口）按 rect 缩放后的路径；描边宽度为边长的 1/12（对应 stroke-width 2）
        static QPainterPath starPath(const QRectF& rect);
        // 设置悬停预览的星星并重绘
        void setHoveredStar(int index);
        bool isInteractive() const;

        int _rating{0};
        int _maximum{5};
        int _starSize{20};
        int _spacing{4};
        bool _isReadOnly{false};
        QColor _activeColor;
        int _hoveredStar{-1};
        bool _isFocusVisible{false};
};

#endif // SHADRATINGPRIVATE_H
