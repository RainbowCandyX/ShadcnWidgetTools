#ifndef SHADDATATABLEPRIVATE_H
#define SHADDATATABLEPRIVATE_H

#include <QHash>
#include <QPointer>
#include <QRect>
#include <QSet>
#include <QSize>
#include <QString>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadProperty.h"

class QAbstractItemModel;
class QSortFilterProxyModel;
class QWidget;
class ShadButton;
class ShadDataTable;
class ShadDataTableModel;
class ShadDropdownMenu;
class ShadInput;
class ShadLabel;
class ShadPagination;
class ShadTable;

class ShadDataTablePrivate
{
    SHAD_D_CREATE(ShadDataTable)

    public:
        ShadDataTablePrivate();
        ~ShadDataTablePrivate();

        // 图片列的尺寸与形状（源模型列号）
        struct ImageColumn
        {
            QSize size;
            ShadTableType::ImageShape shape{ShadTableType::Rounded};
        };

        // 按隐藏设置重新计算可见列
        void updateVisibleColumns();
        // 把源模型列号的设置（可排序、排序状态、固定列宽、图片列、行选择列与行操作列）映射到表格列号
        void updateTableColumns();
        // 刷新翻页按钮可用状态与已选择行数
        void updatePagination();
        void updateToolbar();
        void showColumnsMenu();
        void handleSortRequested(int tableColumn);
        void handleActionRequested(int tableRow, const QRect& buttonRect);

        QPointer<QAbstractItemModel> _sourceModel;
        QSortFilterProxyModel* _proxyModel{nullptr};
        ShadDataTableModel* _tableModel{nullptr};
        ShadTable* _table{nullptr};
        QWidget* _toolbar{nullptr};
        ShadInput* _filterInput{nullptr};
        ShadButton* _columnsButton{nullptr};
        ShadDropdownMenu* _columnsMenu{nullptr};
        ShadLabel* _summaryLabel{nullptr};
        ShadPagination* _pagination{nullptr};
        QPointer<ShadDropdownMenu> _rowActionMenu;
        QSet<int> _sortableColumns;
        QSet<int> _hideableColumns;
        QSet<int> _hiddenColumns;
        QHash<int, int> _fixedWidths;
        QHash<int, ImageColumn> _imageColumns;
        // 拖动后的列宽（源模型列号），列映射变化后重新应用到表格
        QHash<int, int> _userWidths;
        QString _summaryFormat;
        int _filterColumn{-1};
        int _sortColumn{-1};
        Qt::SortOrder _sortOrder{Qt::AscendingOrder};
        int _pageSize{10};
        int _actionRow{-1};
        // 上次映射到表格时的列数，用于清除旧的列设置
        int _lastTableColumnCount{0};
};

#endif // SHADDATATABLEPRIVATE_H
