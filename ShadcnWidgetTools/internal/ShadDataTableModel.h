#ifndef SHADDATATABLEMODEL_H
#define SHADDATATABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QMetaObject>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QSet>
#include <QVariant>

class QAbstractProxyModel;

// ShadDataTable 交给 ShadTable 显示的模型，不对外导出
// 包装排序筛选代理：只显示当前页的行与可见列，并按需在最左侧插入行选择列、最右侧插入行操作列
// 行选择状态以源模型（代理的 sourceModel）第 0 列的持久索引保存，排序、筛选、翻页后保持不变
class ShadDataTableModel : public QAbstractTableModel
{
    Q_OBJECT

    public:
        explicit ShadDataTableModel(QObject* parent = nullptr);
        ~ShadDataTableModel() override;

        // proxy 为排序筛选代理，其 sourceModel 为用户的数据模型
        void setProxyModel(QAbstractProxyModel* proxy);

        void setSelectionEnabled(bool enabled);
        void setActionEnabled(bool enabled);
        // 按显示顺序排列的可见列（代理中的列号）
        void setVisibleColumns(const QList<int>& columns);
        void setPage(int pageIndex, int pageSize);

        int getPageIndex() const;
        int getPageCount() const;
        // 筛选后的总行数
        int getFilteredRowCount() const;
        // 筛选后的行中已选择的行数
        int getFilteredSelectedCount() const;
        QList<int> getSelectedSourceRows() const;
        void clearSelection();

        // 表格列号与代理列号、表格行号与源模型行号的换算，不对应时返回 -1
        int selectionColumn() const;
        int actionColumn() const;
        int proxyColumn(int tableColumn) const;
        int tableColumn(int proxyColumn) const;
        int sourceRow(int tableRow) const;

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Q_SIGNALS:
        void selectionChanged();
        // 代理数据变化或翻页后发出，页码可能已被夹到有效范围
        void pageStateChanged();

    private:
        // 代理中当前页第 tableRow 行对应的索引
        QModelIndex proxyIndex(int tableRow, int proxyColumn) const;
        // 当前页第 tableRow 行在源模型第 0 列的索引，用于保存选择状态
        QModelIndex selectionKey(int tableRow) const;
        void resetFromProxy();

        QPointer<QAbstractProxyModel> _proxy;
        QList<QMetaObject::Connection> _proxyConnections;
        QList<int> _visibleColumns;
        QSet<QPersistentModelIndex> _selectedRows;
        bool _isSelectionEnabled{false};
        bool _isActionEnabled{false};
        int _pageIndex{0};
        int _pageSize{10};
};

#endif // SHADDATATABLEMODEL_H
