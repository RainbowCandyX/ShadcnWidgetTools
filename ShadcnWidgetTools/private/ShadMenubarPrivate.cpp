#include "ShadMenubarPrivate.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QMetaObject>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadDropdownMenuPrivate.h"
#include "ShadMenubar.h"
#include "ShadMenubarTrigger.h"
#include "ShadPlacement.h"

ShadMenubarPrivate::ShadMenubarPrivate()
{
}

ShadMenubarPrivate::~ShadMenubarPrivate()
{
}

int ShadMenubarPrivate::indexOfMenu(const QObject* menu) const
{
    for (int index = 0; index < int(_menus.size()); ++index)
    {
        if (_menus.at(index).menu == menu)
        {
            return index;
        }
    }
    return -1;
}

int ShadMenubarPrivate::indexOfTrigger(const QObject* trigger) const
{
    for (int index = 0; index < int(_menus.size()); ++index)
    {
        if (_menus.at(index).trigger == trigger)
        {
            return index;
        }
    }
    return -1;
}

int ShadMenubarPrivate::triggerAt(const QPoint& globalPos) const
{
    for (int index = 0; index < int(_menus.size()); ++index)
    {
        const ShadMenubarTrigger* trigger = _menus.at(index).trigger;
        if (trigger->isVisible() && trigger->isEnabled() && trigger->rect().contains(trigger->mapFromGlobal(globalPos)))
        {
            return index;
        }
    }
    return -1;
}

void ShadMenubarPrivate::openMenu(int index, int highlight)
{
    Q_Q(ShadMenubar);
    if (index < 0 || index >= int(_menus.size()))
    {
        return;
    }
    const Menu& entry = _menus.at(index);
    ShadDropdownMenuPrivate* menuPrivate = entry.menu->d_func();
    if (_openIndex != index || !entry.menu->isVisible())
    {
        if (_openIndex >= 0 && _openIndex < int(_menus.size()))
        {
            _isSwitching = true;
            _menus.at(_openIndex).menu->hide();
            _isSwitching = false;
        }
        _openIndex = index;
        entry.trigger->setOpen(true);
        const Qt::LayoutDirection direction = q->layoutDirection();
        entry.menu->setLayoutDirection(direction);
        menuPrivate->_anchor = entry.trigger;
        menuPrivate->_anchorWidth = 0;
        const QRect anchor(entry.trigger->mapToGlobal(QPoint(0, 0)), entry.trigger->size());
        const ShadPlacement::Result placement = ShadPlacement::place(anchor, menuPrivate->cardSize(), ShadPlacementType::Bottom, ShadPlacementType::Start, kSideOffset, direction);
        const int alignOffset = direction == Qt::RightToLeft ? -kAlignOffset : kAlignOffset;
        menuPrivate->showAt(placement.rect.topLeft() + QPoint(alignOffset, 0), placement.side);
    }
    if (highlight != 0)
    {
        menuPrivate->highlightEdge(highlight > 0);
    }
    _focusIndex = index;
    updateFocusPolicies();
}

void ShadMenubarPrivate::onMenuHidden(ShadDropdownMenu* menu)
{
    const int index = indexOfMenu(menu);
    if (index < 0)
    {
        return;
    }
    _menus.at(index).trigger->setOpen(false);
    if (_isSwitching)
    {
        return;
    }
    if (_openIndex == index)
    {
        _openIndex = -1;
    }
    focusTrigger(index, _isKeyboardClose ? Qt::TabFocusReason : Qt::PopupFocusReason);
    _isKeyboardClose = false;
}

int ShadMenubarPrivate::wrappedIndex(int index, int step) const
{
    const int count = int(_menus.size());
    if (count == 0)
    {
        return -1;
    }
    return ((index + step) % count + count) % count;
}

void ShadMenubarPrivate::focusTrigger(int index, Qt::FocusReason reason)
{
    if (index < 0 || index >= int(_menus.size()))
    {
        return;
    }
    _focusIndex = index;
    updateFocusPolicies();
    _menus.at(index).trigger->setFocus(reason);
}

