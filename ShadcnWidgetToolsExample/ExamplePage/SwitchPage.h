#ifndef SWITCHPAGE_H
#define SWITCHPAGE_H

#include "ExampleBasePage.h"

class SwitchPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit SwitchPage(QWidget* parent = nullptr);
        ~SwitchPage() override;
};

#endif // SWITCHPAGE_H
