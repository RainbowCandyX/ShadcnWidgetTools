#ifndef COLLAPSIBLEPAGE_H
#define COLLAPSIBLEPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// Collapsible 示例
class CollapsiblePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit CollapsiblePage(QWidget* parent = nullptr);
        ~CollapsiblePage() override;
};

#endif // COLLAPSIBLEPAGE_H
