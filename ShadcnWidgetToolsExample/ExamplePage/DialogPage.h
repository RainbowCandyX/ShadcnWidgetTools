#ifndef DIALOGPAGE_H
#define DIALOGPAGE_H

#include "ExampleBasePage.h"

// 对话框示例：基础用法、自定义关闭按钮、无关闭按钮、粘性页脚、可滚动内容、继承 ShadDialog、exec 返回值与无父窗口
class DialogPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit DialogPage(QWidget* parent = nullptr);
        ~DialogPage() override;
};

#endif // DIALOGPAGE_H
