#ifndef AUTOCOMPLETEPAGE_H
#define AUTOCOMPLETEPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class AutoCompletePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit AutoCompletePage(QWidget* parent = nullptr);
        ~AutoCompletePage() override;
};

#endif // AUTOCOMPLETEPAGE_H
