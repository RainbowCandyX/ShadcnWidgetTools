#include "ShadInputDialog.h"

#include <QDialog>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QObject>
#include <QPlainTextEdit>
#include <QPointer>
#include <QSpinBox>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadDef.h"
#include "ShadDoubleSpinBox.h"
#include "ShadField.h"
#include "ShadInput.h"
#include "ShadInputDialogPrivate.h"
#include "ShadSpinBox.h"
#include "ShadStyleHelper.h"
#include "ShadTextarea.h"

namespace
{
    // 静态函数共用：模态打开并阻塞，接受时通过 getter 取值，最后删除对话框
    // 对话框在 exec 期间被销毁（例如父窗口被删除）时按取消处理
    template <typename Value>
    Value execDialog(ShadInputDialog* dialog, bool* ok, const Value& rejectedValue, Value (ShadInputDialog::*getter)() const)
    {
        QPointer<ShadInputDialog> guard(dialog);
        const int result = dialog->exec();
        const bool isAccepted = !guard.isNull() && result == QDialog::Accepted;
        if (ok != nullptr)
        {
            *ok = isAccepted;
        }
        const Value value = isAccepted ? (guard.data()->*getter)() : rejectedValue;
        delete guard.data();
        return value;
    }

    ShadInputDialog* createDialog(QWidget* parent, const QString& title, const QString& description, const QString& label)
    {
        ShadInputDialog* dialog = new ShadInputDialog(parent);
        dialog->setTitle(title);
        dialog->setDescription(description);
        dialog->setLabelText(label);
        return dialog;
    }
} // namespace

ShadInputDialog::ShadInputDialog(QWidget* parent)
    : ShadDialog(parent), _private(new ShadInputDialogPrivate())
{
    ShadInputDialogPrivate* d = _private.data();
    d->_field = new ShadField(this);
    contentLayout()->addWidget(d->_field);

    d->_cancelButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    d->_okButton = new ShadButton(QString(), this);
    retranslate();
    // 在单行、整数与小数输入框中按 Enter 触发确定
    d->_okButton->setDefault(true);
    footerLayout()->addWidget(d->_cancelButton);
    footerLayout()->addWidget(d->_okButton);
    connect(d->_cancelButton, &ShadButton::clicked, this, &ShadInputDialog::reject);
    connect(d->_okButton, &ShadButton::clicked, this, &ShadInputDialog::accept);

    rebuildControl();
}

ShadInputDialog::~ShadInputDialog()
{
    // 私有数据先于子对象释放，断开输入控件的连接，避免子对象析构期间回调访问已释放的数据
    if (QWidget* control = _private->_field->getControl())
    {
        QObject::disconnect(control, nullptr, this, nullptr);
    }
}

void ShadInputDialog::setInputMode(ShadInputDialogType::InputMode mode)
{
    if (_private->_inputMode == mode)
    {
        return;
    }
    _private->_inputMode = mode;
    rebuildControl();
}

ShadInputDialogType::InputMode ShadInputDialog::getInputMode() const
{
    return _private->_inputMode;
}

void ShadInputDialog::setLabelText(const QString& text)
{
    _private->_field->setLabel(text);
}

QString ShadInputDialog::getLabelText() const
{
    return _private->_field->getLabel();
}

void ShadInputDialog::setTextValue(const QString& text)
{
    ShadInputDialogPrivate* d = _private.data();
    // 有对应的输入控件时写入控件，由控件的变化信号同步保存的值
    if (d->_lineEdit != nullptr)
    {
        d->_lineEdit->setText(text);
        return;
    }
    if (d->_textarea != nullptr)
    {
        if (d->_textarea->toPlainText() != text)
        {
            d->_textarea->setPlainText(text);
        }
        return;
    }
    updateTextValue(text);
}

QString ShadInputDialog::getTextValue() const
{
    return _private->_textValue;
}

void ShadInputDialog::setIntValue(int value)
{
    ShadInputDialogPrivate* d = _private.data();
    if (d->_spinBox != nullptr)
    {
        d->_spinBox->setValue(value);
        return;
    }
    updateIntValue(qBound(d->_intMinimum, value, d->_intMaximum));
}

