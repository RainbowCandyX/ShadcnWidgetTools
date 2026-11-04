#ifndef SHADSIDEBARGROUP_H
#define SHADSIDEBARGROUP_H

#include <QRect>
#include <QString>
#include <QWidget>

#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QVBoxLayout;
class ShadSidebarMenuButton;

// 侧栏分组，对应 SidebarGroup（p-2）：顶部为 SidebarGroupLabel（h-8 px-2 text-xs font-medium，sidebar-foreground/70），
// 下方为 SidebarMenu（gap-1）；标题行结束侧可显示 SidebarGroupAction（w-5，top-3.5 right-3）
// 可折叠时整个标题行作为触发器（悬停 bg-sidebar-accent，结束侧箭头展开时旋转 180°），菜单高度过渡 200ms
// 侧栏收起为图标时隐藏标题行与操作按钮
class ShadSidebarGroupPrivate;

class SHADCN_EXPORT ShadSidebarGroup : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSidebarGroup)
    Q_PROPERTY(QString label READ getLabel WRITE setLabel)
    Q_PROPERTY(bool collapsible READ isCollapsible WRITE setCollapsible)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    friend class ShadSidebarPrivate;

    public:
        explicit ShadSidebarGroup(QWidget* parent = nullptr);
        explicit ShadSidebarGroup(const QString& label, QWidget* parent = nullptr);
        ~ShadSidebarGroup() override;

        // 标题为空时不显示标题行
        void setLabel(const QString& label);
        QString getLabel() const;

        // 是否可折叠，默认 false；可折叠时标题行可点击，Tab 可聚焦，回车或空格切换
        void setCollapsible(bool collapsible);
        bool isCollapsible() const;
        // 展开状态，默认展开
        void setExpanded(bool expanded);
        bool isExpanded() const;

        // 标题行结束侧的操作按钮图标，传 ShadIconType::None 隐藏；点击时发出 actionClicked
        void setActionIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getActionIcon() const;
        QRect getActionRect() const;

        // 在菜单末尾添加一个菜单按钮
        ShadSidebarMenuButton* addItem(ShadIconType::IconName icon, const QString& text);
        // 在菜单末尾添加任意组件，例如 ShadInput
        void addWidget(QWidget* widget);
        QVBoxLayout* menuLayout() const;

    Q_SIGNALS:
        void expandedChanged(bool expanded);
        void actionClicked();

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADSIDEBARGROUP_H
