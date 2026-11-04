#ifndef RADIOGROUPPAGE_H
#define RADIOGROUPPAGE_H

#include "ExampleBasePage.h"

class RadioGroupPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit RadioGroupPage(QWidget* parent = nullptr);
        ~RadioGroupPage() override;
};

#endif // RADIOGROUPPAGE_H
