#include "ShadAccordionPrivate.h"

#include <QApplication>
#include <QLayout>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadAccordion.h"
#include "ShadAccordionPanel.h"
#include "ShadAccordionTrigger.h"
#include "ShadFrameAnimation.h"
#include "ShadStyleHelper.h"

ShadAccordionPrivate::ShadAccordionPrivate()
{
}

ShadAccordionPrivate::~ShadAccordionPrivate()
{
}

ShadAccordionPrivate::Item* ShadAccordionPrivate::findItem(const QString& value)
{
    for (Item& item : _items)
    {
        if (item.value == value)
        {
            return &item;
        }
    }
    return nullptr;
}

const ShadAccordionPrivate::Item* ShadAccordionPrivate::findItem(const QString& value) const
{
    for (const Item& item : _items)
    {
        if (item.value == value)
        {
            return &item;
        }
    }
    return nullptr;
}

bool ShadAccordionPrivate::setExpanded(Item& item, bool expanded)
{
    if (item.isExpanded == expanded)
    {
        return false;
    }
    item.isExpanded = expanded;
    item.trigger->setExpanded(expanded);
    item.animation->stop();
    item.animation->setStartValue(item.panel->progress());
    item.animation->setEndValue(expanded ? 1.0 : 0.0);
    item.animation->setDuration(_duration);
    item.animation->start();
    return true;
}

void ShadAccordionPrivate::applyExpanded(const QStringList& values)
{
    Q_Q(ShadAccordion);
    QStringList targets;
    for (const QString& value : values)
    {
        if (findItem(value) != nullptr && !targets.contains(value))
        {
            targets.append(value);
        }
    }
    if (!_isMultiple && targets.size() > 1)
    {
        targets = {targets.first()};
    }
    bool isChanged = false;
    for (Item& item : _items)
    {
        isChanged = setExpanded(item, targets.contains(item.value)) || isChanged;
    }
    if (isChanged)
    {
        Q_EMIT q->expandedItemsChanged(q->getExpandedItems());
    }
}

void ShadAccordionPrivate::toggle(const QString& value)
{
    Q_Q(ShadAccordion);
    const Item* item = findItem(value);
    if (item == nullptr || !item->trigger->isEnabled())
    {
        return;
    }
    q->setItemExpanded(value, !item->isExpanded);
}

void ShadAccordionPrivate::updateItemMargins()
{
    const bool isBordered = _variant == ShadAccordionType::Bordered;
    // 焦点环画在标题行内，文字缩进其宽度
    const int horizontal = isBordered ? kBorderedPadding - ShadStyleHelper::kFocusRingWidth : 0;
    const int frame = isBordered ? kSeparatorWidth : 0;
    _layout->setContentsMargins(frame, frame, frame, frame);
    for (int index = 0; index < _items.size(); ++index)
    {
        const int bottom = index + 1 < _items.size() ? kSeparatorWidth : 0;
        _items.at(index).container->layout()->setContentsMargins(horizontal, 0, horizontal, bottom);
    }
}

void ShadAccordionPrivate::moveFocus(int key)
{
    QList<QWidget*> triggers;
    for (const Item& item : _items)
    {
        if (item.trigger->isEnabled())
        {
            triggers.append(item.trigger);
        }
    }
    const int count = static_cast<int>(triggers.size());
    const int index = static_cast<int>(triggers.indexOf(QApplication::focusWidget()));
    if (count == 0 || index < 0)
    {
        return;
    }
    int target = index;
    switch (key)
    {
        case Qt::Key_Up:
            target = (index - 1 + count) % count;
            break;
        case Qt::Key_Down:
            target = (index + 1) % count;
            break;
        case Qt::Key_Home:
            target = 0;
            break;
        case Qt::Key_End:
            target = count - 1;
            break;
        default:
            return;
    }
    triggers.at(target)->setFocus(Qt::TabFocusReason);
}
