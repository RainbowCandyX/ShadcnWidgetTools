#ifndef MARKDOWNVIEWPAGE_H
#define MARKDOWNVIEWPAGE_H

#include "ExampleBasePage.h"

class MarkdownViewPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit MarkdownViewPage(QWidget* parent = nullptr);
        ~MarkdownViewPage() override;
};

#endif // MARKDOWNVIEWPAGE_H
