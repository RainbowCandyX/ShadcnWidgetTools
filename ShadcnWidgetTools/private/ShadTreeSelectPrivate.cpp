#include "ShadTreeSelectPrivate.h"

#include <QAbstractItemModel>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetricsF>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMetaType>
#include <QObject>
#include <QPixmap>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringList>
#include <QVariant>
#include <QtMath>

#include <utility>

#include "ShadTreeSelect.h"
#include "ShadTreeSelectPanel.h"
#include "ShadTreeView.h"

namespace
{
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 8;
} // namespace

ShadTreeSelectPrivate::ShadTreeSelectPrivate()
{
}

ShadTreeSelectPrivate::~ShadTreeSelectPrivate()
{
}

bool ShadTreeSelectPrivate::isMultiSelection() const
{
    return _selectionMode == ShadComboBoxType::MultiSelection;
}

QAbstractItemModel* ShadTreeSelectPrivate::currentModel() const
{
    return _panel->treeView()->model();
}

bool ShadTreeSelectPrivate::hasCheckbox(const QModelIndex& index)
{
    return index.isValid() && index.flags().testFlag(Qt::ItemIsUserCheckable) && index.data(Qt::CheckStateRole).isValid();
}

Qt::CheckState ShadTreeSelectPrivate::checkState(const QModelIndex& index)
{
    return static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
}

QIcon ShadTreeSelectPrivate::decorationIcon(const QModelIndex& index)
{
    const QVariant decoration = index.data(Qt::DecorationRole);
    if (decoration.userType() == QMetaType::QPixmap)
    {
        return QIcon(decoration.value<QPixmap>());
    }
    return decoration.value<QIcon>();
}

QString ShadTreeSelectPrivate::displayText(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return QString();
    }
    if (!_isFullPathVisible)
    {
        return index.data(Qt::DisplayRole).toString();
    }
    QStringList parts;
    for (QModelIndex current = index; current.isValid(); current = current.parent())
    {
        parts.prepend(current.data(Qt::DisplayRole).toString());
    }
    return parts.join(QStringLiteral(" / "));
}

void ShadTreeSelectPrivate::connectModel(QAbstractItemModel* model)
{
    Q_Q(ShadTreeSelect);
    if (model == nullptr)
    {
        return;
    }
    const auto structureChanged = [this]() { onModelStructureChanged(); };
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::rowsInserted, q, structureChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::rowsRemoved, q, structureChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::rowsMoved, q, structureChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::modelReset, q, structureChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::layoutChanged, q, structureChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::dataChanged, q, [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles)
    {
        Q_UNUSED(topLeft);
        Q_UNUSED(bottomRight);
        onModelDataChanged(roles);
    }));
    // 用户模型被销毁时改回内部模型
    if (model != _internalModel)
    {
        _modelConnections.append(QObject::connect(model, &QObject::destroyed, q, [q]() { q->setModel(nullptr); }));
    }
}

void ShadTreeSelectPrivate::disconnectModel()
{
    for (const QMetaObject::Connection& connection : std::as_const(_modelConnections))
    {
        QObject::disconnect(connection);
    }
    _modelConnections.clear();
}

void ShadTreeSelectPrivate::onModelStructureChanged()
{
    Q_Q(ShadTreeSelect);
    // 关闭弹出层，避免过滤与高亮指向失效的行
    q->hidePopup();
    invalidateTextWidth();
    checkCurrentIndexValid();
    if (isMultiSelection())
    {
        notifyCheckedChanged();
    }
    q->updateGeometry();
    q->update();
}

void ShadTreeSelectPrivate::onModelDataChanged(const QList<int>& roles)
{
    Q_Q(ShadTreeSelect);
    if (roles.isEmpty() || roles.contains(Qt::DisplayRole) || roles.contains(Qt::DecorationRole) || roles.contains(ShadTreeView::IconRole))
    {
        invalidateTextWidth();
        q->updateGeometry();
    }
    if (isMultiSelection() && (roles.isEmpty() || roles.contains(Qt::CheckStateRole)))
    {
        notifyCheckedChanged();
    }
    q->update();
}

