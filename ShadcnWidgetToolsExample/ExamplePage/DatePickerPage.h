#ifndef DATEPICKERPAGE_H
#define DATEPICKERPAGE_H

#include "ExampleBasePage.h"

// 日期选择示例：日历、基础用法、带下拉箭头、出生日期、日期范围、输入框、日期与时间、时间选择
class DatePickerPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit DatePickerPage(QWidget* parent = nullptr);
        ~DatePickerPage() override;
};

#endif // DATEPICKERPAGE_H
