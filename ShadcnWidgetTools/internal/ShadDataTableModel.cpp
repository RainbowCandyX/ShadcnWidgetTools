#include "ShadDataTableModel.h"

#include <QAbstractProxyModel>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include <algorithm>
#include <utility>

ShadDataTableModel::ShadDataTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

ShadDataTableModel::~ShadDataTableModel()
{
}

void ShadDataTableModel::setProxyModel(QAbstractProxyModel* proxy)
{
    beginResetModel();
    for (const QMetaObject::Connection& connection : std::as_const(_proxyConnections))
    {
        QObject::disconnect(connection);
    }
    _proxyConnections.clear();
    _proxy = proxy;
    _selectedRows.clear();
    _pageIndex = 0;
    if (proxy != nullptr)
    {
        // 代理的任何变化都整体重置：只显示一页数据，代价很小
        const auto reset = [this]() { resetFromProxy(); };
        _proxyConnections.append(connect(proxy, &QAbstractItemModel::modelReset, this, reset));
        _proxyConnections.append(connect(proxy, &QAbstractItemModel::layoutChanged, this, reset));
        _proxyConnections.append(connect(proxy, &QAbstractItemModel::rowsInserted, this, reset));
        _proxyConnections.append(connect(proxy, &QAbstractItemModel::rowsRemoved, this, reset));
        _proxyConnections.append(connect(proxy, &QAbstractItemModel::rowsMoved, this, reset));
        _proxyConnections.append(connect(proxy, &QAbstractItemModel::columnsInserted, this, reset));
        _proxyConnections.append(connect(proxy, &QAbstractItemModel::columnsRemoved, this, reset));
        _proxyConnections.append(connect(proxy, &QAbstractItemModel::dataChanged, this, reset));
        _proxyConnections.append(connect(proxy, &QAbstractItemModel::headerDataChanged, this, reset));
    }
    endResetModel();
    Q_EMIT pageStateChanged();
    Q_EMIT selectionChanged();
}

void ShadDataTableModel::setSelectionEnabled(bool enabled)
{
    if (_isSelectionEnabled == enabled)
    {
        return;
    }
    beginResetModel();
    _isSelectionEnabled = enabled;
    endResetModel();
}

void ShadDataTableModel::setActionEnabled(bool enabled)
{
    if (_isActionEnabled == enabled)
    {
        return;
    }
    beginResetModel();
    _isActionEnabled = enabled;
    endResetModel();
}

void ShadDataTableModel::setVisibleColumns(const QList<int>& columns)
{
    beginResetModel();
    _visibleColumns = columns;
    endResetModel();
}

void ShadDataTableModel::setPage(int pageIndex, int pageSize)
{
    beginResetModel();
    _pageSize = qMax(1, pageSize);
    _pageIndex = qBound(0, pageIndex, getPageCount() - 1);
    endResetModel();
    Q_EMIT pageStateChanged();
}

int ShadDataTableModel::getPageIndex() const
{
    return _pageIndex;
}

int ShadDataTableModel::getPageCount() const
{
    const int filteredRowCount = getFilteredRowCount();
    return qMax(1, (filteredRowCount + _pageSize - 1) / _pageSize);
}

int ShadDataTableModel::getFilteredRowCount() const
{
    return _proxy.isNull() ? 0 : _proxy->rowCount();
}

int ShadDataTableModel::getFilteredSelectedCount() const
{
    if (_proxy.isNull() || _selectedRows.isEmpty())
    {
        return 0;
    }
    int count = 0;
    const int filteredRowCount = getFilteredRowCount();
    for (int row = 0; row < filteredRowCount; ++row)
    {
        if (_selectedRows.contains(QPersistentModelIndex(_proxy->mapToSource(_proxy->index(row, 0)))))
        {
            ++count;
        }
    }
    return count;
}

QList<int> ShadDataTableModel::getSelectedSourceRows() const
{
    QList<int> rows;
    rows.reserve(_selectedRows.size());
    for (const QPersistentModelIndex& index : _selectedRows)
    {
        // 源模型删除行后对应的持久索引会失效
        if (index.isValid())
        {
            rows.append(index.row());
        }
    }
    std::sort(rows.begin(), rows.end());
    return rows;
}

void ShadDataTableModel::clearSelection()
{
    if (_selectedRows.isEmpty())
    {
        return;
    }
    _selectedRows.clear();
    if (rowCount() > 0)
    {
        Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    }
    Q_EMIT selectionChanged();
}

