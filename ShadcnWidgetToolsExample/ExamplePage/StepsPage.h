#ifndef STEPSPAGE_H
#define STEPSPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class StepsPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit StepsPage(QWidget* parent = nullptr);
        ~StepsPage() override;
};

#endif // STEPSPAGE_H
