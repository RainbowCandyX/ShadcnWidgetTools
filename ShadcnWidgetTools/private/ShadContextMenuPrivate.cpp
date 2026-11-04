#include "ShadContextMenuPrivate.h"

#include <QAbstractScrollArea>
#include <QObject>
#include <QWidget>

#include "ShadContextMenu.h"

ShadContextMenuPrivate::ShadContextMenuPrivate()
{
}

ShadContextMenuPrivate::~ShadContextMenuPrivate()
{
}

QWidget* ShadContextMenuPrivate::targetOf(QObject* watched) const
{
    for (const QPointer<QWidget>& target : _targets)
    {
        if (target.isNull())
        {
            continue;
        }
        if (target.data() == watched)
        {
            return target.data();
        }
        const QAbstractScrollArea* scrollArea = qobject_cast<const QAbstractScrollArea*>(target.data());
        if (scrollArea != nullptr && scrollArea->viewport() == watched)
        {
            return target.data();
        }
    }
    return nullptr;
}

void ShadContextMenuPrivate::removeTarget(QWidget* target)
{
    Q_Q(ShadContextMenu);
    for (int index = int(_targets.size()) - 1; index >= 0; --index)
    {
        if (_targets.at(index).isNull() || _targets.at(index).data() == target)
        {
            _targets.removeAt(index);
        }
    }
    if (target == nullptr)
    {
        return;
    }
    target->removeEventFilter(q);
    if (QAbstractScrollArea* scrollArea = qobject_cast<QAbstractScrollArea*>(target))
    {
        scrollArea->viewport()->removeEventFilter(q);
    }
}
