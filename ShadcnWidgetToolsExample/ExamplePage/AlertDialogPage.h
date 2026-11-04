#ifndef ALERTDIALOGPAGE_H
#define ALERTDIALOGPAGE_H

#include "ExampleBasePage.h"

// 警告对话框示例：基础用法、小尺寸、媒体区、危险操作、在对话框中打开与静态函数
class AlertDialogPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit AlertDialogPage(QWidget* parent = nullptr);
        ~AlertDialogPage() override;
};

#endif // ALERTDIALOGPAGE_H
