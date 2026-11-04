#ifndef CHECKBOXPAGE_H
#define CHECKBOXPAGE_H

#include "ExampleBasePage.h"

class CheckboxPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit CheckboxPage(QWidget* parent = nullptr);
        ~CheckboxPage() override;
};

#endif // CHECKBOXPAGE_H
