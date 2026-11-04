#ifndef SHADDROPDOWNMENU_H
#define SHADDROPDOWNMENU_H

#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QAction;
class QActionGroup;
class QEvent;
class QHideEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

// 下拉菜单，菜单项用 QAction 表示；复选项与单选项点击后菜单保持打开
class ShadDropdownMenuPrivate;

class SHADCN_EXPORT ShadDropdownMenu : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadDropdownMenu)
    friend class ShadMenubar;
    friend class ShadMenubarPrivate;

    public:
        explicit ShadDropdownMenu(QWidget* parent = nullptr);
        ~ShadDropdownMenu() override;

        // 普通菜单项，返回的 QAction 归菜单所有；shortcutText 只用于显示，不会注册快捷键
        QAction* addItem(const QString& text, ShadIconType::IconName icon = ShadIconType::None, const QString& shortcutText = QString());
        QAction* addCheckableItem(const QString& text, bool checked = false);
        // group 不归菜单所有，为 nullptr 时不互斥
        QAction* addRadioItem(const QString& text, QActionGroup* group, bool checked = false);
        // 返回的子菜单归本菜单所有
        ShadDropdownMenu* addMenu(const QString& text, ShadIconType::IconName icon = ShadIconType::None);
        // 子菜单在父菜单中的菜单项，不是子菜单时为 nullptr
        QAction* menuAction() const;
        // 分组标题，返回它是第几个分组标题（从 0 开始，clear 后重新计数），用于 setLabelText
        int addLabel(const QString& text, bool inset = false);
        // 修改第 labelIndex 个分组标题的文字，例如切换语言时；序号无效时不处理
        void setLabelText(int labelIndex, const QString& text);
        // 只用于展示的组件，例如用户信息；菜单接管父子关系，clear 时删除
        // 按 sizeHint 占一行，宽度随菜单拉伸；不响应鼠标与键盘，隐藏时不显示该行
        void addWidget(QWidget* widget);
        void addSeparator();
        void clear();

        // 危险操作样式，对应 variant="destructive"：文字与图标为 destructive 色
        void setItemDestructive(QAction* action, bool destructive);
        void setItemInset(QAction* action, bool inset);

        // 菜单宽度至少与锚点的可视宽度相同，对应 w-(--radix-dropdown-menu-trigger-width)，默认 false；popupAt 时不生效
        void setMatchAnchorWidth(bool match);
        bool isMatchAnchorWidth() const;

        // 在 anchor 下方弹出，空间不足时在上方；alignment 为 Qt::AlignRight 时与 anchor 结束侧对齐（对应 align="end"），否则与起始侧对齐
        // 与 Qt 的对齐规则一致：RTL 下 AlignLeft / AlignRight 左右互换，带 Qt::AlignAbsolute 时按物理方向对齐
        // 菜单使用 anchor 的布局方向；菜单打开时再次点击 anchor 会关闭菜单
        void popup(QWidget* anchor, Qt::Alignment alignment = Qt::AlignLeft);
        // 在 anchor 的 side 一侧弹出，沿该边按 align 对齐，对应 side / align；空间不足时翻到对侧，RTL 下 Left / Right 互换
        void popup(QWidget* anchor, ShadPlacementType::Side side, ShadPlacementType::Align align);
        // 以屏幕坐标的锚点矩形 anchorRect 弹出（不扣除焦点环边距），用于锚点不是独立组件的场景，例如表格单元格内的按钮
        // 菜单使用父组件的布局方向，没有父组件时使用程序的布局方向
        void popup(const QRect& anchorRect, Qt::Alignment alignment = Qt::AlignLeft);
        // 以屏幕坐标 globalPos 为菜单起始侧顶角弹出：LTR 时为左上角，RTL 时为右上角
        void popupAt(const QPoint& globalPos);

        QSize sizeHint() const override;

    Q_SIGNALS:
        // 普通菜单项被点击时发出，在 QAction::triggered 之前
        void itemTriggered(QAction* action);
        void aboutToHide();

    protected:
        void keyPressEvent(QKeyEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADDROPDOWNMENU_H
