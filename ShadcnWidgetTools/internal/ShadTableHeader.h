#ifndef SHADTABLEHEADER_H
#define SHADTABLEHEADER_H

#include <QHeaderView>
#include <QRect>
#include <QSet>
#include <QSize>
#include <QString>

class QEvent;
class QMouseEvent;
class QPaintEvent;
class QPainter;
class QPoint;

// ShadTable 的表头，对应 TableHead：h-10 px-2 font-medium，下方 1px 边框，不对外导出
// 可排序列显示 ghost 按钮（文字 + 排序图标），行选择列显示全选复选框
// 开启拖动调整列宽后：列的结束边缘左右 4px 为拖动把手，悬停时显示 2px border 色竖条，拖动中为 primary 色，双击发出重置请求
// 本类只发出请求信号，实际列宽由使用方设置（ShadTable 保存拖动后的宽度并重新分配其余列）
class ShadTableHeader : public QHeaderView
{
    Q_OBJECT

    public:
        // 对应 h-10
        static constexpr int kHeaderHeight = 40;
        // 对应 px-2
        static constexpr int kCellPaddingX = 8;
        // 拖动调整时的最小列宽
        static constexpr int kMinimumResizeWidth = 48;

        explicit ShadTableHeader(QWidget* parent = nullptr);
        ~ShadTableHeader() override;

        void setColumnSortable(int column, bool sortable);
        bool isColumnSortable(int column) const;
        // column 为 -1 时没有排序列
        void setSortState(int column, Qt::SortOrder order);
        void setSelectionColumn(int column);

        // 表头内容所需宽度（含内边距），用于计算列宽
        int contentWidth(int column) const;
        // 当前页（全部行）的勾选状态：全部勾选、部分勾选或没有勾选
        Qt::CheckState selectionState() const;

        // 拖动调整列宽，默认关闭
        void setResizeEnabled(bool enabled);
        bool isResizeEnabled() const;
        // 不能拖动的列（例如行选择列、行操作列、固定宽度的列）
        void setNonResizableColumns(const QSet<int>& columns);

        QSize sizeHint() const override;

    Q_SIGNALS:
        void sortClicked(int column);
        void selectAllClicked();
        // 拖动中按鼠标位置计算的新列宽（不小于 kMinimumResizeWidth）
        void columnResizing(int column, int width);
        // 拖动结束，width 为该列当前宽度
        void columnResizeFinished(int column, int width);
        // 双击拖动把手：恢复自动宽度
        void columnResetRequested(int column);

    protected:
        void paintEvent(QPaintEvent* event) override;
        void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;

    private:
        // 可排序列 ghost 按钮在表头中的区域
        QRect sortButtonRect(int column) const;
        // 行选择列复选框在表头中的区域
        QRect checkboxRect() const;
        QRect sectionRect(int column) const;
        QString headerText(int column) const;
        Qt::Alignment headerAlignment(int column) const;
        void setHoveredSortColumn(int column);
        // 列的结束边缘在视口中的横坐标（RTL 时为左边缘）
        int resizeEdge(int column) const;
        // pos 处的拖动把手所属的列，没有时为 -1
        int resizeHandleAt(const QPoint& pos) const;
        void setHoveredResizeColumn(int column);

        QSet<int> _sortableColumns;
        QSet<int> _nonResizableColumns;
        bool _isResizeEnabled{false};
        int _hoveredResizeColumn{-1};
        // 正在拖动的列与拖动开始时的鼠标位置、列宽
        int _resizingColumn{-1};
        int _resizeStartX{0};
        int _resizeStartWidth{0};
        int _sortColumn{-1};
        Qt::SortOrder _sortOrder{Qt::AscendingOrder};
        int _selectionColumn{-1};
        int _hoveredSortColumn{-1};
        int _pressedSortColumn{-1};
        bool _isCheckboxPressed{false};
};

#endif // SHADTABLEHEADER_H
