#ifndef PROPERTYGRIDPAGE_H
#define PROPERTYGRIDPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class PropertyGridPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit PropertyGridPage(QWidget* parent = nullptr);
        ~PropertyGridPage() override;
};

#endif // PROPERTYGRIDPAGE_H