int ShadInputDialog::getIntValue() const
{
    return _private->_intValue;
}

void ShadInputDialog::setDoubleValue(double value)
{
    ShadInputDialogPrivate* d = _private.data();
    if (d->_doubleSpinBox != nullptr)
    {
        d->_doubleSpinBox->setValue(value);
        return;
    }
    updateDoubleValue(qBound(d->_doubleMinimum, value, d->_doubleMaximum));
}

double ShadInputDialog::getDoubleValue() const
{
    return _private->_doubleValue;
}

void ShadInputDialog::setPlaceholderText(const QString& text)
{
    ShadInputDialogPrivate* d = _private.data();
    d->_placeholderText = text;
    if (d->_lineEdit != nullptr)
    {
        d->_lineEdit->setPlaceholderText(text);
    }
    if (d->_textarea != nullptr)
    {
        d->_textarea->setPlaceholderText(text);
    }
}

QString ShadInputDialog::getPlaceholderText() const
{
    return _private->_placeholderText;
}

void ShadInputDialog::setTextEchoMode(QLineEdit::EchoMode mode)
{
    if (_private->_echoMode == mode)
    {
        return;
    }
    _private->_echoMode = mode;
    applyTextEchoMode();
}

QLineEdit::EchoMode ShadInputDialog::getTextEchoMode() const
{
    return _private->_echoMode;
}

void ShadInputDialog::setIntRange(int minimum, int maximum)
{
    ShadInputDialogPrivate* d = _private.data();
    d->_intMinimum = minimum;
    d->_intMaximum = qMax(minimum, maximum);
    if (d->_spinBox != nullptr)
    {
        d->_spinBox->setRange(d->_intMinimum, d->_intMaximum);
        return;
    }
    updateIntValue(qBound(d->_intMinimum, d->_intValue, d->_intMaximum));
}

int ShadInputDialog::getIntMinimum() const
{
    return _private->_intMinimum;
}

int ShadInputDialog::getIntMaximum() const
{
    return _private->_intMaximum;
}

void ShadInputDialog::setIntStep(int step)
{
    ShadInputDialogPrivate* d = _private.data();
    d->_intStep = step;
    if (d->_spinBox != nullptr)
    {
        d->_spinBox->setSingleStep(step);
    }
}

int ShadInputDialog::getIntStep() const
{
    return _private->_intStep;
}

void ShadInputDialog::setDoubleRange(double minimum, double maximum)
{
    ShadInputDialogPrivate* d = _private.data();
    d->_doubleMinimum = minimum;
    d->_doubleMaximum = qMax(minimum, maximum);
    if (d->_doubleSpinBox != nullptr)
    {
        d->_doubleSpinBox->setRange(d->_doubleMinimum, d->_doubleMaximum);
        return;
    }
    updateDoubleValue(qBound(d->_doubleMinimum, d->_doubleValue, d->_doubleMaximum));
}

double ShadInputDialog::getDoubleMinimum() const
{
    return _private->_doubleMinimum;
}

double ShadInputDialog::getDoubleMaximum() const
{
    return _private->_doubleMaximum;
}

void ShadInputDialog::setDoubleDecimals(int decimals)
{
    ShadInputDialogPrivate* d = _private.data();
    d->_doubleDecimals = qMax(0, decimals);
    if (d->_doubleSpinBox != nullptr)
    {
        d->_doubleSpinBox->setDecimals(d->_doubleDecimals);
    }
}

int ShadInputDialog::getDoubleDecimals() const
{
    return _private->_doubleDecimals;
}

void ShadInputDialog::setDoubleStep(double step)
{
    ShadInputDialogPrivate* d = _private.data();
    d->_doubleStep = step;
    if (d->_doubleSpinBox != nullptr)
    {
        d->_doubleSpinBox->setSingleStep(step);
    }
}

double ShadInputDialog::getDoubleStep() const
{
    return _private->_doubleStep;
}

void ShadInputDialog::setOkButtonText(const QString& text)
{
    _private->_okButton->setText(text);
}

QString ShadInputDialog::getOkButtonText() const
{
    return _private->_okButton->text();
}

