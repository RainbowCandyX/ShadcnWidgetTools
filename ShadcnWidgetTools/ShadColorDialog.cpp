#include "ShadColorDialog.h"

#include <QDialog>
#include <QEvent>
#include <QHBoxLayout>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "ShadButton.h"
#include "ShadColorDialogPrivate.h"
#include "ShadColorPicker.h"
#include "ShadDef.h"
#include "ShadDialogPrivate.h"
#include "ShadStyleHelper.h"

ShadColorDialog::ShadColorDialog(QWidget* parent)
    : ShadColorDialog(QColor(Qt::white), parent)
{
}

ShadColorDialog::ShadColorDialog(const QColor& initial, QWidget* parent)
    : ShadDialog(parent), _private(new ShadColorDialogPrivate())
{
    ShadColorDialogPrivate* d = _private.data();
    resize(ShadColorDialogPrivate::kDialogWidth, height());

    d->_picker = new ShadColorPicker(this);
    d->_picker->setColor(initial);
    contentLayout()->addWidget(d->_picker);

    d->_cancelButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    d->_okButton = new ShadButton(QString(), this);
    retranslate();
    // 在十六进制或数值输入框中按 Enter 触发确定
    d->_okButton->setDefault(true);
    footerLayout()->addWidget(d->_cancelButton);
    footerLayout()->addWidget(d->_okButton);
    connect(d->_cancelButton, &ShadButton::clicked, this, &ShadColorDialog::reject);
    connect(d->_okButton, &ShadButton::clicked, this, [this]()
    {
        Q_EMIT colorSelected(getCurrentColor());
        accept();
    });
    connect(d->_picker, &ShadColorPicker::colorChanged, this, &ShadColorDialog::currentColorChanged);
}

ShadColorDialog::~ShadColorDialog()
{
}

void ShadColorDialog::setCurrentColor(const QColor& color)
{
    _private->_picker->setColor(color);
}

QColor ShadColorDialog::getCurrentColor() const
{
    return _private->_picker->getColor();
}

ShadColorPicker* ShadColorDialog::colorPicker() const
{
    return _private->_picker;
}

ShadButton* ShadColorDialog::okButton() const
{
    return _private->_okButton;
}

ShadButton* ShadColorDialog::cancelButton() const
{
    return _private->_cancelButton;
}

QColor ShadColorDialog::getColor(const QColor& initial, QWidget* parent, const QString& title)
{
    ShadColorDialog * dialog = new ShadColorDialog(initial, parent);
    if (!title.isEmpty())
    {
        dialog->setTitle(title);
    }
    // 对话框在 exec 期间被销毁（例如父窗口被删除）时按取消处理
    QPointer < ShadColorDialog > guard(dialog);
    const int result = dialog->exec();
    const bool isAccepted = !guard.isNull() && result == QDialog::Accepted;
    const QColor color = isAccepted ? guard->getCurrentColor() : QColor();
    delete guard.data();
    return color;
}

void ShadColorDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    ShadDialog::changeEvent(event);
}

void ShadColorDialog::retranslate()
{
    ShadButton* cancelButton = _private->_cancelButton;
    ShadButton* okButton = _private->_okButton;
    ShadStyleHelper::applyDefaultText(this, "title", getTitle(), tr("选择颜色"), [this](const QString& text) { setTitle(text); });
    ShadStyleHelper::applyDefaultText(cancelButton, "text", cancelButton->text(), tr("取消"), [cancelButton](const QString& text) { cancelButton->setText(text); });
    ShadStyleHelper::applyDefaultText(okButton, "text", okButton->text(), tr("确定"), [okButton](const QString& text) { okButton->setText(text); });
}

void ShadColorDialog::setVisible(bool visible)
{
    if (!visible || isVisible())
    {
        ShadDialog::setVisible(visible);
        return;
    }
    // 打开时的颜色作为原始颜色，预览条下半显示，点击后恢复
    _private->_picker->setOriginalColor(getCurrentColor());
    // 解除上次固定的尺寸，宽度固定为 520，并让 ShadDialog 重新按内容计算高度；显示后固定
    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    resize(ShadColorDialogPrivate::kDialogWidth, height());
    ShadDialog::d_ptr->_hasAutoSize = false;
    ShadDialog::setVisible(true);
    if (isVisible())
    {
        setFixedSize(size());
    }
}
