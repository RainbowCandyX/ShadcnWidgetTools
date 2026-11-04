#ifndef PROGRESSPAGE_H
#define PROGRESSPAGE_H

#include "ExampleBasePage.h"

// Progress / Slider 示例
class ProgressPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ProgressPage(QWidget* parent = nullptr);
        ~ProgressPage() override;
};

#endif // PROGRESSPAGE_H
