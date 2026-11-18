#ifndef LOADINGOVERLAYPAGE_H
#define LOADINGOVERLAYPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class LoadingOverlayPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit LoadingOverlayPage(QWidget* parent = nullptr);
        ~LoadingOverlayPage() override;
};

#endif // LOADINGOVERLAYPAGE_H
