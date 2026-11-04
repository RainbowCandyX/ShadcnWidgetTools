#ifndef TOGGLEPAGE_H
#define TOGGLEPAGE_H

#include "ExampleBasePage.h"

class TogglePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit TogglePage(QWidget* parent = nullptr);
        ~TogglePage() override;
};

#endif // TOGGLEPAGE_H
