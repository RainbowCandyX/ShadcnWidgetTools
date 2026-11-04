#ifndef PAGINATIONPAGE_H
#define PAGINATIONPAGE_H

#include "ExampleBasePage.h"

// 分页示例：基础用法、简单、每页行数、页码跳转、只显示图标与 RTL
class PaginationPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit PaginationPage(QWidget* parent = nullptr);
        ~PaginationPage() override;
};

#endif // PAGINATIONPAGE_H
