#ifndef SHADDROPDOWNMENUPRIVATE_H
#define SHADDROPDOWNMENUPRIVATE_H

#include <QElapsedTimer>
#include <QList>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QAction;
class QMouseEvent;
class QPainter;
class QTimer;
class QVariantAnimation;
class QWidget;
class ShadDropdownMenu;

class ShadDropdownMenuPrivate
{
    SHAD_D_CREATE(ShadDropdownMenu)

    public:
        ShadDropdownMenuPrivate();
        ~ShadDropdownMenuPrivate();

        enum class EntryKind
        {
            Item,
            CheckableItem,
            RadioItem,
            Submenu,
            Label,
            Widget,
            Separator,
        };

        struct Entry
        {
            EntryKind kind{EntryKind::Item};
            QAction* action{nullptr};
            QPointer<QWidget> widget;
            QPointer<ShadDropdownMenu> submenu;
            QString labelText;
            ShadIconType::IconName icon{ShadIconType::None};
            QString shortcutText;
            bool isDestructive{false};
            bool isInset{false};
        };

        // 弹出层四周留给 shadow-md 与打开动画位移的透明边距
        static constexpr int kShadowMargin = 12;
        // 与锚点的间距 sideOffset=4，与屏幕边缘保持的最小距离
        static constexpr int kSideOffset = 4;
        static constexpr int kScreenMargin = 8;

        bool isVisibleEntry(int index) const;
        bool isSelectable(int index) const;
        int entryHeight(int index) const;
        // 卡片在窗口中的区域（含打开动画的位移）
        QRect cardRect() const;
        // 第 index 项在窗口中的区域，不可见的项返回空矩形
        QRect entryRect(int index) const;
        int entryAt(const QPoint& pos) const;
        // 卡片内容尺寸（含边框与内边距）
        QSize cardSize() const;
        // 展示组件跟随所在行与打开动画的位移
        void layoutWidgets();

        void setHighlight(int index);
        // 沿 step 方向移动高亮，跳过不可选的项，到达两端时循环
        void moveHighlight(int step);
        void highlightEdge(bool isFirst);
        void activate(int index, bool isKeyboard = false);
        void hoverEntry(int index);
        void openSubmenu(int index, bool highlightFirst);
        void closeSubmenu();
        // 卡片外的鼠标事件转给父菜单
        bool forwardToParent(QMouseEvent* event);
        ShadDropdownMenu* rootMenu();
        // 同步到子菜单
        void setMenubarStyle(bool isMenubarStyle);
        void handleTypeahead(const QString& text);
        // 以卡片左上角的屏幕坐标 cardTopLeft 显示菜单，side 为菜单位于锚点的方向，决定滑入方向
        void showAt(const QPoint& cardTopLeft, ShadPlacementType::Side side);
        // 锚点可视边缘的屏幕矩形，扣除焦点环边距；侧栏菜单按钮的焦点环在内侧，不扣除
        static QRect anchorRect(const QWidget* anchor);
        // 在屏幕坐标的锚点矩形下方（空间不足时上方）弹出；alignment 为逻辑方向，AlignRight 表示结束侧（对应 align="end"，RTL 时为左对齐）
        void popupForRect(const QRect& anchorRect, Qt::Alignment alignment, Qt::LayoutDirection direction);
        // 没有锚点组件时使用的布局方向：父组件的方向，没有父组件时为程序的方向
        Qt::LayoutDirection fallbackDirection() const;
        void paintEntry(QPainter* painter, int index) const;

        QList<Entry> _entries;
        QPointer<QWidget> _anchor;
        bool _isMatchAnchorWidth{false};
        // 本次弹出时锚点的可视宽度，popupAt 弹出时为 0
        int _anchorWidth{0};
        QVariantAnimation* _openAnimation{nullptr};
        qreal _openProgress{1.0};
        ShadPlacementType::Side _side{ShadPlacementType::Bottom};
        int _highlightIndex{-1};
        QString _typeaheadBuffer;
        QElapsedTimer _typeaheadTimer;

        bool _isMenubarStyle{false};
        bool _isContextMenu{false};
        QPointer<ShadDropdownMenu> _parentMenu;
        QAction* _menuAction{nullptr};
        QPointer<ShadDropdownMenu> _openSubmenu;
        int _openSubmenuIndex{-1};
        // 到时打开 _pendingSubmenuIndex 的子菜单，为 -1 时收起子菜单
        QTimer* _submenuTimer{nullptr};
        int _pendingSubmenuIndex{-1};
};

#endif // SHADDROPDOWNMENUPRIVATE_H
