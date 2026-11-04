#ifndef SHADWIDGETPRIVATE_H
#define SHADWIDGETPRIVATE_H

#include "ShadProperty.h"

class ShadTitleBar;
class ShadWidget;
class ShadWindowFrame;

class ShadWidgetPrivate
{
    SHAD_D_CREATE(ShadWidget)

    public:
        ShadWidgetPrivate();
        ~ShadWidgetPrivate();

        static constexpr int kDefaultTitleBarHeight = 40;
        // 默认尺寸
        static constexpr int kDefaultWidth = 300;
        static constexpr int kDefaultHeight = 300;

        ShadTitleBar* _titleBar{nullptr};
        // 边框、原生消息与边缘缩放，与 ShadWindow / ShadDialog 共用；标题栏为浮动模式
        ShadWindowFrame* _frame{nullptr};
        int _titleBarHeight{kDefaultTitleBarHeight};
};

#endif // SHADWIDGETPRIVATE_H
