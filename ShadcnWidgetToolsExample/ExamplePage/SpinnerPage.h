#ifndef SPINNERPAGE_H
#define SPINNERPAGE_H

#include "ExampleBasePage.h"

// 加载指示示例：基础用法、尺寸、自定义、按钮、徽章、输入组与空状态
class SpinnerPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit SpinnerPage(QWidget* parent = nullptr);
        ~SpinnerPage() override;
};

#endif // SPINNERPAGE_H
