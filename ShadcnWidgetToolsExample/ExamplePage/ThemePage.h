#ifndef THEMEPAGE_H
#define THEMEPAGE_H

#include "ExampleBasePage.h"

// 主题示例：基色选择、当前全部颜色 token、五种基色对比
class ThemePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ThemePage(QWidget* parent = nullptr);
        ~ThemePage() override;
};

#endif // THEMEPAGE_H
