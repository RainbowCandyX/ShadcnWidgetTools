#ifndef DATETIMEPICKERPAGE_H
#define DATETIMEPICKERPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class DateTimePickerPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit DateTimePickerPage(QWidget* parent = nullptr);
        ~DateTimePickerPage() override;
};

#endif // DATETIMEPICKERPAGE_H
