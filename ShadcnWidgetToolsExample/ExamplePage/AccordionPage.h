#ifndef ACCORDIONPAGE_H
#define ACCORDIONPAGE_H

#include "ExampleBasePage.h"

// 手风琴示例
class AccordionPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit AccordionPage(QWidget* parent = nullptr);
        ~AccordionPage() override;
};

#endif // ACCORDIONPAGE_H
