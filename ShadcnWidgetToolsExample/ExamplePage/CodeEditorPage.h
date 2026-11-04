#ifndef CODEEDITORPAGE_H
#define CODEEDITORPAGE_H

#include "ExampleBasePage.h"

// 代码编辑器示例：语法高亮、切换语言、只读、隐藏行号、Tab 宽度与错误态
class CodeEditorPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit CodeEditorPage(QWidget* parent = nullptr);
        ~CodeEditorPage() override;
};

#endif // CODEEDITORPAGE_H
