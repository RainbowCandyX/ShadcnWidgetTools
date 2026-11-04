#ifndef COMMANDPAGE_H
#define COMMANDPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// Command 示例
class CommandPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit CommandPage(QWidget* parent = nullptr);
        ~CommandPage() override;
};

#endif // COMMANDPAGE_H
