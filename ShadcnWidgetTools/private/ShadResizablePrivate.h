#ifndef SHADRESIZABLEPRIVATE_H
#define SHADRESIZABLEPRIVATE_H

#include "ShadProperty.h"

class ShadResizable;

class ShadResizablePrivate
{
    SHAD_D_CREATE(ShadResizable)

    public:
        ShadResizablePrivate();
        ~ShadResizablePrivate();

        // 无抓手时手柄宽 5px（1px 分隔线两侧各留 2px 便于拖动），有抓手时与抓手短边一致
        static constexpr int kHandleWidth = 5;
        static constexpr int kGripHandleWidth = 12;

        // 按是否显示抓手刷新手柄宽度并重绘所有手柄
        void applyHandleWidth();

        bool _isGripVisible{false};
};

#endif // SHADRESIZABLEPRIVATE_H
