#include "ShadRatingPrivate.h"

#include <QPointF>
#include <QRect>
#include <QtGlobal>

#include "ShadRating.h"
#include "ShadStyleHelper.h"

namespace
{
    // Lucide star 图标的 10 个顶点（24 视口）
    constexpr qreal kStarPoints[10][2] = {
        {12.0, 2.0},
        {15.09, 8.26},
        {22.0, 9.27},
        {17.0, 14.14},
        {18.18, 21.02},
        {12.0, 17.77},
        {5.82, 21.02},
        {7.0, 14.14},
        {2.0, 9.27},
        {8.91, 8.26},
    };
    constexpr qreal kViewBoxSize = 24.0;
} // namespace

ShadRatingPrivate::ShadRatingPrivate()
{
}

ShadRatingPrivate::~ShadRatingPrivate()
{
}

QRectF ShadRatingPrivate::starRect(int index) const
{
    Q_Q(const ShadRating);
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const QRectF logicalRect(margin + index * (_starSize + _spacing), margin, _starSize, _starSize);
    return ShadStyleHelper::visualRect(q->layoutDirection(), QRectF(q->rect()), logicalRect);
}

int ShadRatingPrivate::starAt(const QPoint& pos) const
{
    Q_Q(const ShadRating);
    // 换算为从起始侧量起的逻辑坐标
    const int logicalX = q->layoutDirection() == Qt::RightToLeft ? q->width() - 1 - pos.x() : pos.x();
    const int offset = logicalX - ShadStyleHelper::kFocusRingWidth;
    const int unit = _starSize + _spacing;
    if (offset < 0 || unit <= 0)
    {
        return -1;
    }
    const int index = offset / unit;
    return index < _maximum ? index : -1;
}

QPainterPath ShadRatingPrivate::starPath(const QRectF& rect)
{
    const qreal scale = rect.width() / kViewBoxSize;
    QPainterPath path;
    for (int index = 0; index < 10; ++index)
    {
        const QPointF point(rect.left() + kStarPoints[index][0] * scale, rect.top() + kStarPoints[index][1] * scale);
        if (index == 0)
        {
            path.moveTo(point);
        }
        else
        {
            path.lineTo(point);
        }
    }
    path.closeSubpath();
    return path;
}

void ShadRatingPrivate::setHoveredStar(int index)
{
    Q_Q(ShadRating);
    if (_hoveredStar == index)
    {
        return;
    }
    _hoveredStar = index;
    q->update();
}

bool ShadRatingPrivate::isInteractive() const
{
    Q_Q(const ShadRating);
    return !_isReadOnly && q->isEnabled();
}
