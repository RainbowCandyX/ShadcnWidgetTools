#include "ShadItemGroupPrivate.h"

#include <QWidget>

#include "ShadItem.h"

ShadItemGroupPrivate::ShadItemGroupPrivate()
{
}

ShadItemGroupPrivate::~ShadItemGroupPrivate()
{
}

int ShadItemGroupPrivate::gap(ShadItemType::Size size)
{
    switch (size)
    {
        case ShadItemType::Small:
            return 10;
        case ShadItemType::ExtraSmall:
            return 8;
        case ShadItemType::Medium:
            break;
    }
    return 16;
}

ShadItemType::Size ShadItemGroupPrivate::smallestSize() const
{
    // has-data-[size=sm]:gap-2.5 has-data-[size=xs]:gap-2，同时存在时 xs 优先
    ShadItemType::Size result = ShadItemType::Medium;
    for (QWidget* entry : _entries)
    {
        const ShadItem* item = qobject_cast<const ShadItem*>(entry);
        if (item != nullptr && item->getSize() > result)
        {
            result = item->getSize();
        }
    }
    return result;
}
