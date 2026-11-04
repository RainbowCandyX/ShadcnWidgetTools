#ifndef SHADSCROLLAREAPRIVATE_H
#define SHADSCROLLAREAPRIVATE_H

#include "ShadProperty.h"

class QScrollBar;
class QTimer;
class ShadScrollArea;
class ShadScrollBar;

class ShadScrollAreaPrivate
{
    SHAD_D_CREATE(ShadScrollArea)

    public:
        ShadScrollAreaPrivate();
        ~ShadScrollAreaPrivate();

        // 鼠标离开后延迟隐藏滚动条，对应 Radix ScrollArea 的 scrollHideDelay
        static constexpr int kHideDelayMs = 600;

        // 原生滚动条与覆盖滚动条双向同步
        void bindScrollBar(QScrollBar* source, ShadScrollBar* overlay);
        // 把原生滚动条的范围、步长与当前值复制到覆盖滚动条
        void syncScrollBar(QScrollBar* source, ShadScrollBar* overlay);
        // 根据视口位置与悬停状态更新覆盖滚动条的几何与可见性
        void updateOverlayBars();

        ShadScrollBar* _verticalBar{nullptr};
        ShadScrollBar* _horizontalBar{nullptr};
        QTimer* _hideTimer{nullptr};
        bool _isHovered{false};
};

#endif // SHADSCROLLAREAPRIVATE_H
