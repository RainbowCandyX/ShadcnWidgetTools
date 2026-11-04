#ifndef INPUTGROUPPAGE_H
#define INPUTGROUPPAGE_H

#include "ExampleBasePage.h"

class ShadToaster;

// 输入组示例：按钮与菜单项的反馈通过主窗口的 toaster 显示
class InputGroupPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        InputGroupPage(ShadToaster* toaster, QWidget* parent = nullptr);
        ~InputGroupPage() override;
};

#endif // INPUTGROUPPAGE_H
