#ifndef SHADSIDEBARLAYOUT_H
#define SHADSIDEBARLAYOUT_H

#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QObject;
class QPaintEvent;
class ShadSidebar;

// 侧栏与主内容的容器，对应 SidebarProvider + SidebarInset：侧栏放在起始侧或结束侧（跟随侧栏的 side），主内容占据其余空间
// 主内容区为 bg-background；侧栏为 Inset 变体时容器铺满 bg-sidebar，主内容区四周留 8px（靠侧栏一侧由侧栏自身的 p-2 提供，
// 侧栏完全收起时补上），显示为 rounded-xl shadow-sm 的卡片
// Ctrl+B（macOS 为 Cmd+B）在容器所在窗口内切换侧栏展开状态，可用 setShortcutEnabled(false) 关闭
class ShadSidebarLayoutPrivate;

class SHADCN_EXPORT ShadSidebarLayout : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSidebarLayout)
    Q_PROPERTY(bool shortcutEnabled READ isShortcutEnabled WRITE setShortcutEnabled)

    public:
        explicit ShadSidebarLayout(QWidget* parent = nullptr);
        ~ShadSidebarLayout() override;

        // 替换侧栏，旧侧栏会被删除，传 nullptr 只移除
        void setSidebar(ShadSidebar* sidebar);
        ShadSidebar* getSidebar() const;

        // 替换主内容组件，旧组件会被删除，传 nullptr 只移除
        void setContentWidget(QWidget* widget);
        QWidget* getContentWidget() const;

        void setShortcutEnabled(bool enabled);
        bool isShortcutEnabled() const;

    protected:
        bool event(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADSIDEBARLAYOUT_H
