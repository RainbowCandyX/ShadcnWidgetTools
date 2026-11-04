#ifndef EXPANDERPAGE_H
#define EXPANDERPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// Expander 示例
class ExpanderPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ExpanderPage(QWidget* parent = nullptr);
        ~ExpanderPage() override;
};

#endif // EXPANDERPAGE_H
