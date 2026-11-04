#ifndef BUTTONPAGE_H
#define BUTTONPAGE_H

#include "ExampleBasePage.h"

class ShadToaster;

// 按钮与按钮组示例：按钮组中按钮与菜单项的反馈通过主窗口的 toaster 显示
class ButtonPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        ButtonPage(ShadToaster* toaster, QWidget* parent = nullptr);
        ~ButtonPage() override;

    private:
        // 对应 shadcn ButtonGroup 的各个示例
        void addButtonGroupSections(ShadToaster* toaster);
};

#endif // BUTTONPAGE_H
