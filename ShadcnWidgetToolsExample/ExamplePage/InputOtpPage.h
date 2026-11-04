#ifndef INPUTOTPPAGE_H
#define INPUTOTPPAGE_H

#include <QString>

#include "ExampleBasePage.h"
#include "ExampleI18n.h"

class QWidget;
class ShadToaster;

// 验证码输入示例：基础用法、仅数字、分隔符、字母与数字、禁用、4 位 PIN、错误态与表单
class InputOtpPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        InputOtpPage(ShadToaster* toaster, QWidget* parent = nullptr);
        ~InputOtpPage() override;

    private:
        // 标签在上、验证码输入在中间、说明文字在下的表单项；isError 为 true 时说明文字为 destructive 色（对应 FieldError）
        // 标签与说明随语言刷新，description 为空函数时不显示说明
        QWidget* createField(const ExampleI18n::TextFunction& labelText, QWidget* field, const ExampleI18n::TextFunction& description = ExampleI18n::TextFunction(), bool isError = false);
};

#endif // INPUTOTPPAGE_H
