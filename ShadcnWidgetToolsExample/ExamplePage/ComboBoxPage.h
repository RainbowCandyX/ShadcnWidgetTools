#ifndef COMBOBOXPAGE_H
#define COMBOBOXPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// ComboBox（Select / Combobox）示例
class ComboBoxPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ComboBoxPage(QWidget* parent = nullptr);
        ~ComboBoxPage() override;
};

#endif // COMBOBOXPAGE_H
