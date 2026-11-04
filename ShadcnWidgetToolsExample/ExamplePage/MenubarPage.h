#ifndef MENUBARPAGE_H
#define MENUBARPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// Menubar 示例
class MenubarPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit MenubarPage(QWidget* parent = nullptr);
        ~MenubarPage() override;
};

#endif // MENUBARPAGE_H
