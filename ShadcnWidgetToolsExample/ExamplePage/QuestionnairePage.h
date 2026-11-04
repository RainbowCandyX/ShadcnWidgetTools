#ifndef QUESTIONNAIREPAGE_H
#define QUESTIONNAIREPAGE_H

#include "ExampleBasePage.h"

// 问卷示例
class QuestionnairePage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit QuestionnairePage(QWidget* parent = nullptr);
        ~QuestionnairePage() override;
};

#endif // QUESTIONNAIREPAGE_H
