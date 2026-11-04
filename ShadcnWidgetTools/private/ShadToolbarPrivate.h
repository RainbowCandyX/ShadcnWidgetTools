#ifndef SHADTOOLBARPRIVATE_H
#define SHADTOOLBARPRIVATE_H

#include <QList>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QAction;
class QWidget;
class ShadDropdownMenu;
class ShadToolbar;
class ShadToolbarButton;

class ShadToolbarPrivate
{
    SHAD_D_CREATE(ShadToolbar)

    public:
        ShadToolbarPrivate();
        ~ShadToolbarPrivate();

        static constexpr int kPadding = 4;
        static constexpr int kSpacing = 4;
        static constexpr int kSeparatorMargin = 4;
        static constexpr int kSeparatorLength = 16;

        enum class ItemKind
        {
            Action = 0,
            Menu,
            Widget,
            Separator,
            Stretch,
        };

        struct Item
        {
            ItemKind kind{ItemKind::Action};
            QPointer<QAction> action;
            bool isOwnedAction{false};
            QPointer<ShadDropdownMenu> menu;
            ShadToolbarButton* button{nullptr};
            QPointer<QWidget> widget;
            QRect separatorRect;
        };

        bool isHorizontal() const;
        ShadToolbarButton* createButton(QAction* action, ShadIconType::IconName icon, ShadDropdownMenu* menu);
        void configureButton(ShadToolbarButton* button) const;
        void addActionItem(QAction* action, ShadIconType::IconName icon, bool isOwned);
        void appendItem(const Item& item);
        int indexOfAction(const QAction* action) const;
        void removeItemAt(int index);
        void removeDeadItems();
        bool isItemShown(const Item& item) const;
        int itemLength(const Item& item) const;
        int crossLength() const;
        void requestLayout();
        void layoutItems();
        void rebuildOverflowMenu();
        void disconnectItems();

        QList<Item> _items;
        Qt::Orientation _orientation{Qt::Horizontal};
        ShadToolbarType::ButtonStyle _buttonStyle{ShadToolbarType::IconOnly};
        bool _isBordered{false};
        QAction* _overflowAction{nullptr};
        ShadDropdownMenu* _overflowMenu{nullptr};
        ShadToolbarButton* _overflowButton{nullptr};
        int _overflowStart{-1};
};

#endif // SHADTOOLBARPRIVATE_H
