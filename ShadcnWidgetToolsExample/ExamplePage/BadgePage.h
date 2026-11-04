#ifndef BADGEPAGE_H
#define BADGEPAGE_H

#include "ExampleBasePage.h"

class BadgePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit BadgePage(QWidget* parent = nullptr);
        ~BadgePage() override;
};

#endif // BADGEPAGE_H
