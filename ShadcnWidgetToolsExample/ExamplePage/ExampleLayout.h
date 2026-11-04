#ifndef EXAMPLELAYOUT_H
#define EXAMPLELAYOUT_H

#include <QDialog>
#include <QPointer>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"

// 示例页面共用的布局常量与辅助函数
namespace ExampleLayout
{
    // 与组件库可聚焦组件四周预留的焦点环边距一致，用于对齐可视边缘
    constexpr int kFocusRingWidth = 3;
    constexpr int kFieldWidth = 320;
    constexpr int kWideFieldWidth = 420;

    // 在窗口现有的标题栏按钮之外显示置顶按钮；适用于 ShadWindow、ShadWidget 与 ShadDialog
    // 示例程序中的窗口与对话框都显示置顶按钮，应在 setButtonFlags 之后调用
    template <typename Window>
    void addStayOnTopButton(Window* window)
    {
        window->setButtonFlags(window->getButtonFlags() | ShadWindowType::StayOnTopButton);
    }

    // 示例中的窗口与对话框都在点击时新建、关闭时销毁，每次打开都是初始状态

    // 打开新建的对话框，关闭后销毁
    inline void openTransientDialog(QDialog* dialog)
    {
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->open();
    }

    // 打开独立窗口：已开着则提到最前，否则新建并居中
    template <typename Window, typename Create>
    void presentTransientWindow(QPointer<Window>& window, Create create)
    {
        if (window.isNull())
        {
            window = create();
            window->setAttribute(Qt::WA_DeleteOnClose);
            window->moveToCenter();
        }
        // WebAssembly 上 show() 会全屏显示顶层窗口
        window->setVisible(true);
        window->raise();
        window->activateWindow();
    }
} // namespace ExampleLayout

#endif // EXAMPLELAYOUT_H
