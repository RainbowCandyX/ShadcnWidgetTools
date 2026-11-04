#ifndef POPCONFIRMPAGE_H
#define POPCONFIRMPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class PopconfirmPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit PopconfirmPage(QWidget* parent = nullptr);
        ~PopconfirmPage() override;
};

#endif
