#ifndef LABELPAGE_H
#define LABELPAGE_H

#include "ExampleBasePage.h"

// 标签示例
class LabelPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit LabelPage(QWidget* parent = nullptr);
        ~LabelPage() override;
};

#endif // LABELPAGE_H
