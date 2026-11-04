#ifndef TRACEPAGE_H
#define TRACEPAGE_H

#include "ExampleBasePage.h"

// 执行记录示例：单行、状态、描述、展开、链接、浮层、来源、思维链、提交与完整回合
class TracePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit TracePage(QWidget* parent = nullptr);
        ~TracePage() override;
};

#endif // TRACEPAGE_H
