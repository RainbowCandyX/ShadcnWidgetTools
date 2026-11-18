#ifndef TAGINPUTPAGE_H
#define TAGINPUTPAGE_H

#include <QString>

#include "ExampleBasePage.h"

class QWidget;

class TagInputPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit TagInputPage(QWidget* parent = nullptr);
        ~TagInputPage() override;

    private:
        QString _rejectedTag;
};

#endif // TAGINPUTPAGE_H
