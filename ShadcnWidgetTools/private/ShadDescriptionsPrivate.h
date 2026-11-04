#ifndef SHADDESCRIPTIONSPRIVATE_H
#define SHADDESCRIPTIONSPRIVATE_H

#include <QList>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>

#include "ShadDef.h"
#include "ShadProperty.h"

class QLabel;
class QWidget;
class ShadDescriptions;

class ShadDescriptionsPrivate
{
    SHAD_D_CREATE(ShadDescriptions)

    public:
        ShadDescriptionsPrivate();
        ~ShadDescriptionsPrivate();

        // 一项：文字项使用 textLabel，组件项使用 widget
        struct Item
        {
            QString label;
            QLabel* labelLabel{nullptr};
            QLabel* textLabel{nullptr};
            QPointer<QWidget> widget;
            int span{1};
        };

        // 随尺寸变化的间距与内边距
        struct Metrics
        {
            int columnGap{24};
            int rowGap{16};
            int cellPaddingX{12};
            int cellPaddingY{8};
        };

        // 一项在网格中的位置
        struct Placement
        {
            int item{0};
            int row{0};
            int column{0};
            int span{1};
        };

        // 一项的逻辑位置（LTR），单元格只用于带边框样式
        struct ItemGeometry
        {
            QRect labelCell;
            QRect contentCell;
            QRect label;
            QRect content;
        };

        // 整体排版结果，坐标为逻辑位置（LTR）
        struct LayoutResult
        {
            QRect titleRect;
            QRect extraRect;
            QRect tableRect;
            QList<ItemGeometry> items;
            int totalHeight{0};
        };

        Metrics metrics() const;
        // 标签显示的文字，含可选冒号
        QString labelText(const QString& label) const;
        // 计算各项在网格中的位置
        QList<Placement> placeItems() const;
        int rowCount(const QList<Placement>& placements) const;
        // 标签不换行时的自然宽度
        int labelNaturalWidth(int index) const;
        // QLabel 在给定宽度下的高度
        static int labelHeight(const QLabel* label, int width);
        // 内容在给定宽度下的尺寸
        QSize contentSize(int index, int width) const;
        // 操作区的尺寸，没有操作区时为空
        QSize extraSize() const;
        bool hasHeader() const;
        LayoutResult computeLayout(int width) const;
        // 无边框与带边框的表格排版，返回表格底部
        int layoutPlain(const QList<Placement>& placements, int width, int top, LayoutResult* result) const;
        int layoutBordered(const QList<Placement>& placements, int width, int top, LayoutResult* result) const;

        Item* itemAt(int index);
        const Item* itemAt(int index) const;
        void updateColors();
        // 放置子组件并记录绘制用的排版结果
        void layoutChildren();
        // 排版或尺寸相关的属性变化后刷新
        void updateContent();

        QString _title;
        QLabel* _titleLabel{nullptr};
        QPointer<QWidget> _extra;
        QList<Item> _items;
        int _columnCount{3};
        ShadDescriptionsType::Layout _itemLayout{ShadDescriptionsType::Horizontal};
        bool _isBordered{false};
        ShadDescriptionsType::Size _size{ShadDescriptionsType::Medium};
        bool _isColonVisible{false};
        // 最近一次 layoutChildren 的结果，paintEvent 使用
        LayoutResult _layout;
};

#endif // SHADDESCRIPTIONSPRIVATE_H
