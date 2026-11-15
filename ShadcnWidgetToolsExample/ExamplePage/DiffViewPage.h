#ifndef DIFFVIEWPAGE_H
#define DIFFVIEWPAGE_H

#include "ExampleBasePage.h"

// 差异对比示例
class DiffViewPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit DiffViewPage(QWidget* parent = nullptr);
        ~DiffViewPage() override;
};

#endif // DIFFVIEWPAGE_H
