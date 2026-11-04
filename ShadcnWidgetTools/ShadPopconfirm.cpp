#include "ShadPopconfirm.h"

#include <QAbstractButton>
#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QObject>
#include <QShowEvent>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadButton.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadPopconfirmIcon.h"
#include "ShadPopconfirmPrivate.h"
#include "ShadPopoverPrivate.h"
#include "ShadStyleHelper.h"

ShadPopconfirm::ShadPopconfirm(QWidget* parent)
    : ShadPopover(parent), _private(new ShadPopconfirmPrivate())
{
    ShadPopconfirmPrivate* d = _private.data();
    ShadPopoverPrivate* base = ShadPopover::d_ptr.data();
    setContentWidth(ShadPopconfirmPrivate::kDefaultWidth);

    QWidget* headerRow = new QWidget(base->_card);
    QHBoxLayout* headerRowLayout = new QHBoxLayout(headerRow);
    headerRowLayout->setContentsMargins(0, 0, 0, 0);
    headerRowLayout->setSpacing(ShadPopconfirmPrivate::kIconGap);
    d->_iconWidget = new ShadPopconfirmIcon(headerRow);
    d->_iconWidget->setLineHeight(QFontMetrics(base->_titleLabel->font()).height());
    base->_cardLayout->removeWidget(base->_header);
    headerRowLayout->addWidget(d->_iconWidget, 0, Qt::AlignTop);
    headerRowLayout->addWidget(base->_header, 1);
    base->_cardLayout->insertWidget(0, headerRow);

    QHBoxLayout* footerLayout = new QHBoxLayout();
    footerLayout->setContentsMargins(0, 0, 0, 0);
    footerLayout->setSpacing(ShadPopconfirmPrivate::kButtonGap);
    footerLayout->addStretch(1);
    d->_cancelButton = new ShadButton(QString(), ShadButtonType::Outline, base->_card);
    d->_cancelButton->setButtonSize(ShadButtonType::Small);
    d->_confirmButton = new ShadButton(QString(), base->_card);
    d->_confirmButton->setButtonSize(ShadButtonType::Small);
    footerLayout->addWidget(d->_cancelButton);
    footerLayout->addWidget(d->_confirmButton);
    base->_cardLayout->addLayout(footerLayout);
    retranslate();

    connect(d->_cancelButton, &ShadButton::clicked, this, [this]() { resolve(false); });
    connect(d->_confirmButton, &ShadButton::clicked, this, [this]() { resolve(true); });
}

ShadPopconfirm::ShadPopconfirm(const QString& title, const QString& description, QWidget* parent)
    : ShadPopconfirm(parent)
{
    setTitle(title);
    setDescription(description);
}

ShadPopconfirm::~ShadPopconfirm()
{
    QObject::disconnect(_private->_cancelButton, nullptr, this, nullptr);
    QObject::disconnect(_private->_confirmButton, nullptr, this, nullptr);
}

void ShadPopconfirm::setShadIcon(ShadIconType::IconName icon)
{
    _private->_iconWidget->setIcon(icon);
}

ShadIconType::IconName ShadPopconfirm::getShadIcon() const
{
    return _private->_iconWidget->getIcon();
}

void ShadPopconfirm::setDestructive(bool destructive)
{
    if (_private->_isDestructive == destructive)
    {
        return;
    }
    _private->_isDestructive = destructive;
    _private->_confirmButton->setVariant(destructive ? ShadButtonType::Destructive : ShadButtonType::Default);
    _private->_iconWidget->setDestructive(destructive);
}

bool ShadPopconfirm::isDestructive() const
{
    return _private->_isDestructive;
}

ShadButton* ShadPopconfirm::confirmButton() const
{
    return _private->_confirmButton;
}

ShadButton* ShadPopconfirm::cancelButton() const
{
    return _private->_cancelButton;
}

void ShadPopconfirm::attach(QAbstractButton* anchor, ShadPlacementType::Side side, ShadPlacementType::Align align)
{
    if (anchor == nullptr)
    {
        return;
    }
    connect(anchor, &QAbstractButton::clicked, this, [this, anchor, side, align]() { popup(anchor, side, align); });
}

void ShadPopconfirm::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    ShadPopover::changeEvent(event);
}

void ShadPopconfirm::showEvent(QShowEvent* event)
{
    _private->_isResolved = false;
    ShadPopover::showEvent(event);
}

void ShadPopconfirm::hideEvent(QHideEvent* event)
{
    const bool isDismissed = !_private->_isResolved;
    _private->_isResolved = true;
    ShadPopover::hideEvent(event);
    if (isDismissed)
    {
        Q_EMIT cancelled();
    }
}

void ShadPopconfirm::retranslate()
{
    ShadButton* cancelButton = _private->_cancelButton;
    ShadButton* confirmButton = _private->_confirmButton;
    ShadStyleHelper::applyDefaultText(cancelButton, "text", cancelButton->text(), tr("取消"), [cancelButton](const QString& text) { cancelButton->setText(text); });
    ShadStyleHelper::applyDefaultText(confirmButton, "text", confirmButton->text(), tr("确定"), [confirmButton](const QString& text) { confirmButton->setText(text); });
}

void ShadPopconfirm::resolve(bool isConfirmed)
{
    _private->_isResolved = true;
    hide();
    if (isConfirmed)
    {
        Q_EMIT confirmed();
    }
    else
    {
        Q_EMIT cancelled();
    }
}
