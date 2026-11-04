#ifndef SHADTREEDELEGATE_H
#define SHADTREEDELEGATE_H

#include <QModelIndex>
#include <QRect>
#include <QSize>
#include <QStyledItemDelegate>

class QAbstractItemModel;
class QEvent;
class QPainter;
class QStyleOptionViewItem;

// ShadTreeView 的单元格绘制，组件库内部使用：只绘制内容（复选框、图标、文字），行背景与展开箭头由 ShadTreeView 绘制
// 第一列内容紧跟展开箭头（左侧 4px），其余列左右 8px；复选框与图标 16px，元素间距 8px
// 复选框读写 Qt::CheckStateRole，外观与 ShadCheckbox 相同，点击或按空格切换
// 带 Qt::ItemIsAutoTristate 的父节点联动：勾选或取消时同步全部子节点，子节点变化后父节点按子节点显示选中、半选或未选中
// 图标优先使用 ShadTreeView::IconRole 中的 Lucide 图标，Folder 在节点展开时显示为 FolderOpen；否则使用 Qt::DecorationRole
// 选中的行文字为 font-medium、accent-foreground 色；没有 ItemIsEnabled 的节点半透明
class ShadTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:
        explicit ShadTreeDelegate(QObject* parent = nullptr);
        ~ShadTreeDelegate() override;

        void setItemHeight(int height);
        int getItemHeight() const;

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

    private:
        // 单元格是否显示复选框
        static bool hasCheckbox(const QModelIndex& index);
        // 复选框在单元格中的实际位置（RTL 时镜像）
        static QRect checkboxRect(const QStyleOptionViewItem& option, const QModelIndex& index);
        // 单元格内容的起始内边距
        static int startPadding(const QModelIndex& index);
        // 把 index 的全部子孙节点设为同一勾选状态
        static void setDescendantsCheckState(QAbstractItemModel* model, const QModelIndex& index, Qt::CheckState state);
        // 从 index 的父节点开始向上，按子节点重新计算带 ItemIsAutoTristate 的祖先的勾选状态
        static void updateAncestorsCheckState(QAbstractItemModel* model, const QModelIndex& index);

        int _itemHeight{32};
};

#endif // SHADTREEDELEGATE_H
