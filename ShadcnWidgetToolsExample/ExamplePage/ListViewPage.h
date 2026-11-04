#ifndef LISTVIEWPAGE_H
#define LISTVIEWPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// 虚拟列表示例
class ListViewPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ListViewPage(QWidget* parent = nullptr);
        ~ListViewPage() override;
};

#endif // LISTVIEWPAGE_H
