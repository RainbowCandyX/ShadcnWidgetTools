#ifndef SHADSIDEBARTRIGGER_H
#define SHADSIDEBARTRIGGER_H

#include "ShadButton.h"
#include "ShadProperty.h"

class QEvent;
class ShadSidebar;

// 侧栏切换按钮，对应 SidebarTrigger：ghost 变体 icon-sm，点击切换绑定侧栏的展开状态
// 图标为 PanelLeft；侧栏实际显示在右侧时（结束侧且 LTR，或起始侧且 RTL）使用 PanelRight
class ShadSidebarTriggerPrivate;

class SHADCN_EXPORT ShadSidebarTrigger : public ShadButton
{
    Q_OBJECT

    public:
        explicit ShadSidebarTrigger(QWidget* parent = nullptr);
        explicit ShadSidebarTrigger(ShadSidebar* sidebar, QWidget* parent = nullptr);
        ~ShadSidebarTrigger() override;

        void setSidebar(ShadSidebar* sidebar);
        ShadSidebar* getSidebar() const;

    protected:
        bool event(QEvent* event) override;

    private:
        void updateIcon();

        QScopedPointer<ShadSidebarTriggerPrivate> _private;
};

#endif // SHADSIDEBARTRIGGER_H
