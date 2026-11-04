#ifndef CARDPAGE_H
#define CARDPAGE_H

#include "ExampleBasePage.h"

class CardPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit CardPage(QWidget* parent = nullptr);
        ~CardPage() override;
};

#endif // CARDPAGE_H
