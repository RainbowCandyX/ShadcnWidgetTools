#ifndef BREADCRUMBPAGE_H
#define BREADCRUMBPAGE_H

#include "ExampleBasePage.h"

class ShadToaster;

// 面包屑示例：点击链接的反馈通过主窗口的 toaster 显示
class BreadcrumbPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        BreadcrumbPage(ShadToaster* toaster, QWidget* parent = nullptr);
        ~BreadcrumbPage() override;
};

#endif // BREADCRUMBPAGE_H
