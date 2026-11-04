#ifndef NAVIGATIONMENUPAGE_H
#define NAVIGATIONMENUPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// NavigationMenu 示例
class NavigationMenuPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit NavigationMenuPage(QWidget* parent = nullptr);
        ~NavigationMenuPage() override;
};

#endif // NAVIGATIONMENUPAGE_H
