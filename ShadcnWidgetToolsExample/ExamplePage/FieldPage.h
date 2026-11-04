#ifndef FIELDPAGE_H
#define FIELDPAGE_H

#include "ExampleBasePage.h"

// 表单项示例：付款表单、各类控件、分组、选择卡片、分隔线、三种方向与校验
class FieldPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit FieldPage(QWidget* parent = nullptr);
        ~FieldPage() override;
};

#endif // FIELDPAGE_H
