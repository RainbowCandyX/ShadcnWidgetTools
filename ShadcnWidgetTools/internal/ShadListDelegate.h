#ifndef SHADLISTDELEGATE_H
#define SHADLISTDELEGATE_H

#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QRect>
#include <QSize>
#include <QStyledItemDelegate>

#include "ShadDef.h"

class QAbstractItemModel;
class QEvent;
class QPainter;
class QStyleOptionViewItem;

// ShadListView 的行绘制委托，组件库内部使用
class ShadListDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:
        explicit ShadListDelegate(QObject* parent = nullptr);
        ~ShadListDelegate() override;

        // 按尺寸与是否显示描述计算行高
        static int rowHeight(ShadItemType::Size size, bool descriptionVisible);

        void setItemSize(ShadItemType::Size size);
        ShadItemType::Size getItemSize() const;
        void setDescriptionVisible(bool visible);
        bool isDescriptionVisible() const;
        void setImageShape(ShadTableType::ImageShape shape);
        ShadTableType::ImageShape getImageShape() const;
        // 悬停行由 ShadListView 记录后传入
        void setHoveredIndex(const QModelIndex& index);
        QModelIndex getHoveredIndex() const;
        // 只有键盘导航时当前行显示焦点环
        void setFocusVisible(bool visible);

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    private:
        static bool hasCheckbox(const QModelIndex& index);
        // 结合行尺寸判断是否显示描述
        bool showsDescription() const;
        // 随行尺寸变化的内边距与元素间距
        int horizontalPadding() const;
        int gap() const;
        int imageSide() const;
        // 复选框在行中的位置
        QRect checkboxRect(const QStyleOptionViewItem& option) const;

        ShadItemType::Size _itemSize{ShadItemType::Medium};
        bool _isDescriptionVisible{true};
        ShadTableType::ImageShape _imageShape{ShadTableType::Rounded};
        QPersistentModelIndex _hoveredIndex;
        bool _isFocusVisible{false};
};

#endif // SHADLISTDELEGATE_H
