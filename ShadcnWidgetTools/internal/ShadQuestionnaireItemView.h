#ifndef SHADQUESTIONNAIREITEMVIEW_H
#define SHADQUESTIONNAIREITEMVIEW_H

#include <QString>
#include <QWidget>

class QVBoxLayout;
class ShadInput;
class ShadLabel;
class ShadQuestionnaireChoice;

// ShadQuestionnaire 中一道题的页面
class ShadQuestionnaireItemView : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadQuestionnaireItemView(QWidget* parent = nullptr);
        ~ShadQuestionnaireItemView() override;

        void setTitle(const QString& title);
        void setDescription(const QString& description);
        // 插在输入框之前
        ShadQuestionnaireChoice* addChoice(const QString& label, const QString& description);
        ShadInput* ensureInput();
        ShadInput* getInput() const;
        // 为空时隐藏
        void setError(const QString& error);

    private:
        ShadLabel* _titleLabel{nullptr};
        ShadLabel* _descriptionLabel{nullptr};
        QVBoxLayout* _choicesLayout{nullptr};
        ShadInput* _input{nullptr};
        ShadLabel* _errorLabel{nullptr};
};

#endif // SHADQUESTIONNAIREITEMVIEW_H
