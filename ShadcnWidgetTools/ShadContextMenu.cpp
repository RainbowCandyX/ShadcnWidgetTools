#include "ShadContextMenu.h"

#include <QAbstractScrollArea>
#include <QContextMenuEvent>
#include <QEvent>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QWidget>

#include "ShadContextMenuPrivate.h"
#include "ShadDropdownMenuPrivate.h"

ShadContextMenu::ShadContextMenu(QWidget* parent)
    : ShadDropdownMenu(parent), d_ptr(new ShadContextMenuPrivate())
{
    Q_D(ShadContextMenu);
    d->q_ptr = this;
    ShadDropdownMenu::d_ptr->setMenubarStyle(true);
    ShadDropdownMenu::d_ptr->_isContextMenu = true;
}

ShadContextMenu::~ShadContextMenu()
{
    Q_D(ShadContextMenu);
    const QList<QPointer<QWidget>> targets = d->_targets;
    for (const QPointer<QWidget>& target : targets)
    {
        d->removeTarget(target.data());
    }
}

void ShadContextMenu::attachTo(QWidget* target)
{
    Q_D(ShadContextMenu);
    if (target == nullptr || d->targetOf(target) == target)
    {
        return;
    }
    d->_targets.append(target);
    target->installEventFilter(this);
    if (QAbstractScrollArea* scrollArea = qobject_cast<QAbstractScrollArea*>(target))
    {
        scrollArea->viewport()->installEventFilter(this);
    }
}

void ShadContextMenu::detachFrom(QWidget* target)
{
    Q_D(ShadContextMenu);
    d->removeTarget(target);
}

bool ShadContextMenu::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadContextMenu);
    if (event->type() != QEvent::ContextMenu)
    {
        return ShadDropdownMenu::eventFilter(watched, event);
    }
    QWidget* target = d->targetOf(watched);
    if (target == nullptr || !target->isEnabled())
    {
        return ShadDropdownMenu::eventFilter(watched, event);
    }
    QContextMenuEvent* menuEvent = static_cast<QContextMenuEvent*>(event);
    const QPoint pos = watched == target ? menuEvent->pos() : target->mapFrom(static_cast<QWidget*>(watched), menuEvent->pos());
    Q_EMIT aboutToPopup(target, pos);
    popupAt(menuEvent->globalPos());
    event->accept();
    return true;
}
