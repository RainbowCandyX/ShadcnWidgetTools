#include "ShadAlertDialog.h"

#include <QDialog>
#include <QEvent>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLayout>
#include <QPixmap>
#include <QPointer>
#include <QSizePolicy>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <initializer_list>

#include "ShadAlertDialogMedia.h"
#include "ShadAlertDialogPrivate.h"
#include "ShadButton.h"
#include "ShadDialogPrivate.h"
#include "ShadLabel.h"
#include "ShadStyleHelper.h"

ShadAlertDialog::ShadAlertDialog(QWidget* parent)
    : ShadDialog(parent), _private(new ShadAlertDialogPrivate())
{
    ShadAlertDialogPrivate* d = _private.data();
    ShadDialogPrivate* base = ShadDialog::d_ptr.data();
    ShadStyleHelper::applyFont(base->_titleLabel, ShadAlertDialogPrivate::kTitlePixelSize, QFont::Medium);

    d->_media = new ShadAlertDialogMedia(base->_header);
    d->_media->hide();

    // 取消按钮先创建，位于焦点链前面，打开时获得焦点
    d->_cancelButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    d->_actionButton = new ShadButton(QString(), this);
    retranslate();
    footerLayout()->addWidget(d->_cancelButton);
    footerLayout()->addWidget(d->_actionButton);
    connect(d->_cancelButton, &ShadButton::clicked, this, &ShadAlertDialog::reject);
    connect(d->_actionButton, &ShadButton::clicked, this, &ShadAlertDialog::accept);

    rebuildHeader();
    updateFooter();
}

ShadAlertDialog::ShadAlertDialog(const QString& title, const QString& description, QWidget* parent)
    : ShadAlertDialog(parent)
{
    setTitle(title);
    setDescription(description);
}

ShadAlertDialog::~ShadAlertDialog()
{
}

void ShadAlertDialog::setSize(ShadAlertDialogType::Size size)
{
    if (_private->_size == size)
    {
        return;
    }
    _private->_size = size;
    rebuildHeader();
    updateFooter();
}

ShadAlertDialogType::Size ShadAlertDialog::getSize() const
{
    return _private->_size;
}

void ShadAlertDialog::setMediaIcon(ShadIconType::IconName icon)
{
    _private->_media->setIcon(icon);
    rebuildHeader();
}

ShadIconType::IconName ShadAlertDialog::getMediaIcon() const
{
    return _private->_media->getIcon();
}

void ShadAlertDialog::setMediaPixmap(const QPixmap& pixmap)
{
    _private->_media->setPixmap(pixmap);
    rebuildHeader();
}

QPixmap ShadAlertDialog::getMediaPixmap() const
{
    return _private->_media->getPixmap();
}

void ShadAlertDialog::setMediaVariant(ShadAlertDialogType::MediaVariant variant)
{
    _private->_media->setVariant(variant);
}

ShadAlertDialogType::MediaVariant ShadAlertDialog::getMediaVariant() const
{
    return _private->_media->getVariant();
}

ShadButton* ShadAlertDialog::actionButton() const
{
    return _private->_actionButton;
}

ShadButton* ShadAlertDialog::cancelButton() const
{
    return _private->_cancelButton;
}

bool ShadAlertDialog::confirm(QWidget* parent, const QString& title, const QString& description, const QString& actionText, const QString& cancelText, ShadButtonType::Variant actionVariant)
{
    ShadAlertDialog* dialog = new ShadAlertDialog(title, description, parent);
    if (!actionText.isEmpty())
    {
        dialog->actionButton()->setText(actionText);
    }
    if (!cancelText.isEmpty())
    {
        dialog->cancelButton()->setText(cancelText);
    }
    dialog->actionButton()->setVariant(actionVariant);
    // 对话框在 exec 期间被销毁（例如父窗口被删除）时按取消处理
    QPointer<ShadAlertDialog> guard(dialog);
    const int result = dialog->exec();
    const bool isAccepted = !guard.isNull() && result == QDialog::Accepted;
    delete guard.data();
    return isAccepted;
}

void ShadAlertDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    ShadDialog::changeEvent(event);
}

