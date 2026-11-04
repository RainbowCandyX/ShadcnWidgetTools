#include "ShadAvatarGroup.h"

#include <QChildEvent>
#include <QEvent>
#include <QObject>
#include <QResizeEvent>
#include <QSizePolicy>

#include "ShadAvatar.h"
#include "ShadAvatarGroupCount.h"
#include "ShadAvatarGroupPrivate.h"
#include "ShadAvatarPrivate.h"

ShadAvatarGroup::ShadAvatarGroup(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadAvatarGroupPrivate())
{
    Q_D(ShadAvatarGroup);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    d->_count = new ShadAvatarGroupCount(this);
    d->_count->setVisible(false);
}

ShadAvatarGroup::~ShadAvatarGroup()
{
}

int ShadAvatarGroup::addAvatar(ShadAvatar* avatar)
{
    Q_D(ShadAvatarGroup);
    return insertAvatar(int(d->_avatars.size()), avatar);
}

int ShadAvatarGroup::insertAvatar(int index, ShadAvatar* avatar)
{
    Q_D(ShadAvatarGroup);
    if (avatar == nullptr)
    {
        return -1;
    }
    d->_avatars.removeAll(avatar);
    if (index < 0 || index > d->_avatars.size())
    {
        index = int(d->_avatars.size());
    }
    d->_avatars.insert(index, avatar);
    if (avatar->parentWidget() != this)
    {
        avatar->setParent(this);
    }
    d->attachAvatar(avatar);
    avatar->show();
    d->updateLayout();
    updateGeometry();
    return index;
}

void ShadAvatarGroup::removeAvatar(int index)
{
    Q_D(ShadAvatarGroup);
    if (index < 0 || index >= d->_avatars.size())
    {
        return;
    }
    ShadAvatar* avatar = d->_avatars.takeAt(index);
    ShadAvatarGroupPrivate::detachAvatar(avatar);
    avatar->setParent(nullptr);
    d->updateLayout();
    updateGeometry();
}

int ShadAvatarGroup::count() const
{
    Q_D(const ShadAvatarGroup);
    return int(d->_avatars.size());
}

ShadAvatar* ShadAvatarGroup::avatar(int index) const
{
    Q_D(const ShadAvatarGroup);
    return index >= 0 && index < d->_avatars.size() ? d->_avatars.at(index) : nullptr;
}

void ShadAvatarGroup::setAvatarSize(ShadAvatarType::Size size)
{
    Q_D(ShadAvatarGroup);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    for (ShadAvatar* avatar : d->_avatars)
    {
        avatar->setAvatarSize(size);
    }
    d->_count->setAvatarSize(size);
    d->updateLayout();
    updateGeometry();
}

ShadAvatarType::Size ShadAvatarGroup::getAvatarSize() const
{
    Q_D(const ShadAvatarGroup);
    return d->_size;
}

void ShadAvatarGroup::setCountText(const QString& text)
{
    Q_D(ShadAvatarGroup);
    d->_count->setText(text);
    d->updateCountVisibility();
    d->updateLayout();
    updateGeometry();
}

QString ShadAvatarGroup::getCountText() const
{
    Q_D(const ShadAvatarGroup);
    return d->_count->getText();
}

void ShadAvatarGroup::setCountIcon(ShadIconType::IconName icon)
{
    Q_D(ShadAvatarGroup);
    d->_count->setIcon(icon);
    d->updateCountVisibility();
    d->updateLayout();
    updateGeometry();
}

ShadIconType::IconName ShadAvatarGroup::getCountIcon() const
{
    Q_D(const ShadAvatarGroup);
    return d->_count->getIcon();
}

void ShadAvatarGroup::clearCount()
{
    Q_D(ShadAvatarGroup);
    d->_count->setText(QString());
    d->_count->setIcon(ShadIconType::None);
    d->updateCountVisibility();
    d->updateLayout();
    updateGeometry();
}

bool ShadAvatarGroup::hasCount() const
{
    Q_D(const ShadAvatarGroup);
    return !d->_count->getText().isEmpty() || d->_count->getIcon() != ShadIconType::None;
}

QSize ShadAvatarGroup::sizeHint() const
{
    Q_D(const ShadAvatarGroup);
    const int diameter = ShadAvatarPrivate::diameter(d->_size);
    const int side = diameter + 2 * ShadAvatarPrivate::kRingWidth;
    const int itemCount = d->itemCount();
    if (itemCount == 0)
    {
        return QSize(0, side);
    }
    return QSize((itemCount - 1) * (diameter - ShadAvatarGroupPrivate::kOverlap) + side, side);
}

QSize ShadAvatarGroup::minimumSizeHint() const
{
    return sizeHint();
}

bool ShadAvatarGroup::event(QEvent* event)
{
    Q_D(ShadAvatarGroup);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->updateLayout();
    }
    return QWidget::event(event);
}

void ShadAvatarGroup::childEvent(QChildEvent* event)
{
    Q_D(ShadAvatarGroup);
    // 头像被删除或移到别处时从列表中移除；只按 QObject 指针比较，析构过程中的头像不会被访问
    if (event->type() == QEvent::ChildRemoved)
    {
        bool isRemoved = false;
        for (int index = int(d->_avatars.size()) - 1; index >= 0; --index)
        {
            if (static_cast<QObject*>(d->_avatars.at(index)) == event->child())
            {
                d->_avatars.removeAt(index);
                isRemoved = true;
            }
        }
        if (isRemoved)
        {
            d->updateLayout();
            updateGeometry();
        }
    }
    QWidget::childEvent(event);
}

void ShadAvatarGroup::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadAvatarGroup);
    QWidget::resizeEvent(event);
    d->updateLayout();
}
