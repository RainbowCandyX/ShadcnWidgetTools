#ifndef DRAWERPAGE_H
#define DRAWERPAGE_H

#include "ExampleBasePage.h"

// 抽屉示例
class DrawerPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit DrawerPage(QWidget* parent = nullptr);
        ~DrawerPage() override;
};

#endif // DRAWERPAGE_H
