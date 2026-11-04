#ifndef SHADINPUTDIALOG_H
#define SHADINPUTDIALOG_H

#include <QLineEdit>
#include <QScopedPointer>
#include <QString>

#include "ShadDef.h"
#include "ShadDialog.h"
#include "ShadProperty.h"

class QEvent;
class QWidget;

// 输入对话框：在 ShadDialog 的标题区下方显示一个带标签的输入控件（ShadField），页脚为取消（outline）与确定按钮
// 标题与描述使用 ShadDialog 的 setTitle / setDescription；标题栏、遮罩、动画与 RTL 行为和 ShadDialog 相同
// 尺寸不能调整：宽度默认 448（第一次打开前可调用 resize 修改），每次打开时高度按当前模式的内容计算，显示后固定
// 输入模式决定输入控件，切换模式时替换控件，文本、整数与小数的值分别保留
// 单行、整数与小数模式下按 Enter 确定，多行模式下 Enter 换行；Esc 取消
// 静态函数 getText / getMultiLineText / getInt / getDouble 以应用模态打开并阻塞，关闭后删除对话框并返回结果
class ShadInputDialogPrivate;

class SHADCN_EXPORT ShadInputDialog : public ShadDialog
{
    Q_OBJECT
    Q_PROPERTY(ShadInputDialogType::InputMode inputMode READ getInputMode WRITE setInputMode)
    Q_PROPERTY(QString labelText READ getLabelText WRITE setLabelText)
    Q_PROPERTY(QString textValue READ getTextValue WRITE setTextValue NOTIFY textValueChanged)
    Q_PROPERTY(int intValue READ getIntValue WRITE setIntValue NOTIFY intValueChanged)
    Q_PROPERTY(double doubleValue READ getDoubleValue WRITE setDoubleValue NOTIFY doubleValueChanged)
    Q_PROPERTY(QString placeholderText READ getPlaceholderText WRITE setPlaceholderText)
    Q_PROPERTY(QLineEdit::EchoMode textEchoMode READ getTextEchoMode WRITE setTextEchoMode)
    Q_PROPERTY(QString okButtonText READ getOkButtonText WRITE setOkButtonText)
    Q_PROPERTY(QString cancelButtonText READ getCancelButtonText WRITE setCancelButtonText)

    public:
        explicit ShadInputDialog(QWidget* parent = nullptr);
        ~ShadInputDialog() override;

        // 默认 TextInput
        void setInputMode(ShadInputDialogType::InputMode mode);
        ShadInputDialogType::InputMode getInputMode() const;

        // 输入控件上方的标签，为空时不显示
        void setLabelText(const QString& text);
        QString getLabelText() const;

        // 文本值，单行与多行模式共用
        void setTextValue(const QString& text);
        QString getTextValue() const;

        // 整数值，按整数范围限制
        void setIntValue(int value);
        int getIntValue() const;

        // 小数值，按小数范围限制
        void setDoubleValue(double value);
        double getDoubleValue() const;

        // 单行与多行模式的占位文字
        void setPlaceholderText(const QString& text);
        QString getPlaceholderText() const;

        // 单行模式的回显方式，默认 Normal：Normal 时有文字显示清空按钮，Password 时显示切换密码显示的按钮
        void setTextEchoMode(QLineEdit::EchoMode mode);
        QLineEdit::EchoMode getTextEchoMode() const;

        // 整数范围，默认 -2147483647 到 2147483647；maximum 小于 minimum 时按 minimum 处理
        void setIntRange(int minimum, int maximum);
        int getIntMinimum() const;
        int getIntMaximum() const;
        // 整数步长，默认 1
        void setIntStep(int step);
        int getIntStep() const;

        // 小数范围，默认 -2147483647.0 到 2147483647.0；maximum 小于 minimum 时按 minimum 处理
        void setDoubleRange(double minimum, double maximum);
        double getDoubleMinimum() const;
        double getDoubleMaximum() const;
        // 小数位数，默认 2
        void setDoubleDecimals(int decimals);
        int getDoubleDecimals() const;
        // 小数步长，默认 1.0
        void setDoubleStep(double step);
        double getDoubleStep() const;

        // 页脚按钮文字，默认为 "确定" 与 "取消"
        void setOkButtonText(const QString& text);
        QString getOkButtonText() const;
        void setCancelButtonText(const QString& text);
        QString getCancelButtonText() const;

        // 取消或对话框被销毁时返回空字符串
        static QString getText(QWidget* parent, const QString& title, const QString& description, const QString& label, const QString& text = QString(), bool* ok = nullptr, QLineEdit::EchoMode echoMode = QLineEdit::Normal);
        static QString getMultiLineText(QWidget* parent, const QString& title, const QString& description, const QString& label, const QString& text = QString(), bool* ok = nullptr);
        // 取消或对话框被销毁时返回传入的 value
        static int getInt(QWidget* parent, const QString& title, const QString& description, const QString& label, int value = 0, int minimum = -2147483647, int maximum = 2147483647, int step = 1, bool* ok = nullptr);
        static double getDouble(QWidget* parent, const QString& title, const QString& description, const QString& label, double value = 0.0, double minimum = -2147483647.0, double maximum = 2147483647.0, int decimals = 2, bool* ok = nullptr, double step = 1.0);

        // 打开时按内容计算尺寸并固定
        void setVisible(bool visible) override;

    Q_SIGNALS:
        void textValueChanged(const QString& text);
        void intValueChanged(int value);
        void doubleValueChanged(double value);

    protected:
        void changeEvent(QEvent* event) override;

    private:
        // 设置按钮的默认文字，通过 setOkButtonText / setCancelButtonText 设置过的文字保持不变
        void retranslate();
        // 按当前模式创建输入控件并替换 ShadField 中的旧控件
        void rebuildControl();
        // 把回显方式应用到单行输入框
        void applyTextEchoMode();
        void updateTextValue(const QString& text);
        void updateIntValue(int value);
        void updateDoubleValue(double value);

        QScopedPointer<ShadInputDialogPrivate> _private;
};

#endif // SHADINPUTDIALOG_H
