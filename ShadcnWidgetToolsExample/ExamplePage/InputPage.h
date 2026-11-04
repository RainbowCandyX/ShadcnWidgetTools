#ifndef INPUTPAGE_H
#define INPUTPAGE_H

#include <QString>

#include "ExampleBasePage.h"
#include "ExampleI18n.h"

class QWidget;

// Input / Textarea / Label 示例
class InputPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit InputPage(QWidget* parent = nullptr);
        ~InputPage() override;

    private:
        // 标签在上、输入组件在下的表单项，对应 grid gap-3
        QWidget* createField(ExampleI18n::TextFunction labelText, QWidget* field);
};

#endif // INPUTPAGE_H
