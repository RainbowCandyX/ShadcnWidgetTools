#ifndef SHADSLIDERPRIVATE_H
#define SHADSLIDERPRIVATE_H

#include <QPoint>
#include <QPointF>
#include <QRectF>

#include "ShadProperty.h"

class ShadSlider;
class ShadTransition;

class ShadSliderPrivate
{
    SHAD_D_CREATE(ShadSlider)

    public:
        ShadSliderPrivate();
        ~ShadSliderPrivate();

        enum class Thumb
        {
            None,
            Lower,
            Upper,
        };

        // 是否从轨道末端开始计算（垂直方向默认最小值在底部）
        bool isUpsideDown() const;
        // 滑块中心可移动的像素跨度
        int thumbSpan() const;
        int positionOfValue(int value) const;
        QPointF thumbCenter(int value) const;
        QRectF thumbRect(int value) const;
        QRectF thumbRect(Thumb thumb) const;
        QRectF trackRect() const;
        QRectF rangeRect() const;
        int valueFromPosition(const QPoint& pos) const;
        // 悬停命中的滑块，上限滑块绘制在上层，优先命中
        Thumb thumbAt(const QPoint& pos) const;
        // 点击轨道时选择离鼠标更近的滑块
        Thumb nearestThumb(const QPoint& pos) const;
        // 把指定滑块移动到 value，范围模式下自动限制在另一个滑块的同侧
        void moveThumb(Thumb thumb, int value);
        // 根据悬停、拖动与键盘焦点刷新各滑块外圈 ring 动画目标
        void updateRingState();

        bool _isRangeMode{false};
        int _lowerValue{0};
        bool _isFocusVisible{false};
        Thumb _hoveredThumb{Thumb::None};
        Thumb _draggingThumb{Thumb::None};
        // 键盘操作作用的滑块，最近一次点击或拖动的滑块
        Thumb _activeThumb{Thumb::Upper};
        ShadTransition* _upperRingTransition{nullptr};
        ShadTransition* _lowerRingTransition{nullptr};
};

#endif // SHADSLIDERPRIVATE_H
