#ifndef SHADSCROLLBARPRIVATE_H
#define SHADSCROLLBARPRIVATE_H

#include <QPoint>
#include <QRectF>
#include <QtGlobal>

#include "ShadProperty.h"

class ShadScrollBar;
class ShadTransition;

class ShadScrollBarPrivate
{
    SHAD_D_CREATE(ShadScrollBar)

    public:
        ShadScrollBarPrivate();
        ~ShadScrollBarPrivate();

        QRectF trackRect() const;
        // 是否反向显示：与 QStyleOptionSlider::upsideDown 一致，水平方向为 invertedAppearance 与 RTL 异或，垂直方向为 invertedAppearance
        bool isReversed() const;
        // 不考虑反向时的滑块位置（最小值在左侧或上方），拖动与按页滚动按这个坐标计算
        QRectF logicalThumbRect() const;
        // 实际绘制与命中的滑块位置，反向时沿主轴镜像
        QRectF thumbRect() const;
        // 主轴方向的逻辑坐标（水平取 x，垂直取 y），反向时沿主轴镜像
        qreal mainCoordinate(const QPoint& pos) const;
        // 根据滑块起点在轨道中的位置换算数值
        int valueFromThumbStart(qreal thumbStart) const;
        void updateHoverState(const QPoint& pos);

        int _trackInset{0};
        bool _isDragging{false};
        bool _isThumbHovered{false};
        // 按下时鼠标相对滑块起点的偏移，拖动时保持不变
        qreal _dragOffset{0.0};
        ShadTransition* _hoverTransition{nullptr};
};

#endif // SHADSCROLLBARPRIVATE_H
