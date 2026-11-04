#ifndef POPOVERPAGE_H
#define POPOVERPAGE_H

#include "ExampleBasePage.h"

class PopoverPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit PopoverPage(QWidget* parent = nullptr);
        ~PopoverPage() override;
};

#endif // POPOVERPAGE_H
