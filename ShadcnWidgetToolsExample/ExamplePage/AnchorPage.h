#ifndef ANCHORPAGE_H
#define ANCHORPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class AnchorPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit AnchorPage(QWidget* parent = nullptr);
        ~AnchorPage() override;
};

#endif // ANCHORPAGE_H
