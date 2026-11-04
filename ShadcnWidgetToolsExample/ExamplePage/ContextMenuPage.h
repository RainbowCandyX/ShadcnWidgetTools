#ifndef CONTEXTMENUPAGE_H
#define CONTEXTMENUPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// ContextMenu 示例
class ContextMenuPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ContextMenuPage(QWidget* parent = nullptr);
        ~ContextMenuPage() override;
};

#endif // CONTEXTMENUPAGE_H
