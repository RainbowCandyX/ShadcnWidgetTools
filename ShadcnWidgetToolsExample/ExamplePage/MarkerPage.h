#ifndef MARKERPAGE_H
#define MARKERPAGE_H

#include "ExampleBasePage.h"

class MarkerPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit MarkerPage(QWidget* parent = nullptr);
        ~MarkerPage() override;
};

#endif // MARKERPAGE_H
