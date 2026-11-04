#ifndef SIDEBARPAGE_H
#define SIDEBARPAGE_H

#include "ExampleBasePage.h"

// 侧栏示例：嵌入一个迷你应用，可切换变体、收起方式与位置
class SidebarPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit SidebarPage(QWidget* parent = nullptr);
        ~SidebarPage() override;
};

#endif // SIDEBARPAGE_H
