#ifndef ATTACHMENTPAGE_H
#define ATTACHMENTPAGE_H

#include "ExampleBasePage.h"

class ShadToaster;

// 附件示例：操作按钮与可点击卡片通过主窗口的 toaster 给出反馈
class AttachmentPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        AttachmentPage(ShadToaster* toaster, QWidget* parent = nullptr);
        ~AttachmentPage() override;
};

#endif // ATTACHMENTPAGE_H
