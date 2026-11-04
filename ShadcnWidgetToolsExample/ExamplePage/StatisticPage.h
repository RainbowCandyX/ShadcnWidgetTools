#ifndef STATISTICPAGE_H
#define STATISTICPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class StatisticPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit StatisticPage(QWidget* parent = nullptr);
        ~StatisticPage() override;
};

#endif
