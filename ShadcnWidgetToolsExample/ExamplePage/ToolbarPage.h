#ifndef TOOLBARPAGE_H
#define TOOLBARPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class ToolbarPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ToolbarPage(QWidget* parent = nullptr);
        ~ToolbarPage() override;
};

#endif // TOOLBARPAGE_H
