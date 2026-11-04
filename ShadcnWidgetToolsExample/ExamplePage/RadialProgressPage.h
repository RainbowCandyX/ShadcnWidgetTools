#ifndef RADIALPROGRESSPAGE_H
#define RADIALPROGRESSPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class RadialProgressPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit RadialProgressPage(QWidget* parent = nullptr);
        ~RadialProgressPage() override;
};

#endif
