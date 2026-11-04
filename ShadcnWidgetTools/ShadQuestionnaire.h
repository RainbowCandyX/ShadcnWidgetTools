#ifndef SHADQUESTIONNAIRE_H
#define SHADQUESTIONNAIRE_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QKeyEvent;

// 问卷：一次显示一道题
class ShadQuestionnairePrivate;

class SHADCN_EXPORT ShadQuestionnaire : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadQuestionnaire)
    Q_PROPERTY(ShadQuestionnaireType::Shortcuts shortcuts READ getShortcuts WRITE setShortcuts)
    Q_PROPERTY(bool progressVisible READ isProgressVisible WRITE setProgressVisible)
    Q_PROPERTY(QString progressFormat READ getProgressFormat WRITE setProgressFormat)
    Q_PROPERTY(QString currentItem READ getCurrentItem WRITE setCurrentItem NOTIFY currentItemChanged)

    public:
        explicit ShadQuestionnaire(QWidget* parent = nullptr);
        ~ShadQuestionnaire() override;

        // name 重复时忽略
        void addItem(const QString& name, const QString& title, const QString& description = QString());
        QStringList getItems() const;
        void setItemRequired(const QString& name, bool required);
        bool isItemRequired(const QString& name) const;
        void setItemMultiple(const QString& name, bool multiple);
        bool isItemMultiple(const QString& name) const;
        // 禁用的题不参与导航
        void setItemDisabled(const QString& name, bool disabled);
        bool isItemDisabled(const QString& name) const;

        // 修改已添加的题目标题与说明，例如切换语言时；name 不存在时不处理
        void setItemTitle(const QString& name, const QString& title);
        void setItemDescription(const QString& name, const QString& description);

        void addChoice(const QString& name, const QString& value, const QString& label, const QString& description = QString());
        void setChoiceDisabled(const QString& name, const QString& value, bool disabled);
        // 修改已添加选项的文字与说明，作答状态不变；name 或 value 不存在时不处理
        void setChoiceLabel(const QString& name, const QString& value, const QString& label);
        void setChoiceDescription(const QString& name, const QString& value, const QString& description);
        void setItemInput(const QString& name, const QString& placeholder);

        void setShortcuts(ShadQuestionnaireType::Shortcuts shortcuts);
        ShadQuestionnaireType::Shortcuts getShortcuts() const;

        void setProgressVisible(bool visible);
        bool isProgressVisible() const;
        // %1 为当前题号，%2 为题数
        void setProgressFormat(const QString& format);
        QString getProgressFormat() const;

        void setPreviousText(const QString& text);
        QString getPreviousText() const;
        void setSkipText(const QString& text);
        QString getSkipText() const;
        void setNextText(const QString& text);
        QString getNextText() const;
        void setSubmitText(const QString& text);
        QString getSubmitText() const;

        void setCurrentItem(const QString& name);
        QString getCurrentItem() const;
        // 没有题时为 -1
        int getCurrentIndex() const;
        int getItemCount() const;

        ShadQuestionnaireType::ItemStatus getItemStatus(const QString& name) const;
        // 选项为 value，输入框为文本
        QStringList getAnswers(const QString& name) const;
        void setAnswers(const QString& name, const QStringList& answers);
        // 不含禁用与跳过的题
        QHash<QString, QStringList> getAllAnswers() const;

        // message 为空时清除
        void setItemError(const QString& name, const QString& message);
        QString getItemError(const QString& name) const;

    public Q_SLOTS:
        void previous();
        void next();
        void skip();
        // 全部通过时发出 submitted
        void submit();
        void reset();

    Q_SIGNALS:
        void currentItemChanged(const QString& name);
        void itemStatusChanged(const QString& name, ShadQuestionnaireType::ItemStatus status);
        void answersChanged(const QString& name);
        void submitted();

    protected:
        void changeEvent(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        // 设置按钮文字与进度格式的默认值，使用者改过的保持不变
        void retranslate();
};

#endif // SHADQUESTIONNAIRE_H
