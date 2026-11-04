#ifndef TREEVIEWPAGE_H
#define TREEVIEWPAGE_H

#include "ExampleBasePage.h"

// 树形视图示例：文件树、复选框、多列与表头、拖放排序、样式选项与 RTL
class TreeViewPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit TreeViewPage(QWidget* parent = nullptr);
        ~TreeViewPage() override;
};

#endif // TREEVIEWPAGE_H
