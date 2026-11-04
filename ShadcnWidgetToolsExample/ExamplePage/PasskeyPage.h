#ifndef PASSKEYPAGE_H
#define PASSKEYPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class PasskeyPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit PasskeyPage(QWidget* parent = nullptr);
        ~PasskeyPage() override;
};

#endif
