#ifndef SIGNATUREPADPAGE_H
#define SIGNATUREPADPAGE_H

#include "ExampleBasePage.h"

class SignaturePadPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit SignaturePadPage(QWidget* parent = nullptr);
        ~SignaturePadPage() override;
};

#endif