void ShadTreeSelectPrivate::checkCurrentIndexValid()
{
    Q_Q(ShadTreeSelect);
    if (!_hasCurrentIndex || _currentIndex.isValid())
    {
        return;
    }
    _hasCurrentIndex = false;
    _currentIndex = QPersistentModelIndex();
    q->update();
    Q_EMIT q->currentIndexChanged(QModelIndex());
}

void ShadTreeSelectPrivate::configureInternalItem(QStandardItem* item) const
{
    if (isMultiSelection())
    {
        // setCheckable 在没有勾选状态时会设为未勾选
        item->setCheckable(true);
        item->setAutoTristate(true);
    }
    else
    {
        item->setAutoTristate(false);
        item->setCheckable(false);
    }
}

void ShadTreeSelectPrivate::configureInternalItems(QStandardItem* parent) const
{
    for (int row = 0; row < parent->rowCount(); ++row)
    {
        QStandardItem* child = parent->child(row);
        if (child == nullptr)
        {
            continue;
        }
        configureInternalItem(child);
        configureInternalItems(child);
    }
}

void ShadTreeSelectPrivate::applyCheckState(const QModelIndex& index, Qt::CheckState state)
{
    QAbstractItemModel* model = currentModel();
    if (!model->setData(index, static_cast<int>(state), Qt::CheckStateRole))
    {
        return;
    }
    // 联动规则与 ShadTreeView 一致
    if (index.flags().testFlag(Qt::ItemIsAutoTristate) && state != Qt::PartiallyChecked)
    {
        setDescendantsCheckState(index, state);
    }
    updateAncestorsCheckState(index);
}

void ShadTreeSelectPrivate::setDescendantsCheckState(const QModelIndex& index, Qt::CheckState state)
{
    QAbstractItemModel* model = currentModel();
    for (int row = 0; row < model->rowCount(index); ++row)
    {
        const QModelIndex child = model->index(row, 0, index);
        if (hasCheckbox(child) && child.flags().testFlag(Qt::ItemIsEnabled))
        {
            model->setData(child, static_cast<int>(state), Qt::CheckStateRole);
        }
        setDescendantsCheckState(child, state);
    }
}

void ShadTreeSelectPrivate::updateAncestorsCheckState(const QModelIndex& index)
{
    QAbstractItemModel* model = currentModel();
    for (QModelIndex parent = index.parent(); parent.isValid(); parent = parent.parent())
    {
        if (!hasCheckbox(parent) || !parent.flags().testFlag(Qt::ItemIsAutoTristate))
        {
            break;
        }
        // 按子节点状态计算祖先的三态
        int checkableCount = 0;
        int checkedCount = 0;
        int uncheckedCount = 0;
        for (int row = 0; row < model->rowCount(parent); ++row)
        {
            const QModelIndex child = model->index(row, 0, parent);
            if (!hasCheckbox(child))
            {
                continue;
            }
            ++checkableCount;
            const Qt::CheckState childState = checkState(child);
            if (childState == Qt::Checked)
            {
                ++checkedCount;
            }
            else if (childState == Qt::Unchecked)
            {
                ++uncheckedCount;
            }
        }
        if (checkableCount == 0)
        {
            break;
        }
        Qt::CheckState parentState = Qt::PartiallyChecked;
        if (checkedCount == checkableCount)
        {
            parentState = Qt::Checked;
        }
        else if (uncheckedCount == checkableCount)
        {
            parentState = Qt::Unchecked;
        }
        model->setData(parent, static_cast<int>(parentState), Qt::CheckStateRole);
    }
}

void ShadTreeSelectPrivate::collectCheckedIndexes(const QModelIndex& parent, QList<QModelIndex>* indexes) const
{
    const QAbstractItemModel* model = currentModel();
    for (int row = 0; row < model->rowCount(parent); ++row)
    {
        const QModelIndex index = model->index(row, 0, parent);
        const bool isChecked = hasCheckbox(index) && checkState(index) == Qt::Checked;
        switch (_checkedStrategy)
        {
            case ShadTreeSelectType::ShowParent:
                {
                    // 节点已勾选时不再列出它的子孙
                    if (isChecked)
                    {
                        indexes->append(index);
                        continue;
                    }
                    break;
                }
            case ShadTreeSelectType::ShowChild:
                {
                    if (isChecked && !model->hasChildren(index))
                    {
                        indexes->append(index);
                    }
                    break;
                }
            case ShadTreeSelectType::ShowAll:
                {
                    if (isChecked)
                    {
                        indexes->append(index);
                    }
                    break;
                }
        }
        collectCheckedIndexes(index, indexes);
    }
}

