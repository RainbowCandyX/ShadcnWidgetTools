#include "ShadComboBoxPrivate.h"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QStringList>
#include <QVariant>

#include "ShadComboBox.h"
#include "ShadComboBoxSearch.h"
#include "ShadSelectBadges.h"

ShadComboBoxPrivate::ShadComboBoxPrivate()
{
}

ShadComboBoxPrivate::~ShadComboBoxPrivate()
{
}

int ShadComboBoxPrivate::bodyHeight(ShadComboBoxType::Size size)
{
    // Medium 对应 data-[size=default]:h-9；Small 为 24px，比 shadcn 的 h-8 更紧凑
    return size == ShadComboBoxType::Small ? 24 : 36;
}

void ShadComboBoxPrivate::activateRow(int row)
{
    Q_Q(ShadComboBox);
    if (row < 0 || row >= q->count() || !isRowSelectable(row))
    {
        return;
    }
    q->setCurrentIndex(row);
    Q_EMIT q->activated(row);
    Q_EMIT q->textActivated(q->itemText(row));
}

void ShadComboBoxPrivate::handleTypeahead(const QString& text)
{
    Q_Q(ShadComboBox);
    // 多选模式下当前项不参与显示，按键查找没有意义
    if (isMultiSelection())
    {
        return;
    }
    if (!isTypeaheadActive())
    {
        _typeaheadBuffer.clear();
    }
    _typeaheadBuffer += text;
    _typeaheadTimer.restart();
    QStringList texts;
    texts.reserve(q->count());
    for (int row = 0; row < q->count(); ++row)
    {
        texts.append(isRowSelectable(row) ? q->itemText(row) : QString());
    }
    const int row = ShadComboBoxSearch::typeaheadIndex(texts, _typeaheadBuffer, q->currentIndex());
    if (row >= 0 && row != q->currentIndex())
    {
        activateRow(row);
    }
}

bool ShadComboBoxPrivate::isTypeaheadActive() const
{
    return !_typeaheadBuffer.isEmpty() && _typeaheadTimer.isValid() && _typeaheadTimer.elapsed() <= ShadComboBoxSearch::kTypeaheadTimeoutMs;
}

bool ShadComboBoxPrivate::isRowSelectable(int row) const
{
    Q_Q(const ShadComboBox);
    const QAbstractItemModel* model = q->model();
    if (model == nullptr)
    {
        return false;
    }
    const QModelIndex index = model->index(row, q->modelColumn(), q->rootModelIndex());
    if (!index.isValid())
    {
        return false;
    }
    return !isSeparatorRow(row) && model->flags(index).testFlag(Qt::ItemIsEnabled);
}

bool ShadComboBoxPrivate::isSeparatorRow(int row) const
{
    Q_Q(const ShadComboBox);
    // QComboBox::insertSeparator 插入的分隔项
    return q->itemData(row, Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator");
}

bool ShadComboBoxPrivate::isMultiSelection() const
{
    return _selectionMode == ShadComboBoxType::MultiSelection;
}

void ShadComboBoxPrivate::toggleRow(int row)
{
    Q_Q(ShadComboBox);
    if (!isRowSelectable(row))
    {
        return;
    }
    q->setRowChecked(row, !q->isRowChecked(row));
}

void ShadComboBoxPrivate::paintCheckedBadges(QPainter* painter, const QRectF& rect) const
{
    Q_Q(const ShadComboBox);
    ShadSelectBadges::paint(painter, rect, q->getCheckedTexts(), q->font(), q->layoutDirection());
}
