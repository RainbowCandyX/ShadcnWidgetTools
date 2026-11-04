#ifndef DESCRIPTIONSPAGE_H
#define DESCRIPTIONSPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// Descriptions 示例
class DescriptionsPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit DescriptionsPage(QWidget* parent = nullptr);
        ~DescriptionsPage() override;
};

#endif // DESCRIPTIONSPAGE_H
