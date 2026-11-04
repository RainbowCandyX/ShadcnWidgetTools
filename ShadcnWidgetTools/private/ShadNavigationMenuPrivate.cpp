#include "ShadNavigationMenuPrivate.h"

#include <QAction>
#include <QPoint>
#include <QRect>
#include <QTimer>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <utility>

#include "ShadNavigationMenu.h"
#include "ShadNavigationMenuTrigger.h"
#include "ShadNavigationMenuViewport.h"

ShadNavigationMenuPrivate::ShadNavigationMenuPrivate()
{
}

ShadNavigationMenuPrivate::~ShadNavigationMenuPrivate()
{
}

int ShadNavigationMenuPrivate::indexOfTrigger(const QObject* trigger) const
{
    for (int index = 0; index < int(_items.size()); ++index)
    {
        if (_items.at(index).trigger == trigger)
        {
            return index;
        }
    }
    return -1;
}

int ShadNavigationMenuPrivate::triggerAt(const QPoint& globalPos) const
{
    for (int index = 0; index < int(_items.size()); ++index)
    {
        const ShadNavigationMenuTrigger* trigger = _items.at(index).trigger;
        if (trigger->isVisible() && trigger->isEnabled() && trigger->rect().contains(trigger->mapFromGlobal(globalPos)))
        {
            return index;
        }
    }
    return -1;
}

bool ShadNavigationMenuPrivate::hasMenu(int index) const
{
    return index >= 0 && index < int(_items.size()) && !_items.at(index).content.isNull();
}

void ShadNavigationMenuPrivate::openMenu(int index, bool focusContent)
{
    Q_Q(ShadNavigationMenu);
    if (!hasMenu(index) || !_items.at(index).trigger->isEnabled())
    {
        return;
    }
    _openTimer->stop();
    _closeTimer->stop();
    _pendingIndex = -1;
    const bool isOpen = _viewport->isVisible() && _openIndex >= 0;
    const int step = isOpen ? index - _openIndex : 0;
    if (isOpen && _openIndex != index)
    {
        _items.at(_openIndex).trigger->setOpen(false);
    }
    _openIndex = index;
    _items.at(index).trigger->setOpen(true);
    _viewport->setLayoutDirection(q->layoutDirection());
    _viewport->showContent(_items.at(index).content.data(), step, barRect(), focusContent);
    _focusIndex = index;
    updateFocusPolicies();
    syncHover();
}

void ShadNavigationMenuPrivate::onHoverChanged(int index, bool hovered)
{
    if (index < 0)
    {
        return;
    }
    if (!hovered)
    {
        if (_pendingIndex == index)
        {
            _openTimer->stop();
            _pendingIndex = -1;
        }
        if (_suppressHoverIndex == index)
        {
            _suppressHoverIndex = -1;
        }
        return;
    }
    if (!hasMenu(index) || index == _suppressHoverIndex || _viewport->isVisible())
    {
        return;
    }
    if (_closedTimer.isValid() && _closedTimer.elapsed() < kSkipDelayMs)
    {
        openMenu(index, false);
        return;
    }
    _pendingIndex = index;
    _openTimer->start(kOpenDelayMs);
}

void ShadNavigationMenuPrivate::onTriggerClicked(int index)
{
    if (index < 0)
    {
        return;
    }
    if (!hasMenu(index))
    {
        if (_items.at(index).action != nullptr)
        {
            _items.at(index).action->trigger();
        }
        return;
    }
    if (_openIndex == index && _viewport->isVisible())
    {
        _suppressHoverIndex = index;
        _viewport->hide();
        return;
    }
    openMenu(index, false);
}

void ShadNavigationMenuPrivate::onViewportClosed()
{
    _closeTimer->stop();
    if (_openIndex < 0)
    {
        return;
    }
    const int index = _openIndex;
    _openIndex = -1;
    _items.at(index).trigger->setOpen(false);
    _closedTimer.start();
    syncHover();
    if (_isKeyboardClose)
    {
        focusTrigger(index, Qt::TabFocusReason);
    }
    _isKeyboardClose = false;
}

void ShadNavigationMenuPrivate::syncHover()
{
    for (const Item& item : std::as_const(_items))
    {
        item.trigger->syncHover();
    }
}

int ShadNavigationMenuPrivate::wrappedIndex(int index, int step) const
{
    const int count = int(_items.size());
    if (count == 0)
    {
        return -1;
    }
    // 跳过隐藏或禁用的菜单项
    int next = index;
    for (int attempt = 0; attempt < count; ++attempt)
    {
        next = ((next + step) % count + count) % count;
        const ShadNavigationMenuTrigger* trigger = _items.at(next).trigger;
        if (!trigger->isHidden() && trigger->isEnabled())
        {
            return next;
        }
    }
    return index;
}

void ShadNavigationMenuPrivate::focusTrigger(int index, Qt::FocusReason reason)
{
    if (index < 0 || index >= int(_items.size()))
    {
        return;
    }
    _focusIndex = index;
    updateFocusPolicies();
    _items.at(index).trigger->setFocus(reason);
}

void ShadNavigationMenuPrivate::updateFocusPolicies()
{
    for (int index = 0; index < int(_items.size()); ++index)
    {
        _items.at(index).trigger->setFocusPolicy(index == _focusIndex ? Qt::StrongFocus : Qt::ClickFocus);
    }
}

QRect ShadNavigationMenuPrivate::barRect() const
{
    Q_Q(const ShadNavigationMenu);
    return QRect(q->mapToGlobal(QPoint(0, 0)), q->size());
}
