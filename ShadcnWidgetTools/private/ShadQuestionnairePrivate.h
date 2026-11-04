#ifndef SHADQUESTIONNAIREPRIVATE_H
#define SHADQUESTIONNAIREPRIVATE_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadProperty.h"

class QWidget;
class ShadButton;
class ShadLabel;
class ShadQuestionnaire;
class ShadQuestionnaireChoice;
class ShadQuestionnaireItemView;
class ShadStackedWidget;

class ShadQuestionnairePrivate
{
    SHAD_D_CREATE(ShadQuestionnaire)

    public:
        ShadQuestionnairePrivate();
        ~ShadQuestionnairePrivate();

        static constexpr int kSectionGap = 24;
        static constexpr int kActionGap = 8;

        struct Choice
        {
            QString value;
            ShadQuestionnaireChoice* widget{nullptr};
        };

        struct Item
        {
            QString name;
            bool isRequired{false};
            bool isMultiple{false};
            bool isDisabled{false};
            QList<Choice> choices;
            bool isInputSelected{false};
            bool isSkipped{false};
            bool isValidationAttempted{false};
            QString error;
            ShadQuestionnaireType::ItemStatus lastStatus{ShadQuestionnaireType::Unanswered};
            QStringList lastAnswers;
            ShadQuestionnaireItemView* view{nullptr};
        };

        Item* findItem(const QString& name);
        const Item* findItem(const QString& name) const;
        QList<Item*> logicalItems();
        QList<const Item*> logicalItems() const;
        Item* currentItem();
        int currentIndex() const;

        ShadQuestionnaireType::ItemStatus status(const Item& item) const;
        bool isValid(const Item& item) const;
        QStringList answers(const Item& item) const;
        QList<QWidget*> answerControls(const Item& item) const;

        void createItemView(Item& item);
        void onChoiceClicked(Item& item, ShadQuestionnaireChoice* widget);
        void onInputChanged(Item& item, const QString& text);
        void clearSelection(Item& item);
        void refreshItem(Item& item);
        bool validate(Item& item);
        void focusFirstAnswer(const Item& item);
        void updateShortcuts(Item& item);
        void setCurrent(const QString& name, bool moveFocus);
        void ensureCurrent();
        void updateNavigation();
        void confirm();
        bool handleShortcut(const QString& key);
        bool moveAnswerFocus(bool forward);

        QList<Item> _items;
        QString _currentName;
        ShadQuestionnaireType::Shortcuts _shortcuts{ShadQuestionnaireType::NoShortcuts};
        bool _isProgressVisible{true};
        QString _progressFormat;
        ShadLabel* _progressLabel{nullptr};
        ShadStackedWidget* _stack{nullptr};
        QWidget* _actions{nullptr};
        ShadButton* _previousButton{nullptr};
        ShadButton* _skipButton{nullptr};
        ShadButton* _nextButton{nullptr};
        ShadButton* _submitButton{nullptr};
        // 程序修改答案时忽略控件信号
        bool _isSyncing{false};
};

#endif // SHADQUESTIONNAIREPRIVATE_H
