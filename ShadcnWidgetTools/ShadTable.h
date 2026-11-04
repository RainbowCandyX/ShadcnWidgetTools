#ifndef SHADTABLE_H
#define SHADTABLE_H

#include <QRect>
#include <QSize>
#include <QString>
#include <QTableView>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadProperty.h"

class QAbstractItemModel;
class QEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;

// 表格，对应 shadcn Table：接受任意 QAbstractItemModel，只负责外观
// 表头 h-10 px-2 font-medium，单元格 p-2，每行 border-b，悬停 bg-muted/50；对齐读取 TextAlignmentRole，字重等读取 FontRole
// 列宽按内容计算，宽度有剩余时按比例铺满（对应 w-full 的自动表格布局），内容超宽时横向滚动
// 默认开启自动高度：高度随行数撑开、不在表格内部纵向滚动，由外层页面滚动
// 图片列（setColumnImage）：Qt::DecorationRole 的 QPixmap / QImage / QIcon 按 object-cover 显示为圆角矩形或圆形，有文字时显示在图片右侧
// 所有行等高：默认按单元格内容自动计算（普通行 36，有行操作列时 48，图片列为图片高度加 16，有说明文字时至少 56）
class ShadTablePrivate;

class SHADCN_EXPORT ShadTable : public QTableView
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTable)
    Q_PROPERTY(bool autoHeight READ isAutoHeight WRITE setAutoHeight)
    Q_PROPERTY(QString caption READ getCaption WRITE setCaption)
    Q_PROPERTY(QString emptyText READ getEmptyText WRITE setEmptyText)
    Q_PROPERTY(int defaultRowHeight READ getDefaultRowHeight WRITE setDefaultRowHeight)

    public:
        // 单元格第二行说明文字的数据角色（QString）：有内容时文字下方显示一行 muted-foreground 色的说明，例如头像列中姓名下方的邮箱
        static constexpr int DescriptionRole = Qt::UserRole + 0x201;

        explicit ShadTable(QWidget* parent = nullptr);
        ~ShadTable() override;

        void setModel(QAbstractItemModel* model) override;

        // 固定列宽（对应 w-[100px]），不参与剩余宽度分配；width 小于 0 时取消
        void setColumnFixedWidth(int column, int width);

        // 表尾一个单元格，对应 TableFooter：bg-muted/50 border-t font-medium；span 为跨越的列数（对应 colSpan）
        void setFooterText(int column, const QString& text, Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter, int span = 1);
        void clearFooter();

        // 表格下方的说明文字，对应 TableCaption：mt-4 text-sm text-muted-foreground，水平居中
        void setCaption(const QString& caption);
        QString getCaption() const;

        // 没有数据时在表体居中显示的文字（对应 h-24 text-center 的 "No results." 行），默认为空不显示
        void setEmptyText(const QString& text);
        QString getEmptyText() const;

        // 自动高度，默认开启；关闭后表格内部纵向滚动，表尾与说明文字跟在最后一行之后，只在内容不超过可视高度时可见
        void setAutoHeight(bool autoHeight);
        bool isAutoHeight() const;

        // 行选择列：该列显示复选框（读写模型的 Qt::CheckStateRole），点击单元格切换；表头显示全选复选框（支持半选）；勾选的行为 bg-muted
        void setSelectionColumn(int column);
        int getSelectionColumn() const;

        // 行操作列：该列每行显示 more-horizontal 图标按钮，点击时发出 actionRequested；设置后行高为 48px
        void setActionColumn(int column);
        int getActionColumn() const;

        // 可排序列：表头显示 ghost 按钮与排序图标，点击时发出 sortRequested，实际排序由调用方完成
        void setColumnSortable(int column, bool sortable);
        // 表头排序图标的状态，column 为 -1 时都显示未排序图标
        void setSortIndicator(int column, Qt::SortOrder order);

        // 图片列：图片尺寸与形状按列设置，同一表格中可以有多个不同尺寸的图片列；没有图片的单元格显示占位块
        void setColumnImage(int column, const QSize& size, ShadTableType::ImageShape shape = ShadTableType::Rounded);
        void clearColumnImage(int column);

        // 统一行高，-1（默认）为按内容自动计算；函数名避开 QTableView::setRowHeight(int row, int height)
        void setDefaultRowHeight(int height);
        int getDefaultRowHeight() const;

        // 拖动表头列的结束边缘调整列宽，双击边缘恢复自动宽度，默认开启；行选择列、行操作列与固定宽度的列不能拖动
        // 拖动过的列保持该宽度，其余列仍按内容计算并分配剩余宽度；所有列都拖动过时剩余宽度留在右侧
        void setColumnsResizable(bool resizable);
        bool isColumnsResizable() const;
        // 拖动后的列宽，可用于保存与恢复；width 小于 0 时恢复自动宽度（不发出 columnResized）
        void setColumnUserWidth(int column, int width);
        int getColumnUserWidth(int column) const;
        // 全部恢复自动宽度
        void resetColumnWidths();

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void sortRequested(int column);
        // row 为视图中的行号，buttonRect 为按钮的屏幕坐标区域，可直接传给 ShadDropdownMenu::popup
        void actionRequested(int row, const QRect& buttonRect);
        // 用户拖动结束或双击恢复时发出，恢复自动宽度时 width 为 -1
        void columnResized(int column, int width);

    protected:
        bool viewportEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADTABLE_H
