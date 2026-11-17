#include "ShadTransferPrivate.h"

#include <QSet>
#include <QString>
#include <QtCore/qnamespace.h>

#include <utility>

#include "ShadButton.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTransferPanel.h"

ShadTransferPrivate::ShadTransferPrivate()
{
}

ShadTransferPrivate::~ShadTransferPrivate()
{
}

void ShadTransferPrivate::refreshPanels()
{
    const QSet<QString> targetKeySet(_targetKeys.cbegin(), _targetKeys.cend());
    QList<ShadTransferItem> sourceItems;
    QList<ShadTransferItem> targetItems;
    for (const ShadTransferItem& item : std::as_const(_items))
    {
        if (targetKeySet.contains(item.key))
        {
            targetItems.append(item);
        }
        else
        {
            sourceItems.append(item);
        }
    }
    _sourcePanel->setItems(sourceItems);
    _targetPanel->setItems(targetItems);
    updateButtons();
}

void ShadTransferPrivate::updateButtons()
{
    _toTargetButton->setEnabled(!_sourcePanel->checkedKeys().isEmpty());
    _toSourceButton->setEnabled(!_targetPanel->checkedKeys().isEmpty());
}

void ShadTransferPrivate::updateButtonIcons()
{
    Q_Q(ShadTransfer);
    const bool isRightToLeft = q->layoutDirection() == Qt::RightToLeft;
    _toTargetButton->setShadIcon(isRightToLeft ? ShadIconType::ChevronLeft : ShadIconType::ChevronRight);
    _toSourceButton->setShadIcon(isRightToLeft ? ShadIconType::ChevronRight : ShadIconType::ChevronLeft);
}

void ShadTransferPrivate::retranslate()
{
    Q_Q(ShadTransfer);
    ShadStyleHelper::applyDefaultText(q, "sourceTitle", _sourcePanel->getTitle(), ShadTransfer::tr("可选项"), [this](const QString& text) { _sourcePanel->setTitle(text); });
    ShadStyleHelper::applyDefaultText(q, "targetTitle", _targetPanel->getTitle(), ShadTransfer::tr("已选项"), [this](const QString& text) { _targetPanel->setTitle(text); });
    const QString placeholder = ShadTransfer::tr("搜索...");
    _sourcePanel->setSearchPlaceholderText(placeholder);
    _targetPanel->setSearchPlaceholderText(placeholder);
    ShadStyleHelper::applyDefaultText(_toTargetButton, "toolTip", _toTargetButton->toolTip(), ShadTransfer::tr("添加所选项"), [this](const QString& text) { _toTargetButton->setToolTip(text); });
    ShadStyleHelper::applyDefaultText(_toSourceButton, "toolTip", _toSourceButton->toolTip(), ShadTransfer::tr("移除所选项"), [this](const QString& text) { _toSourceButton->setToolTip(text); });
}

void ShadTransferPrivate::move(ShadTransferType::Direction direction)
{
    Q_Q(ShadTransfer);
    ShadTransferPanel* fromPanel = direction == ShadTransferType::ToTarget ? _sourcePanel : _targetPanel;
    const QStringList movedKeys = fromPanel->checkedKeys();
    if (movedKeys.isEmpty())
    {
        return;
    }
    fromPanel->uncheckKeys(movedKeys);
    if (direction == ShadTransferType::ToTarget)
    {
        _targetKeys.append(movedKeys);
    }
    else
    {
        for (const QString& key : movedKeys)
        {
            _targetKeys.removeAll(key);
        }
    }
    refreshPanels();
    Q_EMIT q->targetKeysChanged(_targetKeys, direction, movedKeys);
}
