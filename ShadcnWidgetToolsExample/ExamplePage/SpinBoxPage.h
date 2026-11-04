#ifndef SPINBOXPAGE_H
#define SPINBOXPAGE_H

#include <QString>

#include "ExampleBasePage.h"
#include "ExampleI18n.h"

class QWidget;

// 数字输入框示例：基础用法、按钮布局、小数、前后缀、禁用与错误态、数值变化
class SpinBoxPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit SpinBoxPage(QWidget* parent = nullptr);
        ~SpinBoxPage() override;

    private:
        // 标签在上、输入框在下的表单项，对应 grid gap-3；标签文字随语言刷新
        QWidget* createField(const ExampleI18n::TextFunction& labelText, QWidget* field);
};

#endif // SPINBOXPAGE_H
