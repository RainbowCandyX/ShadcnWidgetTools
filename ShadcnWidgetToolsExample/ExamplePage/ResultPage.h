#ifndef RESULTPAGE_H
#define RESULTPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class ResultPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ResultPage(QWidget* parent = nullptr);
        ~ResultPage() override;
};

#endif // RESULTPAGE_H
