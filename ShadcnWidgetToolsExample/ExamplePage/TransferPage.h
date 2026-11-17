#ifndef TRANSFERPAGE_H
#define TRANSFERPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class TransferPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit TransferPage(QWidget* parent = nullptr);
        ~TransferPage() override;
};

#endif // TRANSFERPAGE_H