void ShadAlertDialog::retranslate()
{
    ShadButton* cancelButton = _private->_cancelButton;
    ShadButton* actionButton = _private->_actionButton;
    ShadStyleHelper::applyDefaultText(cancelButton, "text", cancelButton->text(), tr("取消"), [cancelButton](const QString& text) { cancelButton->setText(text); });
    ShadStyleHelper::applyDefaultText(actionButton, "text", actionButton->text(), tr("继续"), [actionButton](const QString& text) { actionButton->setText(text); });
}

void ShadAlertDialog::setVisible(bool visible)
{
    if (!visible || isVisible())
    {
        ShadDialog::setVisible(visible);
        return;
    }
    // 解除上次固定的尺寸，宽度按尺寸设置，并让 ShadDialog 重新按内容计算高度；显示后固定
    const bool isSmall = _private->_size == ShadAlertDialogType::Small;
    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    resize(isSmall ? ShadAlertDialogPrivate::kSmallWidth : ShadAlertDialogPrivate::kMediumWidth, height());
    ShadDialog::d_ptr->_hasAutoSize = false;
    ShadDialog::setVisible(true);
    if (isVisible())
    {
        setFixedSize(size());
    }
}

void ShadAlertDialog::rebuildHeader()
{
    ShadAlertDialogPrivate* d = _private.data();
    ShadDialogPrivate* base = ShadDialog::d_ptr.data();
    QWidget* header = base->_header;
    // 删除布局不会删除其中的组件
    delete header->layout();

    const bool isSmall = d->_size == ShadAlertDialogType::Small;
    const bool hasMedia = d->_media->hasContent();
    // Small 居中（text-center）；Medium 靠起始侧，QLabel 的 AlignLeft 在 RTL 下自动对调
    const Qt::Alignment textAlignment = (isSmall ? Qt::AlignHCenter : Qt::AlignLeft) | Qt::AlignTop;
    base->_titleLabel->setAlignment(textAlignment);
    base->_descriptionLabel->setAlignment(textAlignment);
    d->_media->setVisible(hasMedia);

    if (hasMedia && !isSmall)
    {
        // Medium 有媒体区：媒体区占两行位于起始侧，标题与描述为一列；多出的高度给描述所在行（grid-rows-[auto_1fr]）
        QGridLayout* layout = new QGridLayout(header);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setHorizontalSpacing(ShadAlertDialogPrivate::kMediaColumnGap);
        layout->setVerticalSpacing(ShadAlertDialogPrivate::kHeaderGap);
        layout->addWidget(d->_media, 0, 0, 2, 1, Qt::AlignTop);
        layout->addWidget(base->_titleLabel, 0, 1);
        layout->addWidget(base->_descriptionLabel, 1, 1);
        layout->setColumnStretch(1, 1);
        layout->setRowStretch(1, 1);
        layout->setAlignment(Qt::AlignTop);
        return;
    }
    // 其余情况上下排列：Small 时媒体区居中位于标题上方
    QVBoxLayout* layout = new QVBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(ShadAlertDialogPrivate::kHeaderGap);
    if (hasMedia)
    {
        layout->addWidget(d->_media, 0, Qt::AlignHCenter);
    }
    layout->addWidget(base->_titleLabel);
    layout->addWidget(base->_descriptionLabel);
    layout->setAlignment(Qt::AlignTop);
}

void ShadAlertDialog::updateFooter()
{
    ShadAlertDialogPrivate* d = _private.data();
    const bool isSmall = d->_size == ShadAlertDialogType::Small;
    QHBoxLayout* layout = footerLayout();
    // Small：两个按钮等宽占满一行（grid-cols-2）；Medium：按钮按内容宽度靠结束侧排列（justify-end）
    layout->setAlignment(isSmall ? Qt::Alignment(Qt::AlignBottom) : Qt::Alignment(Qt::AlignRight | Qt::AlignBottom));
    for (ShadButton* button : {d->_cancelButton, d->_actionButton})
    {
        button->setSizePolicy(isSmall ? QSizePolicy::Expanding : QSizePolicy::Minimum, QSizePolicy::Fixed);
        layout->setStretchFactor(button, isSmall ? 1 : 0);
    }
}
