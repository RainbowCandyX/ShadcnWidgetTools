#ifndef TREESELECTPAGE_H
#define TREESELECTPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// TreeSelect（树形下拉选择）示例
class TreeSelectPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit TreeSelectPage(QWidget* parent = nullptr);
        ~TreeSelectPage() override;
};

#endif // TREESELECTPAGE_H
