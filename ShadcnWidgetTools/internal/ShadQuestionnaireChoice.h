#ifndef SHADQUESTIONNAIRECHOICE_H
#define SHADQUESTIONNAIRECHOICE_H

#include <QAbstractButton>
#include <QSize>
#include <QString>

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QPaintEvent;
class ShadTransition;

// ShadQuestionnaire 的选项卡片
class ShadQuestionnaireChoice : public QAbstractButton
{
    Q_OBJECT

    public:
        explicit ShadQuestionnaireChoice(QWidget* parent = nullptr);
        ~ShadQuestionnaireChoice() override;

        void setLabel(const QString& label);
        QString getLabel() const;
        void setDescription(const QString& description);
        QString getDescription() const;
        // 单选时再次点击不取消
        void setRadio(bool radio);
        bool isRadio() const;
        void setShortcutText(const QString& text);
        QString getShortcutText() const;
        void setInvalid(bool invalid);
        bool isInvalid() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;
        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;

    protected:
        void nextCheckState() override;
        // 方向键与回车交给外层处理
        void keyPressEvent(QKeyEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        int chromeWidth() const;
        int textWidth(int width) const;
        int textHeight(int width) const;
        QFont labelFont() const;
        QFont shortcutFont() const;

        QString _label;
        QString _description;
        QString _shortcutText;
        bool _isRadio{true};
        bool _isInvalid{false};
        bool _isFocusVisible{false};
        ShadTransition* _hoverTransition{nullptr};
};

#endif // SHADQUESTIONNAIRECHOICE_H
