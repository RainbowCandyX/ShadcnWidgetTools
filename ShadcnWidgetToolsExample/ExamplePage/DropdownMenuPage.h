#ifndef DROPDOWNMENUPAGE_H
#define DROPDOWNMENUPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// DropdownMenu 示例
class DropdownMenuPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit DropdownMenuPage(QWidget* parent = nullptr);
        ~DropdownMenuPage() override;
};

#endif // DROPDOWNMENUPAGE_H
