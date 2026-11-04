#ifndef SHADNAVIGATIONMENUPRIVATE_H
#define SHADNAVIGATIONMENUPRIVATE_H

#include <QElapsedTimer>
#include <QList>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadProperty.h"

class QAction;
class QHBoxLayout;
class QObject;
class QTimer;
class ShadNavigationMenu;
class ShadNavigationMenuTrigger;
class ShadNavigationMenuViewport;

class ShadNavigationMenuPrivate
{
    SHAD_D_CREATE(ShadNavigationMenu)

    public:
        ShadNavigationMenuPrivate();
        ~ShadNavigationMenuPrivate();

        struct Item
        {
            ShadNavigationMenuTrigger* trigger{nullptr};
            QPointer<QWidget> content;
            QAction* action{nullptr};
        };

        // gap-1
        static constexpr int kGap = 4;
        static constexpr int kOpenDelayMs = 200;
        // 关闭后这段时间内悬停直接打开
        static constexpr int kSkipDelayMs = 300;
        static constexpr int kCloseDelayMs = 150;

        int indexOfTrigger(const QObject* trigger) const;
        int triggerAt(const QPoint& globalPos) const;
        bool hasMenu(int index) const;
        void openMenu(int index, bool focusContent);
        void onHoverChanged(int index, bool hovered);
        void onTriggerClicked(int index);
        void onViewportClosed();
        void syncHover();
        int wrappedIndex(int index, int step) const;
        void focusTrigger(int index, Qt::FocusReason reason);
        // 只有最近获得焦点的菜单项参与 Tab 切换
        void updateFocusPolicies();
        QRect barRect() const;

        QHBoxLayout* _layout{nullptr};
        QList<Item> _items;
        ShadNavigationMenuViewport* _viewport{nullptr};
        QTimer* _openTimer{nullptr};
        QTimer* _closeTimer{nullptr};
        QElapsedTimer _closedTimer;
        int _openIndex{-1};
        int _focusIndex{0};
        int _pendingIndex{-1};
        // 点击关闭后，光标离开前不再悬停打开
        int _suppressHoverIndex{-1};
        bool _isKeyboardClose{false};
};

#endif // SHADNAVIGATIONMENUPRIVATE_H
