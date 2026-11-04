#ifndef SHADPLACEMENT_H
#define SHADPLACEMENT_H

#include <QRect>
#include <QSize>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"

// 浮层相对锚点的定位，供提示与 Popover 共用，不对外导出
namespace ShadPlacement
{
    // 与屏幕可用区域边缘保持的最小距离
    constexpr int kScreenMargin = 8;

    struct Result
    {
        // 浮层在屏幕坐标中的位置
        QRect rect;
        // 实际使用的物理方向（已处理 RTL 互换与空间不足时的翻转）
        ShadPlacementType::Side side{ShadPlacementType::Bottom};
    };

    // anchor 为锚点的屏幕坐标矩形，size 为浮层尺寸，offset 为与锚点的距离（sideOffset）
    // 主方向空间不足且对侧空间更大时翻到对侧；交叉方向按 align 对齐后限制在屏幕可用区域内
    Result place(const QRect& anchor, const QSize& size, ShadPlacementType::Side side, ShadPlacementType::Align align, int offset, Qt::LayoutDirection direction);
} // namespace ShadPlacement

#endif // SHADPLACEMENT_H
