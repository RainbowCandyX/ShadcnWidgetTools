#ifndef CODEBLOCKPAGE_H
#define CODEBLOCKPAGE_H

#include "ExampleBasePage.h"

// 代码块示例：标题栏、操作按钮、行号、换行、高亮行、diff、限高、尺寸、错误样式、语法高亮、ANSI 与流式输出
class CodeBlockPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit CodeBlockPage(QWidget* parent = nullptr);
        ~CodeBlockPage() override;
};

#endif // CODEBLOCKPAGE_H
