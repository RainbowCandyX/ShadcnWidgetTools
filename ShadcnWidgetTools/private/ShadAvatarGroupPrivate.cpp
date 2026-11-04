#include "ShadAvatarGroupPrivate.h"

#include <QList>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QWidget>

#include "ShadAvatar.h"
#include "ShadAvatarGroup.h"
#include "ShadAvatarGroupCount.h"
#include "ShadAvatarPrivate.h"
#include "ShadStyleHelper.h"

ShadAvatarGroupPrivate::ShadAvatarGroupPrivate()
{
}

ShadAvatarGroupPrivate::~ShadAvatarGroupPrivate()
{
}

void ShadAvatarGroupPrivate::attachAvatar(ShadAvatar* avatar)
{
    avatar->setAvatarSize(_size);
    avatar->d_func()->_isGroupRingVisible = true;
    avatar->update();
}

void ShadAvatarGroupPrivate::detachAvatar(ShadAvatar* avatar)
{
    avatar->d_func()->_isGroupRingVisible = false;
    avatar->update();
}

void ShadAvatarGroupPrivate::updateCountVisibility()
{
    _count->setVisible(!_count->getText().isEmpty() || _count->getIcon() != ShadIconType::None);
}

int ShadAvatarGroupPrivate::itemCount() const
{
    return int(_avatars.size()) + (_count->isVisibleTo(_count->parentWidget()) ? 1 : 0);
}

void ShadAvatarGroupPrivate::updateLayout()
{
    Q_Q(ShadAvatarGroup);
    QList<QWidget*> items;
    for (ShadAvatar* avatar : _avatars)
    {
        items.append(avatar);
    }
    if (_count->isVisibleTo(q))
    {
        items.append(_count);
    }
    const int diameter = ShadAvatarPrivate::diameter(_size);
    const int side = diameter + 2 * ShadAvatarPrivate::kRingWidth;
    const int step = diameter - kOverlap;
    int x = 0;
    for (QWidget* item : items)
    {
        const QRect logicalRect(QPoint(x, 0), QSize(side, side));
        item->setGeometry(ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), logicalRect));
        // 按顺序提升层级，后面的盖在前面的上方
        item->raise();
        x += step;
    }
}