void ShadInputDialog::setCancelButtonText(const QString& text)
{
    _private->_cancelButton->setText(text);
}

QString ShadInputDialog::getCancelButtonText() const
{
    return _private->_cancelButton->text();
}

void ShadInputDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    ShadDialog::changeEvent(event);
}

void ShadInputDialog::retranslate()
{
    ShadButton* cancelButton = _private->_cancelButton;
    ShadButton* okButton = _private->_okButton;
    ShadStyleHelper::applyDefaultText(cancelButton, "text", cancelButton->text(), tr("取消"), [cancelButton](const QString& text) { cancelButton->setText(text); });
    ShadStyleHelper::applyDefaultText(okButton, "text", okButton->text(), tr("确定"), [okButton](const QString& text) { okButton->setText(text); });
}

void ShadInputDialog::setVisible(bool visible)
{
    if (!visible || isVisible())
    {
        ShadDialog::setVisible(visible);
        return;
    }
    // 打开前解除上次固定的尺寸，由 ShadDialog 按当前模式的内容重新计算，显示后固定为计算结果
    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    ShadDialog::setVisible(true);
    if (isVisible())
    {
        setFixedSize(size());
    }
}

QString ShadInputDialog::getText(QWidget* parent, const QString& title, const QString& description, const QString& label, const QString& text, bool* ok, QLineEdit::EchoMode echoMode)
{
    ShadInputDialog * dialog = createDialog(parent, title, description, label);
    dialog->setTextEchoMode(echoMode);
    dialog->setTextValue(text);
    return execDialog<QString>(dialog, ok, QString(), &ShadInputDialog::getTextValue);
}

QString ShadInputDialog::getMultiLineText(QWidget* parent, const QString& title, const QString& description, const QString& label, const QString& text, bool* ok)
{
    ShadInputDialog * dialog = createDialog(parent, title, description, label);
    dialog->setInputMode(ShadInputDialogType::MultiLineTextInput);
    dialog->setTextValue(text);
    return execDialog<QString>(dialog, ok, QString(), &ShadInputDialog::getTextValue);
}

int ShadInputDialog::getInt(QWidget* parent, const QString& title, const QString& description, const QString& label, int value, int minimum, int maximum, int step, bool* ok)
{
    ShadInputDialog * dialog = createDialog(parent, title, description, label);
    dialog->setInputMode(ShadInputDialogType::IntInput);
    dialog->setIntRange(minimum, maximum);
    dialog->setIntStep(step);
    dialog->setIntValue(value);
    return execDialog<int>(dialog, ok, value, &ShadInputDialog::getIntValue);
}

double ShadInputDialog::getDouble(QWidget* parent, const QString& title, const QString& description, const QString& label, double value, double minimum, double maximum, int decimals, bool* ok, double step)
{
    ShadInputDialog * dialog = createDialog(parent, title, description, label);
    dialog->setInputMode(ShadInputDialogType::DoubleInput);
    dialog->setDoubleDecimals(decimals);
    dialog->setDoubleRange(minimum, maximum);
    dialog->setDoubleStep(step);
    dialog->setDoubleValue(value);
    return execDialog<double>(dialog, ok, value, &ShadInputDialog::getDoubleValue);
}

