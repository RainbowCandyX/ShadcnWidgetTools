#include "ShadDataTablePrivate.h"

#include <QAbstractItemModel>
#include <QAction>
#include <QList>
#include <QObject>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QVariant>
#include <QtGlobal>

#include <utility>

#include "ShadButton.h"
#include "ShadDataTable.h"
#include "ShadDataTableModel.h"
#include "ShadDropdownMenu.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadPagination.h"
#include "ShadStyleHelper.h"
#include "ShadTable.h"

ShadDataTablePrivate::ShadDataTablePrivate()
{
}

ShadDataTablePrivate::~ShadDataTablePrivate()
{
}

void ShadDataTablePrivate::updateVisibleColumns()
{
    QList<int> columns;
    const int columnCount = _proxyModel->columnCount();
    for (int column = 0; column < columnCount; ++column)
    {
        if (!_hiddenColumns.contains(column))
        {
            columns.append(column);
        }
    }
    _tableModel->setVisibleColumns(columns);
    updateTableColumns();
}

void ShadDataTablePrivate::updateTableColumns()
{
    _table->setSelectionColumn(_tableModel->selectionColumn());
    _table->setActionColumn(_tableModel->actionColumn());
    // 先清除上一次映射的列设置，列数可能因隐藏列或行选择列而变化
    const int tableColumnCount = _tableModel->columnCount();
    const int clearCount = qMax(_lastTableColumnCount, tableColumnCount);
    for (int column = 0; column < clearCount; ++column)
    {
        _table->setColumnSortable(column, false);
        _table->setColumnFixedWidth(column, -1);
        _table->clearColumnImage(column);
        _table->setColumnUserWidth(column, -1);
    }
    _lastTableColumnCount = tableColumnCount;
    for (const int column : std::as_const(_sortableColumns))
    {
        const int tableColumn = _tableModel->tableColumn(column);
        if (tableColumn >= 0)
        {
            _table->setColumnSortable(tableColumn, true);
        }
    }
    for (auto iterator = _fixedWidths.cbegin(); iterator != _fixedWidths.cend(); ++iterator)
    {
        const int tableColumn = _tableModel->tableColumn(iterator.key());
        if (tableColumn >= 0)
        {
            _table->setColumnFixedWidth(tableColumn, iterator.value());
        }
    }
    for (auto iterator = _imageColumns.cbegin(); iterator != _imageColumns.cend(); ++iterator)
    {
        const int tableColumn = _tableModel->tableColumn(iterator.key());
        if (tableColumn >= 0)
        {
            _table->setColumnImage(tableColumn, iterator.value().size, iterator.value().shape);
        }
    }
    for (auto iterator = _userWidths.cbegin(); iterator != _userWidths.cend(); ++iterator)
    {
        const int tableColumn = _tableModel->tableColumn(iterator.key());
        if (tableColumn >= 0)
        {
            _table->setColumnUserWidth(tableColumn, iterator.value());
        }
    }
    _table->setSortIndicator(_sortColumn >= 0 ? _tableModel->tableColumn(_sortColumn) : -1, _sortOrder);
}

void ShadDataTablePrivate::updatePagination()
{
    // 同步分页的页数与当前页，不触发 currentPageChanged
    {
        const QSignalBlocker blocker(_pagination);
        _pagination->setPageCount(qMax(1, _tableModel->getPageCount()));
        _pagination->setCurrentPage(_tableModel->getPageIndex() + 1);
    }
    _summaryLabel->setText(_summaryFormat.arg(_tableModel->getFilteredSelectedCount()).arg(_tableModel->getFilteredRowCount()));
    _summaryLabel->setVisible(_tableModel->selectionColumn() >= 0);
}

void ShadDataTablePrivate::updateToolbar()
{
    _filterInput->setVisible(_filterColumn >= 0);
    _columnsButton->setVisible(!_hideableColumns.isEmpty());
    _toolbar->setVisible(_filterColumn >= 0 || !_hideableColumns.isEmpty());
}

void ShadDataTablePrivate::showColumnsMenu()
{
    Q_Q(ShadDataTable);
    // 每次打开时按当前列显示状态重建，对应 DropdownMenuCheckboxItem（capitalize）
    _columnsMenu->clear();
    const int columnCount = _proxyModel->columnCount();
    for (int column = 0; column < columnCount; ++column)
    {
        if (!_hideableColumns.contains(column))
        {
            continue;
        }
        QString text = _proxyModel->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString();
        if (!text.isEmpty())
        {
            text[0] = text.at(0).toUpper();
        }
        QAction* action = _columnsMenu->addCheckableItem(text, !_hiddenColumns.contains(column));
        QObject::connect(action, &QAction::toggled, q, [q, column](bool checked) { q->setColumnVisible(column, checked); });
    }
    _columnsMenu->popup(_columnsButton, Qt::AlignRight);
}

void ShadDataTablePrivate::handleSortRequested(int tableColumn)
{
    const int column = _tableModel->proxyColumn(tableColumn);
    if (column < 0)
    {
        return;
    }
    // 第一次点击升序，再次点击在升序与降序之间切换
    _sortOrder = column == _sortColumn && _sortOrder == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder;
    _sortColumn = column;
    _proxyModel->sort(column, _sortOrder);
    _tableModel->setPage(0, _pageSize);
    updateTableColumns();
}

void ShadDataTablePrivate::handleActionRequested(int tableRow, const QRect& buttonRect)
{
    Q_Q(ShadDataTable);
    _actionRow = _tableModel->sourceRow(tableRow);
    Q_EMIT q->rowActionRequested(_actionRow);
    if (!_rowActionMenu.isNull())
    {
        // 对应 DropdownMenuContent align="end"
        _rowActionMenu->popup(buttonRect, Qt::AlignRight);
    }
}
