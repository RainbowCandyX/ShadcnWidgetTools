#ifndef TABLEPAGE_H
#define TABLEPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// Table 示例
class TablePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit TablePage(QWidget* parent = nullptr);
        ~TablePage() override;
};

#endif // TABLEPAGE_H