int ShadDataTableModel::selectionColumn() const
{
    return _isSelectionEnabled ? 0 : -1;
}

int ShadDataTableModel::actionColumn() const
{
    return _isActionEnabled ? columnCount() - 1 : -1;
}

int ShadDataTableModel::proxyColumn(int tableColumn) const
{
    const int offset = _isSelectionEnabled ? 1 : 0;
    const int visibleIndex = tableColumn - offset;
    if (visibleIndex < 0 || visibleIndex >= int(_visibleColumns.size()))
    {
        return -1;
    }
    return _visibleColumns.at(visibleIndex);
}

int ShadDataTableModel::tableColumn(int proxyColumn) const
{
    const int visibleIndex = int(_visibleColumns.indexOf(proxyColumn));
    if (visibleIndex < 0)
    {
        return -1;
    }
    return visibleIndex + (_isSelectionEnabled ? 1 : 0);
}

int ShadDataTableModel::sourceRow(int tableRow) const
{
    const QModelIndex key = selectionKey(tableRow);
    return key.isValid() ? key.row() : -1;
}

int ShadDataTableModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid() || _proxy.isNull())
    {
        return 0;
    }
    const int pageStart = _pageIndex * _pageSize;
    return qBound(0, getFilteredRowCount() - pageStart, _pageSize);
}

int ShadDataTableModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return int(_visibleColumns.size()) + (_isSelectionEnabled ? 1 : 0) + (_isActionEnabled ? 1 : 0);
}

QVariant ShadDataTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || _proxy.isNull())
    {
        return QVariant();
    }
    if (index.column() == selectionColumn())
    {
        if (role == Qt::CheckStateRole)
        {
            const bool isSelected = _selectedRows.contains(QPersistentModelIndex(selectionKey(index.row())));
            return static_cast<int>(isSelected ? Qt::Checked : Qt::Unchecked);
        }
        return QVariant();
    }
    if (index.column() == actionColumn())
    {
        return QVariant();
    }
    const int column = proxyColumn(index.column());
    return column >= 0 ? proxyIndex(index.row(), column).data(role) : QVariant();
}

bool ShadDataTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || index.column() != selectionColumn() || role != Qt::CheckStateRole)
    {
        return false;
    }
    const QPersistentModelIndex key(selectionKey(index.row()));
    if (!key.isValid())
    {
        return false;
    }
    const bool isChecked = value.toInt() == Qt::Checked;
    if (_selectedRows.contains(key) == isChecked)
    {
        return true;
    }
    if (isChecked)
    {
        _selectedRows.insert(key);
    }
    else
    {
        _selectedRows.remove(key);
    }
    // 整行背景随选择状态变化（data-state="selected"）
    Q_EMIT dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount() - 1));
    Q_EMIT selectionChanged();
    return true;
}

Qt::ItemFlags ShadDataTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags itemFlags = Qt::ItemIsEnabled;
    if (index.column() == selectionColumn())
    {
        itemFlags |= Qt::ItemIsUserCheckable;
    }
    return itemFlags;
}

QVariant ShadDataTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || _proxy.isNull())
    {
        return QVariant();
    }
    if (section == selectionColumn() || section == actionColumn())
    {
        return role == Qt::DisplayRole ? QVariant(QString()) : QVariant();
    }
    const int column = proxyColumn(section);
    return column >= 0 ? _proxy->headerData(column, Qt::Horizontal, role) : QVariant();
}

QModelIndex ShadDataTableModel::proxyIndex(int tableRow, int proxyColumn) const
{
    if (_proxy.isNull())
    {
        return QModelIndex();
    }
    return _proxy->index(_pageIndex * _pageSize + tableRow, proxyColumn);
}

QModelIndex ShadDataTableModel::selectionKey(int tableRow) const
{
    if (_proxy.isNull())
    {
        return QModelIndex();
    }
    return _proxy->mapToSource(proxyIndex(tableRow, 0));
}

void ShadDataTableModel::resetFromProxy()
{
    beginResetModel();
    // 筛选后行数变少时把页码夹到有效范围
    _pageIndex = qBound(0, _pageIndex, getPageCount() - 1);
    endResetModel();
    Q_EMIT pageStateChanged();
}