void ShadInputDialog::rebuildControl()
{
    ShadInputDialogPrivate* d = _private.data();
    // 旧控件由 ShadField 延迟删除，先断开与对话框的连接
    if (QWidget* oldControl = d->_field->getControl())
    {
        QObject::disconnect(oldControl, nullptr, this, nullptr);
    }
    d->_lineEdit = nullptr;
    d->_textarea = nullptr;
    d->_spinBox = nullptr;
    d->_doubleSpinBox = nullptr;

    QWidget* control = nullptr;
    switch (d->_inputMode)
    {
        case ShadInputDialogType::TextInput:
            {
                ShadInput* lineEdit = new ShadInput(d->_field);
                lineEdit->setPlaceholderText(d->_placeholderText);
                lineEdit->setText(d->_textValue);
                connect(lineEdit, &QLineEdit::textChanged, this, &ShadInputDialog::updateTextValue);
                d->_lineEdit = lineEdit;
                applyTextEchoMode();
                control = lineEdit;
                break;
            }
        case ShadInputDialogType::MultiLineTextInput:
            {
                ShadTextarea* textarea = new ShadTextarea(d->_field);
                // 对话框尺寸固定，输入框高度不随内容增长，内容超出时滚动
                textarea->setAutoResize(false);
                textarea->setMinimumHeight(ShadInputDialogPrivate::kMultiLineMinimumHeight + 2 * ShadStyleHelper::kFocusRingWidth);
                textarea->setPlaceholderText(d->_placeholderText);
                textarea->setPlainText(d->_textValue);
                connect(textarea, &QPlainTextEdit::textChanged, this, [this, textarea]() { updateTextValue(textarea->toPlainText()); });
                d->_textarea = textarea;
                control = textarea;
                break;
            }
        case ShadInputDialogType::IntInput:
            {
                ShadSpinBox* spinBox = new ShadSpinBox(d->_field);
                spinBox->setButtonLayout(ShadSpinBoxType::PlusMinusSide);
                spinBox->setRange(d->_intMinimum, d->_intMaximum);
                spinBox->setSingleStep(d->_intStep);
                spinBox->setValue(d->_intValue);
                // 配置完成后再连接，避免设置范围时的中间值覆盖保存的值
                connect(spinBox, &QSpinBox::valueChanged, this, &ShadInputDialog::updateIntValue);
                d->_spinBox = spinBox;
                updateIntValue(spinBox->value());
                control = spinBox;
                break;
            }
        case ShadInputDialogType::DoubleInput:
            {
                ShadDoubleSpinBox* doubleSpinBox = new ShadDoubleSpinBox(d->_field);
                doubleSpinBox->setButtonLayout(ShadSpinBoxType::PlusMinusSide);
                // 小数位数先于范围与值设置，范围与值按位数取整
                doubleSpinBox->setDecimals(d->_doubleDecimals);
                doubleSpinBox->setRange(d->_doubleMinimum, d->_doubleMaximum);
                doubleSpinBox->setSingleStep(d->_doubleStep);
                doubleSpinBox->setValue(d->_doubleValue);
                connect(doubleSpinBox, &QDoubleSpinBox::valueChanged, this, &ShadInputDialog::updateDoubleValue);
                d->_doubleSpinBox = doubleSpinBox;
                updateDoubleValue(doubleSpinBox->value());
                control = doubleSpinBox;
                break;
            }
    }
    d->_field->setControl(control);
    // 新控件位于焦点链末尾，移到页脚按钮之前：打开时焦点落在输入控件上，Tab 顺序为输入控件、取消、确定
    QWidget::setTabOrder(control, d->_cancelButton);
    QWidget::setTabOrder(d->_cancelButton, d->_okButton);
}

void ShadInputDialog::applyTextEchoMode()
{
    ShadInput* lineEdit = _private->_lineEdit;
    if (lineEdit == nullptr)
    {
        return;
    }
    const QLineEdit::EchoMode mode = _private->_echoMode;
    // PasswordToggleAction 自行把 echoMode 设为 Password，点击按钮切换显示
    if (mode == QLineEdit::Password)
    {
        lineEdit->setTrailingAction(ShadInputType::PasswordToggleAction);
        return;
    }
    lineEdit->setTrailingAction(mode == QLineEdit::Normal ? ShadInputType::ClearAction : ShadInputType::NoAction);
    lineEdit->setEchoMode(mode);
}

void ShadInputDialog::updateTextValue(const QString& text)
{
    if (_private->_textValue == text)
    {
        return;
    }
    _private->_textValue = text;
    Q_EMIT textValueChanged(text);
}

void ShadInputDialog::updateIntValue(int value)
{
    if (_private->_intValue == value)
    {
        return;
    }
    _private->_intValue = value;
    Q_EMIT intValueChanged(value);
}

void ShadInputDialog::updateDoubleValue(double value)
{
    if (_private->_doubleValue == value)
    {
        return;
    }
    _private->_doubleValue = value;
    Q_EMIT doubleValueChanged(value);
}
