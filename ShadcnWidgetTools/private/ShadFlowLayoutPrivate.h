#ifndef SHADFLOWLAYOUTPRIVATE_H
#define SHADFLOWLAYOUTPRIVATE_H

#include <QList>
#include <QRect>

#include "ShadProperty.h"

class QLayoutItem;
class ShadFlowLayout;

class ShadFlowLayoutPrivate
{
    SHAD_D_CREATE(ShadFlowLayout)

    public:
        ShadFlowLayoutPrivate();
        ~ShadFlowLayoutPrivate();

        // 在 rect 内排列组件，isTestOnly 为 true 时只计算不设置几何；返回包含上下边距的总高度
        int doLayout(const QRect& rect, bool isTestOnly) const;

        QList<QLayoutItem*> _items;
        int _horizontalSpacing{8};
        int _verticalSpacing{8};
};

#endif // SHADFLOWLAYOUTPRIVATE_H
