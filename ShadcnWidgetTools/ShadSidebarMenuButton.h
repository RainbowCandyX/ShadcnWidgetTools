#ifndef SHADSIDEBARMENUBUTTON_H
#define SHADSIDEBARMENUBUTTON_H

#include <QAbstractButton>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class ShadAvatar;

// 侧栏菜单按钮，对应 SidebarMenuButton：rounded-md p-2 gap-2 text-sm，高度 28 / 32 / 48，悬停与选中为 bg-sidebar-accent，选中时 font-medium
// Large 尺寸的图标显示在 32×32 的 bg-sidebar-primary 方块中，可用 setAvatar 换成头像、setDescription 显示第二行文字
// 结束侧可显示徽章（SidebarMenuBadge）或操作按钮（SidebarMenuAction，默认悬停或选中时显示）；addSubItem 添加子菜单（SidebarMenuSub），
// 有子菜单时点击按钮展开 / 折叠子菜单，结束侧显示旋转的箭头；没有子菜单时点击按钮在所属 ShadSidebar 中成为唯一的选中项，setSelectable(false) 除外
// 侧栏收起为图标时缩为 32×32，只显示图标，悬停时以提示显示文字；子菜单、徽章与操作按钮隐藏
// addSubItem 第一次调用时，把按钮在所在 QBoxLayout 中的位置替换为菜单项容器（SidebarMenuItem），按钮与子菜单上下排列在容器中，
// 按钮的父组件随之变为该容器；按钮需要先加入布局（例如 ShadSidebarGroup::addItem）
class ShadSidebarMenuButtonPrivate;

class SHADCN_EXPORT ShadSidebarMenuButton : public QAbstractButton
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSidebarMenuButton)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(ShadSidebarMenuButtonType::Size buttonSize READ getButtonSize WRITE setButtonSize)
    Q_PROPERTY(ShadSidebarMenuButtonType::Variant variant READ getVariant WRITE setVariant)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QString badgeText READ getBadgeText WRITE setBadgeText)
    Q_PROPERTY(bool subMenuExpanded READ isSubMenuExpanded WRITE setSubMenuExpanded NOTIFY subMenuExpandedChanged)
    Q_PROPERTY(bool selectable READ isSelectable WRITE setSelectable)
    Q_PROPERTY(bool open READ isOpen WRITE setOpen)
    friend class ShadSidebarPrivate;

    public:
        explicit ShadSidebarMenuButton(QWidget* parent = nullptr);
        ShadSidebarMenuButton(ShadIconType::IconName icon, const QString& text, QWidget* parent = nullptr);
        ~ShadSidebarMenuButton() override;

        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        // 第二行文字（text-xs），主要用于 Large 尺寸
        void setDescription(const QString& description);
        QString getDescription() const;

        void setButtonSize(ShadSidebarMenuButtonType::Size size);
        ShadSidebarMenuButtonType::Size getButtonSize() const;
        void setVariant(ShadSidebarMenuButtonType::Variant variant);
        ShadSidebarMenuButtonType::Variant getVariant() const;

        // 选中状态（data-active）；用户点击时由所属侧栏保证只有一个选中项
        void setActive(bool active);
        bool isActive() const;
        // 点击后是否成为选中项，默认 true；false 时只发出 clicked，用于菜单触发按钮
        void setSelectable(bool selectable);
        bool isSelectable() const;
        // 关联的菜单是否打开（data-state=open），打开时显示 bg-sidebar-accent，默认 false
        void setOpen(bool open);
        bool isOpen() const;

        // Large 尺寸下代替图标方块的头像，按钮接管父子关系，替换时删除旧头像，nullptr 为移除
        // 头像不响应鼠标，其他尺寸与子菜单项不显示
        void setAvatar(ShadAvatar* avatar);
        ShadAvatar* getAvatar() const;

        // 结束侧的徽章文字，为空时不显示
        void setBadgeText(const QString& text);
        QString getBadgeText() const;

        // 结束侧的操作按钮图标，传 ShadIconType::None 隐藏；点击时发出 actionClicked，不触发按钮本身
        void setActionIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getActionIcon() const;
        // 操作按钮是否始终显示，默认 false（对应 showOnHover，悬停、聚焦或选中时显示）
        void setActionAlwaysVisible(bool alwaysVisible);
        bool isActionAlwaysVisible() const;
        // 操作按钮在本组件中的区域，可用于在其旁边弹出菜单；没有操作按钮时为空矩形
        QRect getActionRect() const;

        // 添加子菜单项（SidebarMenuSubButton，高 28），返回的按钮同样可设置选中状态
        ShadSidebarMenuButton* addSubItem(const QString& text, ShadIconType::IconName icon = ShadIconType::None);
        bool hasSubItems() const;
        // 子菜单的展开状态，默认折叠
        void setSubMenuExpanded(bool expanded);
        bool isSubMenuExpanded() const;
        // 是否为子菜单项
        bool isSubItem() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void activeChanged(bool active);
        void actionClicked();
        void subMenuExpandedChanged(bool expanded);

    protected:
        bool event(QEvent* event) override;
        bool hitButton(const QPoint& pos) const override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
};

#endif // SHADSIDEBARMENUBUTTON_H
