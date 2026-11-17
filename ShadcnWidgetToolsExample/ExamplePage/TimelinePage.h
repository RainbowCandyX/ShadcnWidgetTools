#ifndef TIMELINEPAGE_H
#define TIMELINEPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class TimelinePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit TimelinePage(QWidget* parent = nullptr);
        ~TimelinePage() override;
};

#endif // TIMELINEPAGE_H
