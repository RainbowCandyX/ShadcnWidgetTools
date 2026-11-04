#ifndef TABSPAGE_H
#define TABSPAGE_H

#include "ExampleBasePage.h"

// 标签页示例：基础用法、线条样式、纵向、禁用、图标与键盘切换
class TabsPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit TabsPage(QWidget* parent = nullptr);
        ~TabsPage() override;
};

#endif // TABSPAGE_H
