#ifndef INPUTDIALOGPAGE_H
#define INPUTDIALOGPAGE_H

#include "ExampleBasePage.h"

// 输入对话框示例：单行文本、密码、多行文本、整数与小数
class InputDialogPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit InputDialogPage(QWidget* parent = nullptr);
        ~InputDialogPage() override;
};

#endif // INPUTDIALOGPAGE_H
