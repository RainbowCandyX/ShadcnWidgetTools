#ifndef KBDPAGE_H
#define KBDPAGE_H

#include "ExampleBasePage.h"

class ShadToaster;

// 按键示例：快捷键触发后的反馈通过主窗口的 toaster 显示
class KbdPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        KbdPage(ShadToaster* toaster, QWidget* parent = nullptr);
        ~KbdPage() override;
};

#endif // KBDPAGE_H
