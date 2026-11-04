#ifndef COLORPICKERPAGE_H
#define COLORPICKERPAGE_H

#include "ExampleBasePage.h"

// 颜色选择器示例：内嵌面板、对话框、静态函数、自定义预设与 RTL
class ColorPickerPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ColorPickerPage(QWidget* parent = nullptr);
        ~ColorPickerPage() override;
};

#endif // COLORPICKERPAGE_H
