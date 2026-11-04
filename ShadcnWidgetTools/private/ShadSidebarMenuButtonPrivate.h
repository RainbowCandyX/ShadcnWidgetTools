#ifndef SHADSIDEBARMENUBUTTONPRIVATE_H
#define SHADSIDEBARMENUBUTTONPRIVATE_H

#include <QFont>
#include <QPointer>
#include <QRect>
#include <QString>
#include <QWidget>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class ShadAvatar;
class ShadSidebar;
class ShadSidebarMenuButton;
class ShadSidebarMenuSub;

class ShadSidebarMenuButtonPrivate
{
    SHAD_D_CREATE(ShadSidebarMenuButton)

    public:
        ShadSidebarMenuButtonPrivate();
        ~ShadSidebarMenuButtonPrivate();

        static constexpr int kPadding = 8;
        static constexpr int kGap = 8;
        static constexpr int kIconSize = 16;
        static constexpr int kIconBoxSize = 32;
        static constexpr int kCollapsedSize = 32;
        static constexpr int kEndItemSize = 20;
        static constexpr int kEndInset = 4;
        static constexpr int kSubItemHeight = 28;
        static constexpr int kFocusRingWidth = 2;
        static constexpr int kMarkWidth = 2;
        static constexpr qreal kMarkInset = 6.0;

        int buttonHeight() const;
        // Large 尺寸（非子菜单项）的图标方块或头像位置
        bool isBoxed() const;
        bool hasLeadingMedia() const;
        void updateAvatarGeometry();
        QFont titleFont() const;
        QFont descriptionFont() const;
        bool isActionVisible() const;
        int endReserve() const;
        // 以下矩形为 LTR 逻辑坐标
        QRect logicalActionRect() const;
        QRect visualRect(const QRect& logicalRect) const;
        // 所属侧栏，没有时返回 nullptr
        ShadSidebar* sidebar() const;
        void onClicked();
        // 由 ShadSidebar 调用
        void setIconCollapsed(bool collapsed, ShadPlacementType::Side tooltipSide);
        // 侧栏收起为图标的过渡进度
        void setIconProgress(qreal progress);
        // 图标方块的起始位置
        int boxLeft() const;
        void setMark(bool visible, qreal topInset, qreal bottomInset);
        ShadSidebarMenuSub* ownerSubMenu() const;

        ShadIconType::IconName _icon{ShadIconType::None};
        QString _description;
        ShadSidebarMenuButtonType::Size _size{ShadSidebarMenuButtonType::Medium};
        ShadSidebarMenuButtonType::Variant _variant{ShadSidebarMenuButtonType::Default};
        bool _isActive{false};
        bool _isSelectable{true};
        bool _isOpen{false};
        QPointer<ShadAvatar> _avatar;
        QString _badgeText;
        ShadIconType::IconName _actionIcon{ShadIconType::None};
        bool _isActionAlwaysVisible{false};
        bool _isSubItem{false};
        QPointer<ShadSidebarMenuSub> _subMenu;
        // 有子菜单时按钮与子菜单所在的容器
        QPointer<QWidget> _menuItem;
        bool _isIconCollapsed{false};
        qreal _iconProgress{1.0};
        bool _isHovered{false};
        bool _isActionHovered{false};
        bool _isActionPressed{false};
        bool _isFocusVisible{false};
        bool _isMarkVisible{false};
        qreal _markTop{kMarkInset};
        qreal _markBottom{kMarkInset};
};

#endif // SHADSIDEBARMENUBUTTONPRIVATE_H
