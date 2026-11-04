#ifndef SHADTREEVIEW_H
#define SHADTREEVIEW_H

#include <QModelIndex>
#include <QTreeView>
#include <QtCore/qnamespace.h>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPainter;
class QRect;
class QStyleOptionViewItem;

// 树形视图：继承 QTreeView，模型、展开折叠、选择、拖放与键盘导航沿用 Qt 接口，只负责外观
// 行高 32（h-8），rounded-md，文字 text-sm；悬停 bg-accent，选中 bg-accent font-medium；行背景从该行的展开箭头处开始
// 展开箭头为 16px 的 ChevronRight（muted-foreground），展开时 150ms 旋转 90 度；每级缩进 20px，祖先层级显示 1px 缩进线
// 复选框读写 Qt::CheckStateRole，外观与 ShadCheckbox 相同；带 Qt::ItemIsAutoTristate（例如 QStandardItem::setAutoTristate）的父节点与子节点联动：
// 勾选或取消父节点时同步全部子节点，点击子节点后父节点显示选中、半选或未选中；联动只在点击或按空格时进行，代码修改勾选状态时需自行设置父节点
// 图标可用 IconRole 指定 Lucide 图标，或使用 Qt::DecorationRole
// 键盘导航时当前行显示 2px 焦点环；表头默认隐藏，显示后与 ShadTable 的表头外观一致；默认不可编辑（NoEditTriggers）
class ShadTreeViewPrivate;

class SHADCN_EXPORT ShadTreeView : public QTreeView
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTreeView)
    Q_PROPERTY(int itemHeight READ getItemHeight WRITE setItemHeight)
    Q_PROPERTY(bool indentGuidesVisible READ isIndentGuidesVisible WRITE setIndentGuidesVisible)

    public:
        // 存放 ShadIconType::IconName 的数据角色，例如 item->setData(ShadIconType::Folder, ShadTreeView::IconRole)
        // Folder 图标在节点展开时自动显示为 FolderOpen
        static constexpr int IconRole = Qt::UserRole + 0x200;

        explicit ShadTreeView(QWidget* parent = nullptr);
        ~ShadTreeView() override;

        // 行高，默认 32
        void setItemHeight(int height);
        int getItemHeight() const;

        // 缩进线，默认显示
        void setIndentGuidesVisible(bool visible);
        bool isIndentGuidesVisible() const;

    protected:
        void drawRow(QPainter* painter, const QStyleOptionViewItem& options, const QModelIndex& index) const override;
        void drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const override;
        bool viewportEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
};

#endif // SHADTREEVIEW_H
