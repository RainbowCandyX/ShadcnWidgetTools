#ifndef SHADTABLEDELEGATE_H
#define SHADTABLEDELEGATE_H

#include <QFont>
#include <QHash>
#include <QModelIndex>
#include <QPixmap>
#include <QRect>
#include <QSize>
#include <QStyledItemDelegate>
#include <QVariant>
#include <QtGlobal>

#include "ShadDef.h"

class QPainter;
class QStyleOptionViewItem;

// ShadTable 的单元格绘制，对应 TableRow / TableCell，不对外导出
// 行：border-b（表体最后一行除外），悬停 bg-muted/50，勾选 bg-muted；单元格 p-2 whitespace-nowrap
// 行选择列绘制复选框，行操作列绘制 ghost 图标按钮（more-horizontal）
// 图片列：Qt::DecorationRole 的图片按 object-cover 缩放裁剪为圆角矩形或圆形，没有图片时为占位块；有文字时显示在图片右侧
// 任意单元格的 ShadTable::DescriptionRole 有内容时，文字下方显示一行 muted 色说明
class ShadTableDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:
        // 对应 p-2
        static constexpr int kCellPaddingX = 8;
        // 行操作按钮 h-8 w-8
        static constexpr int kActionButtonSize = 32;
        static constexpr int kCheckboxSize = 16;
        // 图片列上下内边距（p-2）、图片与文字的间距（gap-3）
        static constexpr int kImagePaddingY = 8;
        static constexpr int kImageTextGap = 12;
        // 单行单元格的默认高度：p-2 + text-sm 行高
        static constexpr int kDefaultRowHeight = 36;

        // 图片列的尺寸与形状
        struct ImageColumn
        {
            QSize size;
            ShadTableType::ImageShape shape{ShadTableType::Rounded};
        };

        explicit ShadTableDelegate(QObject* parent = nullptr);
        ~ShadTableDelegate() override;

        void setSelectionColumn(int column);
        void setActionColumn(int column);
        void setImageColumns(const QHash<int, ImageColumn>& columns);
        void setHoveredRow(int row);
        void setHoveredActionRow(int row);
        int getHoveredRow() const;
        int getHoveredActionRow() const;

        // 单元格内行操作按钮与复选框的区域，direction 为表格的布局方向，RTL 时在单元格内镜像
        QRect actionButtonRect(const QRect& cellRect, const QModelIndex& index, Qt::LayoutDirection direction) const;
        QRect checkboxRect(const QRect& cellRect, Qt::LayoutDirection direction) const;
        // 该行在行选择列中是否勾选（对应 data-state="selected"）
        bool isRowChecked(const QModelIndex& index) const;
        // 单元格字体：在表格字体基础上应用模型 FontRole 中显式设置的属性（例如字重）
        static QFont cellFont(const QFont& baseFont, const QModelIndex& index);
        // 按形状裁剪并缓存的图片，ShadListView 共用
        static QPixmap renderedImage(const QVariant& decoration, const QSize& size, ShadTableType::ImageShape shape, qreal devicePixelRatio);

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    private:
        static Qt::Alignment cellAlignment(const QModelIndex& index);
        // 文字与说明：有说明时两行在 textRect 中垂直居中，否则单行；textRect 为实际位置
        void paintText(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, const QRect& textRect, Qt::LayoutDirection direction) const;
        // 图片列的图片或占位块，imageRect 为实际位置
        void paintImage(QPainter* painter, const QRect& imageRect, const QVariant& decoration, const ImageColumn& image) const;

        QHash<int, ImageColumn> _imageColumns;
        int _selectionColumn{-1};
        int _actionColumn{-1};
        int _hoveredRow{-1};
        int _hoveredActionRow{-1};
};

#endif // SHADTABLEDELEGATE_H
