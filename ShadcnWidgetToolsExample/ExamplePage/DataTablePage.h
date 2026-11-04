#ifndef DATATABLEPAGE_H
#define DATATABLEPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// Data Table 示例
class DataTablePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit DataTablePage(QWidget* parent = nullptr);
        ~DataTablePage() override;
};

#endif // DATATABLEPAGE_H
