#ifndef SHADDATATABLE_H
#define SHADDATATABLE_H

#include <QList>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QAbstractItemModel;
class QEvent;
class ShadDropdownMenu;
class ShadPagination;
class ShadTable;

// 数据表格，对应 shadcn Data Table：在 ShadTable 外组合筛选、排序、列显示、行选择、行操作与分页
// 上方：筛选输入框（max-w-sm）与右侧"列"按钮（列显示菜单）；中间：rounded-md border 包裹的表格；下方：已选择行数与 ShadPagination 分页（上一页、页码、下一页）
// 接口中的列号与行号都是源模型（setSourceModel 传入的模型）中的列号与行号
class ShadDataTablePrivate;

class SHADCN_EXPORT ShadDataTable : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadDataTable)

    public:
        explicit ShadDataTable(QWidget* parent = nullptr);
        ~ShadDataTable() override;

        // 数据源模型，不接管所有权；更换模型会清空行选择、排序与列显示设置
        void setSourceModel(QAbstractItemModel* model);
        QAbstractItemModel* getSourceModel() const;

        // 筛选：按该列做不区分大小写的包含匹配，-1 时不显示筛选输入框（默认）
        void setFilterColumn(int column);
        int getFilterColumn() const;
        void setFilterPlaceholderText(const QString& text);

        // 排序：可排序列的表头显示排序按钮，点击在升序与降序之间切换，排序后回到第一页
        void setColumnSortable(int column, bool sortable);
        // 排序使用的数据角色，默认 Qt::DisplayRole；数值列可把原始数值放在其他角色（例如 Qt::UserRole）中按数值排序
        void setSortRole(int role);

        // 列显示：可隐藏的列出现在"列"菜单中，没有可隐藏的列时不显示该按钮
        void setColumnHideable(int column, bool hideable);
        void setColumnVisible(int column, bool visible);
        bool isColumnVisible(int column) const;

        // 固定列宽（对应 w-[100px]），width 小于 0 时取消
        void setColumnFixedWidth(int column, int width);

        // 图片列：与 ShadTable::setColumnImage 相同，列号为源模型列号；第二行说明文字使用 ShadTable::DescriptionRole
        void setColumnImage(int column, const QSize& size, ShadTableType::ImageShape shape = ShadTableType::Rounded);
        void clearColumnImage(int column);

        // 调整列宽：与 ShadTable 相同，默认开启；拖动后的宽度按源模型列号保存，隐藏再显示列或开关行选择列后不会错位
        void setColumnsResizable(bool resizable);
        bool isColumnsResizable() const;
        void setColumnUserWidth(int column, int width);
        int getColumnUserWidth(int column) const;
        void resetColumnWidths();

        // 行选择：最左侧显示复选框列，表头复选框选择当前页全部行；下方显示已选择行数
        void setSelectionEnabled(bool enabled);
        bool isSelectionEnabled() const;
        QList<int> getSelectedRows() const;
        void clearSelection();

        // 行操作：设置后最右侧每行显示 more-horizontal 按钮，点击时发出 rowActionRequested 并弹出该菜单（右对齐）
        // 菜单项触发时可通过 getActionRow 得到对应的源模型行号；菜单不归 ShadDataTable 所有，传 nullptr 移除行操作列
        void setRowActionMenu(ShadDropdownMenu* menu);
        int getActionRow() const;

        // 分页，每页默认 10 行
        void setPageSize(int pageSize);
        int getPageSize() const;
        void setPageIndex(int pageIndex);
        int getPageIndex() const;
        int getPageCount() const;

        // 界面文字：空状态默认"没有结果"，列按钮默认"列"，翻页按钮默认"上一页 / 下一页"，已选择行数默认"已选择 %1 / %2 行"（%1 为已选择行数，%2 为筛选后的总行数）
        void setEmptyText(const QString& text);
        void setColumnsButtonText(const QString& text);
        void setPaginationTexts(const QString& previousText, const QString& nextText);
        void setSelectionSummaryFormat(const QString& format);

        // 内部表格，用于调整外观；其列号包含行选择列，与源模型列号不同
        ShadTable* getTable() const;
        // 下方的分页，用于调整外观（例如开启页码跳转）；页数与当前页由数据表格管理，翻页请使用 setPageIndex
        ShadPagination* getPagination() const;

    Q_SIGNALS:
        void selectionChanged();
        // 点击某行的操作按钮时发出，在弹出行操作菜单之前
        void rowActionRequested(int row);
        // 用户拖动结束或双击恢复时发出，column 为源模型列号，恢复自动宽度时 width 为 -1
        void columnResized(int column, int width);

    protected:
        void changeEvent(QEvent* event) override;

    private:
        // 设置筛选提示、列按钮、空结果与选择摘要的默认文字，使用者改过的保持不变
        void retranslate();
};

#endif // SHADDATATABLE_H
