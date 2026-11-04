#include "ShadToggleGroupPrivate.h"

#include "ShadStyleHelper.h"
#include "ShadToggle.h"
#include "ShadToggleGroup.h"

ShadToggleGroupPrivate::ShadToggleGroupPrivate()
{
}

ShadToggleGroupPrivate::~ShadToggleGroupPrivate()
{
}

QList<ShadToggle*> ShadToggleGroupPrivate::visibleItems() const
{
    QList<ShadToggle*> result;
    for (ShadToggle* item : _items)
    {
        if (!item->isHidden())
        {
            result.append(item);
        }
    }
    return result;
}

int ShadToggleGroupPrivate::itemOverlap() const
{
    return 2 * ShadStyleHelper::kFocusRingWidth + (_variant == ShadToggleType::Outline ? 1 : 0);
}

void ShadToggleGroupPrivate::onItemToggled(ShadToggle* item, bool checked)
{
    Q_Q(ShadToggleGroup);
    if (_isUpdating)
    {
        return;
    }
    if (_selectionMode == ShadToggleGroupType::Single && checked)
    {
        _isUpdating = true;
        for (ShadToggle* other : _items)
        {
            if (other != item && other->isChecked())
            {
                other->setChecked(false);
            }
        }
        _isUpdating = false;
    }
    const int currentIndex = q->getCurrentIndex();
    const bool isCurrentChanged = currentIndex != _lastCurrentIndex;
    _lastCurrentIndex = currentIndex;
    if (_selectionMode == ShadToggleGroupType::Single && isCurrentChanged)
    {
        Q_EMIT q->currentIndexChanged(currentIndex);
    }
    Q_EMIT q->checkedIndexesChanged();
}

void ShadToggleGroupPrivate::applyCheckedStates(const QList<bool>& states)
{
    Q_Q(ShadToggleGroup);
    bool isChanged = false;
    _isUpdating = true;
    for (int i = 0; i < _items.size() && i < states.size(); ++i)
    {
        if (_items.at(i)->isChecked() != states.at(i))
        {
            _items.at(i)->setChecked(states.at(i));
            isChanged = true;
        }
    }
    _isUpdating = false;
    if (!isChanged)
    {
        return;
    }
    const int currentIndex = q->getCurrentIndex();
    const bool isCurrentChanged = currentIndex != _lastCurrentIndex;
    _lastCurrentIndex = currentIndex;
    if (_selectionMode == ShadToggleGroupType::Single && isCurrentChanged)
    {
        Q_EMIT q->currentIndexChanged(currentIndex);
    }
    Q_EMIT q->checkedIndexesChanged();
}
