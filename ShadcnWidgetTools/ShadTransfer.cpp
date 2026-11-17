#include "ShadTransfer.h"

#include <QAbstractButton>
#include <QEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "ShadButton.h"
#include "ShadStyleHelper.h"
#include "ShadTransferPanel.h"
#include "ShadTransferPrivate.h"

namespace
{
    constexpr int kPanelGap = 8;
    constexpr int kButtonGap = 8;
} // namespace

ShadTransfer::ShadTransfer(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadTransferPrivate())
{
    Q_D(ShadTransfer);
    d->q_ptr = this;
    const int focusMargin = ShadStyleHelper::kFocusRingWidth;
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(kPanelGap - focusMargin);

    d->_sourcePanel = new ShadTransferPanel(this);
    d->_targetPanel = new ShadTransferPanel(this);
    QWidget* buttonColumn = new QWidget(this);
    QVBoxLayout* buttonLayout = new QVBoxLayout(buttonColumn);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(kButtonGap - 2 * focusMargin);
    d->_toTargetButton = new ShadButton(QString(), ShadButtonType::Outline, buttonColumn);
    d->_toTargetButton->setButtonSize(ShadButtonType::IconSmall);
    d->_toSourceButton = new ShadButton(QString(), ShadButtonType::Outline, buttonColumn);
    d->_toSourceButton->setButtonSize(ShadButtonType::IconSmall);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(d->_toTargetButton);
    buttonLayout->addWidget(d->_toSourceButton);
    buttonLayout->addStretch(1);

    layout->addWidget(d->_sourcePanel, 1);
    layout->addWidget(buttonColumn);
    layout->addWidget(d->_targetPanel, 1);

    connect(d->_toTargetButton, &QAbstractButton::clicked, this, &ShadTransfer::moveToTarget);
    connect(d->_toSourceButton, &QAbstractButton::clicked, this, &ShadTransfer::moveToSource);
    connect(d->_sourcePanel, &ShadTransferPanel::checkedChanged, this, [d]() { d->updateButtons(); });
    connect(d->_targetPanel, &ShadTransferPanel::checkedChanged, this, [d]() { d->updateButtons(); });
    d->updateButtonIcons();
    d->retranslate();
    d->refreshPanels();
}

ShadTransfer::~ShadTransfer()
{
    Q_D(ShadTransfer);
    QObject::disconnect(d->_sourcePanel, nullptr, this, nullptr);
    QObject::disconnect(d->_targetPanel, nullptr, this, nullptr);
}

void ShadTransfer::setItems(const QList<ShadTransferItem>& items)
{
    Q_D(ShadTransfer);
    d->_items = items;
    d->refreshPanels();
}

QList<ShadTransferItem> ShadTransfer::getItems() const
{
    Q_D(const ShadTransfer);
    return d->_items;
}

void ShadTransfer::setTargetKeys(const QStringList& keys)
{
    Q_D(ShadTransfer);
    QStringList uniqueKeys = keys;
    uniqueKeys.removeDuplicates();
    d->_targetKeys = uniqueKeys;
    d->refreshPanels();
}

QStringList ShadTransfer::getTargetKeys() const
{
    Q_D(const ShadTransfer);
    return d->_targetKeys;
}

void ShadTransfer::setTitles(const QString& sourceTitle, const QString& targetTitle)
{
    Q_D(ShadTransfer);
    d->_sourcePanel->setTitle(sourceTitle);
    d->_targetPanel->setTitle(targetTitle);
}

QString ShadTransfer::getSourceTitle() const
{
    Q_D(const ShadTransfer);
    return d->_sourcePanel->getTitle();
}

QString ShadTransfer::getTargetTitle() const
{
    Q_D(const ShadTransfer);
    return d->_targetPanel->getTitle();
}

void ShadTransfer::setSearchEnabled(bool enabled)
{
    Q_D(ShadTransfer);
    if (d->_isSearchEnabled == enabled)
    {
        return;
    }
    d->_isSearchEnabled = enabled;
    d->_sourcePanel->setSearchEnabled(enabled);
    d->_targetPanel->setSearchEnabled(enabled);
}

bool ShadTransfer::isSearchEnabled() const
{
    Q_D(const ShadTransfer);
    return d->_isSearchEnabled;
}

QStringList ShadTransfer::getSourceCheckedKeys() const
{
    Q_D(const ShadTransfer);
    return d->_sourcePanel->checkedKeys();
}

QStringList ShadTransfer::getTargetCheckedKeys() const
{
    Q_D(const ShadTransfer);
    return d->_targetPanel->checkedKeys();
}

void ShadTransfer::moveToTarget()
{
    Q_D(ShadTransfer);
    d->move(ShadTransferType::ToTarget);
}

void ShadTransfer::moveToSource()
{
    Q_D(ShadTransfer);
    d->move(ShadTransferType::ToSource);
}

void ShadTransfer::changeEvent(QEvent* event)
{
    Q_D(ShadTransfer);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->updateButtonIcons();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        d->retranslate();
    }
    QWidget::changeEvent(event);
}