void ShadMenubarPrivate::updateFocusPolicies()
{
    for (int index = 0; index < int(_menus.size()); ++index)
    {
        _menus.at(index).trigger->setFocusPolicy(index == _focusIndex ? Qt::StrongFocus : Qt::ClickFocus);
    }
}

bool ShadMenubarPrivate::usesNativeMenuBar() const
{
#ifdef Q_OS_MACOS
    return _isNativeMenuBar;
#else
    return false;
#endif
}

void ShadMenubarPrivate::requestNativeSync()
{
    Q_Q(ShadMenubar);
    if (_isNativeSyncPending)
    {
        return;
    }
    _isNativeSyncPending = true;
    QMetaObject::invokeMethod(q, [this]() { syncNativeMenuBar(); }, Qt::QueuedConnection);
}

void ShadMenubarPrivate::syncNativeMenuBar()
{
    Q_Q(ShadMenubar);
    _isNativeSyncPending = false;
    delete _nativeMenuBar.data();
    if (!usesNativeMenuBar())
    {
        if (!_nativeWindow.isNull())
        {
            _nativeWindow->removeEventFilter(q);
            _nativeWindow = nullptr;
        }
        return;
    }
    // 窗口显示后才有原生窗口，届时重建
    QWidget* window = q->window();
    if (_nativeWindow != window)
    {
        if (!_nativeWindow.isNull())
        {
            _nativeWindow->removeEventFilter(q);
        }
        _nativeWindow = window;
        window->installEventFilter(q);
    }
    _nativeMenuBar = new QMenuBar(window == q ? nullptr : window);
    // Qt::AA_DontUseNativeMenuBar 时会显示在窗口内
    if (!_nativeMenuBar->isNativeMenuBar())
    {
        qWarning("系统菜单栏不可用，ShadMenubar 的 nativeMenuBar 不生效");
        delete _nativeMenuBar.data();
        return;
    }
    for (const Menu& entry : std::as_const(_menus))
    {
        QMenu* nativeMenu = _nativeMenuBar->addMenu(entry.trigger->text());
        ShadDropdownMenu* menu = entry.menu;
        fillNativeMenu(nativeMenu, menu);
        QObject::connect(nativeMenu, &QMenu::aboutToShow, nativeMenu, [this, nativeMenu, menu]() { fillNativeMenu(nativeMenu, menu); });
    }
}

void ShadMenubarPrivate::fillNativeMenu(QMenu* nativeMenu, ShadDropdownMenu* menu)
{
    using EntryKind = ShadDropdownMenuPrivate::EntryKind;
    // clear 不删除共享的 QAction，子菜单需单独删除
    nativeMenu->clear();
    for (QMenu* child : nativeMenu->findChildren<QMenu*>(QString(), Qt::FindDirectChildrenOnly))
    {
        delete child;
    }
    const ShadDropdownMenuPrivate* menuPrivate = menu->d_func();
    for (const ShadDropdownMenuPrivate::Entry& entry : menuPrivate->_entries)
    {
        switch (entry.kind)
        {
            case EntryKind::Item:
            case EntryKind::CheckableItem:
            case EntryKind::RadioItem:
                {
                    nativeMenu->addAction(entry.action);
                    break;
                }
            case EntryKind::Submenu:
                {
                    if (entry.submenu.isNull())
                    {
                        break;
                    }
                    QMenu* nativeSubmenu = nativeMenu->addMenu(entry.action->text());
                    nativeSubmenu->menuAction()->setEnabled(entry.action->isEnabled());
                    nativeSubmenu->menuAction()->setVisible(entry.action->isVisible());
                    fillNativeMenu(nativeSubmenu, entry.submenu);
                    break;
                }
            case EntryKind::Separator:
                {
                    nativeMenu->addSeparator();
                    break;
                }
            case EntryKind::Label:
                {
                    nativeMenu->addSection(entry.labelText);
                    break;
                }
            case EntryKind::Widget:
                {
                    break;
                }
        }
    }
}
