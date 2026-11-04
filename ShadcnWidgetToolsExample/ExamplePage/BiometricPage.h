#ifndef BIOMETRICPAGE_H
#define BIOMETRICPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class BiometricPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit BiometricPage(QWidget* parent = nullptr);
        ~BiometricPage() override;
};

#endif
