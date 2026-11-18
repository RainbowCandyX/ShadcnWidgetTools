#ifndef LOGVIEWPAGE_H
#define LOGVIEWPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class LogViewPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit LogViewPage(QWidget* parent = nullptr);
        ~LogViewPage() override;
};

#endif // LOGVIEWPAGE_H