void ShadTreeSelectPrivate::uncheckRows(const QModelIndex& parent)
{
    QAbstractItemModel* model = currentModel();
    for (int row = 0; row < model->rowCount(parent); ++row)
    {
        const QModelIndex index = model->index(row, 0, parent);
        if (hasCheckbox(index) && checkState(index) != Qt::Unchecked)
        {
            model->setData(index, static_cast<int>(Qt::Unchecked), Qt::CheckStateRole);
        }
        uncheckRows(index);
    }
}

void ShadTreeSelectPrivate::notifyCheckedChanged()
{
    Q_Q(ShadTreeSelect);
    if (_checkBatchDepth > 0)
    {
        _isCheckedDirty = true;
        return;
    }
    // 联动会产生多次 dataChanged，排队合并为一次
    if (_isCheckedSignalQueued)
    {
        return;
    }
    _isCheckedSignalQueued = true;
    QMetaObject::invokeMethod(q, [this]()
    {
        _isCheckedSignalQueued = false;
        emitCheckedChanged();
    }, Qt::QueuedConnection);
}

void ShadTreeSelectPrivate::emitCheckedChanged()
{
    Q_Q(ShadTreeSelect);
    q->update();
    const QList<QModelIndex> indexes = q->getCheckedIndexes();
    QList<QPersistentModelIndex> checkedIndexes;
    checkedIndexes.reserve(indexes.size());
    for (const QModelIndex& index : indexes)
    {
        checkedIndexes.append(QPersistentModelIndex(index));
    }
    if (checkedIndexes == _lastCheckedIndexes)
    {
        return;
    }
    _lastCheckedIndexes = checkedIndexes;
    Q_EMIT q->checkedIndexesChanged();
}

int ShadTreeSelectPrivate::contentWidth() const
{
    Q_Q(const ShadTreeSelect);
    if (_cachedContentWidth >= 0)
    {
        return _cachedContentWidth;
    }
    const QFontMetricsF metrics(q->font());
    qreal textWidth = metrics.horizontalAdvance(_placeholderText);
    bool hasIcon = false;
    measureRows(QModelIndex(), metrics, &textWidth, &hasIcon);
    _cachedContentWidth = qCeil(textWidth) + (hasIcon ? kIconSize + kIconGap : 0);
    return _cachedContentWidth;
}

void ShadTreeSelectPrivate::measureRows(const QModelIndex& parent, const QFontMetricsF& metrics, qreal* textWidth, bool* hasIcon) const
{
    const QAbstractItemModel* model = currentModel();
    for (int row = 0; row < model->rowCount(parent); ++row)
    {
        const QModelIndex index = model->index(row, 0, parent);
        *textWidth = qMax(*textWidth, metrics.horizontalAdvance(displayText(index)));
        *hasIcon = *hasIcon || index.data(ShadTreeView::IconRole).isValid() || !decorationIcon(index).isNull();
        measureRows(index, metrics, textWidth, hasIcon);
    }
}

void ShadTreeSelectPrivate::invalidateTextWidth()
{
    _cachedContentWidth = -1;
}

void ShadTreeSelectPrivate::onTreePressed(const QModelIndex& index)
{
    const QModelIndex node = index.siblingAtColumn(0);
    _pressedIndex = QPersistentModelIndex(node);
    _pressedCheckState = hasCheckbox(node) ? static_cast<int>(checkState(node)) : -1;
}

