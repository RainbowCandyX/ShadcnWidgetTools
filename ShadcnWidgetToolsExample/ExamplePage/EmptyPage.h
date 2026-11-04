#ifndef EMPTYPAGE_H
#define EMPTYPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class EmptyPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit EmptyPage(QWidget* parent = nullptr);
        ~EmptyPage() override;
};

#endif // EMPTYPAGE_H
