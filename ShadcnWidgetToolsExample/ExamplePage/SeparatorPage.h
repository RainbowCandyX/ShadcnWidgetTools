#ifndef SEPARATORPAGE_H
#define SEPARATORPAGE_H

#include "ExampleBasePage.h"

class SeparatorPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit SeparatorPage(QWidget* parent = nullptr);
        ~SeparatorPage() override;
};

#endif // SEPARATORPAGE_H
