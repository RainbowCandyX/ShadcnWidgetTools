#ifndef SHADSIDEBARGROUPPRIVATE_H
#define SHADSIDEBARGROUPPRIVATE_H

#include <QRect>
#include <QString>
#include <QtGlobal>

#include "ShadIconType.h"
#include "ShadProperty.h"

class QVBoxLayout;
class ShadFrameAnimation;
class QWidget;
class ShadSidebarCollapseArea;
class ShadSidebarGroup;

class ShadSidebarGroupPrivate
{
    SHAD_D_CREATE(ShadSidebarGroup)

    public:
        ShadSidebarGroupPrivate();
        ~ShadSidebarGroupPrivate();

        // p-2，标题 h-8 px-2，菜单 gap-1
        static constexpr int kPadding = 8;
        static constexpr int kLabelHeight = 32;
        static constexpr int kLabelPaddingX = 8;
        static constexpr int kMenuGap = 4;
        // 操作按钮 w-5，位于 top-3.5 right-3
        static constexpr int kActionSize = 20;
        static constexpr int kActionTop = 14;
        static constexpr int kActionEnd = 12;
        static constexpr int kIconSize = 16;
        static constexpr int kAnimationDurationMs = 200;

        bool isLabelVisible() const;
        bool isActionVisible() const;
        // 以下矩形为 LTR 逻辑坐标
        QRect logicalLabelRect() const;
        QRect logicalActionRect() const;
        QRect visualRect(const QRect& logicalRect) const;
        // 标题行可见性变化后调整布局的上边距
        void updateMargins();
        void setProgress(qreal progress);
        // 侧栏收起为图标时由 ShadSidebar 调用
        void setIconCollapsed(bool collapsed);
        // 侧栏收起为图标的过渡进度
        void setLabelProgress(qreal progress);

        QVBoxLayout* _mainLayout{nullptr};
        // 菜单区的裁剪容器，折叠过渡期间按进度裁剪菜单区，不压缩菜单项
        ShadSidebarCollapseArea* _menuClip{nullptr};
        // 菜单区为裁剪容器的内容组件
        QWidget* _menuArea{nullptr};
        QVBoxLayout* _menuLayout{nullptr};
        ShadFrameAnimation* _animation{nullptr};
        QString _label;
        ShadIconType::IconName _actionIcon{ShadIconType::None};
        bool _isCollapsible{false};
        bool _isExpanded{true};
        bool _isIconCollapsed{false};
        qreal _progress{1.0};
        qreal _labelProgress{1.0};
        bool _isLabelHovered{false};
        bool _isActionHovered{false};
        bool _isActionPressed{false};
        bool _isLabelPressed{false};
        bool _isFocusVisible{false};
};

#endif // SHADSIDEBARGROUPPRIVATE_H