void ShadTreeSelectPrivate::onTreeClicked(const QModelIndex& index)
{
    const QModelIndex node = index.siblingAtColumn(0);
    if (isMultiSelection())
    {
        // 按下与松开时勾选状态相同说明点在复选框以外，由这里切换勾选
        const bool isUnchanged = _pressedIndex == node && hasCheckbox(node) && static_cast<int>(checkState(node)) == _pressedCheckState;
        _pressedIndex = QPersistentModelIndex();
        _pressedCheckState = -1;
        if (!isUnchanged)
        {
            return;
        }
    }
    activateIndex(node);
}

void ShadTreeSelectPrivate::activateIndex(const QModelIndex& index)
{
    Q_Q(ShadTreeSelect);
    if (!index.isValid() || !index.flags().testFlag(Qt::ItemIsEnabled))
    {
        return;
    }
    if (isMultiSelection())
    {
        if (hasCheckbox(index))
        {
            q->setIndexChecked(index, checkState(index) != Qt::Checked);
        }
        return;
    }
    if (_isLeafOnly && currentModel()->hasChildren(index))
    {
        ShadTreeView* treeView = _panel->treeView();
        treeView->setExpanded(index, !treeView->isExpanded(index));
        return;
    }
    q->hidePopup();
    q->setCurrentIndex(index);
}

void ShadTreeSelectPrivate::onFiltered(const QModelIndex& firstMatch)
{
    // 过滤后高亮第一个匹配项
    highlightIndex(firstMatch.isValid() ? firstMatch : initialHighlightIndex());
}

QModelIndex ShadTreeSelectPrivate::initialHighlightIndex() const
{
    if (!isMultiSelection() && _currentIndex.isValid() && _panel->isIndexUnhidden(_currentIndex))
    {
        return _currentIndex;
    }
    return _panel->firstVisibleIndex();
}

void ShadTreeSelectPrivate::highlightIndex(const QModelIndex& index)
{
    ShadTreeView* treeView = _panel->treeView();
    QItemSelectionModel* selectionModel = treeView->selectionModel();
    if (selectionModel == nullptr)
    {
        return;
    }
    if (!index.isValid())
    {
        selectionModel->clearSelection();
        return;
    }
    selectionModel->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    treeView->scrollTo(index);
}

bool ShadTreeSelectPrivate::handleTreeKey(QKeyEvent* event)
{
    Q_Q(ShadTreeSelect);
    ShadTreeView* treeView = _panel->treeView();
    switch (event->key())
    {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            {
                activateIndex(treeView->currentIndex().siblingAtColumn(0));
                event->accept();
                return true;
            }
        case Qt::Key_Space:
            {
                // 多选时空格由树切换复选框；单选时空格与回车相同
                if (isMultiSelection())
                {
                    return false;
                }
                activateIndex(treeView->currentIndex().siblingAtColumn(0));
                event->accept();
                return true;
            }
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            {
                q->hidePopup();
                event->accept();
                return true;
            }
        default:
            {
                break;
            }
    }
    // 有搜索框时直接输入的文字转到搜索框
    const QString text = event->text();
    const bool hasCommandModifier = event->modifiers().testAnyFlags(Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier);
    if (_isSearchEnabled && !hasCommandModifier && !text.isEmpty() && text.at(0).isPrint())
    {
        QLineEdit* searchEdit = _panel->searchEdit();
        searchEdit->setFocus(Qt::OtherFocusReason);
        QCoreApplication::sendEvent(searchEdit, event);
        return true;
    }
    return false;
}

bool ShadTreeSelectPrivate::handleSearchKey(QKeyEvent* event)
{
    Q_Q(ShadTreeSelect);
    switch (event->key())
    {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            {
                // 焦点留在搜索框，方向键交给树移动高亮项
                QKeyEvent forwarded(QEvent::KeyPress, event->key(), event->modifiers(), event->text());
                QCoreApplication::sendEvent(_panel->treeView(), &forwarded);
                event->accept();
                return true;
            }
        case Qt::Key_Return:
        case Qt::Key_Enter:
            {
                activateIndex(_panel->treeView()->currentIndex().siblingAtColumn(0));
                event->accept();
                return true;
            }
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            {
                q->hidePopup();
                event->accept();
                return true;
            }
        default:
            {
                return false;
            }
    }
}
