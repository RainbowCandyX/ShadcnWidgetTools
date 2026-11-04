#include "ShadSidebarMenuButtonPrivate.h"

#include <QLayout>
#include <QRect>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadAvatar.h"
#include "ShadSidebar.h"
#include "ShadSidebarMenuButton.h"
#include "ShadSidebarMenuSub.h"
#include "ShadSidebarPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTooltip.h"

ShadSidebarMenuButtonPrivate::ShadSidebarMenuButtonPrivate()
{
}

ShadSidebarMenuButtonPrivate::~ShadSidebarMenuButtonPrivate()
{
}

int ShadSidebarMenuButtonPrivate::buttonHeight() const
{
    if (_isIconCollapsed)
    {
        return kCollapsedSize;
    }
    if (_isSubItem)
    {
        return kSubItemHeight;
    }
    int height = 32;
    switch (_size)
    {
        case ShadSidebarMenuButtonType::Small:
            height = 28;
            break;
        case ShadSidebarMenuButtonType::Large:
            height = 48;
            break;
        case ShadSidebarMenuButtonType::Medium:
        default:
            break;
    }
    return kCollapsedSize + qRound((height - kCollapsedSize) * _iconProgress);
}

int ShadSidebarMenuButtonPrivate::boxLeft() const
{
    return _isIconCollapsed ? 0 : qRound(kPadding * _iconProgress);
}

void ShadSidebarMenuButtonPrivate::setIconProgress(qreal progress)
{
    Q_Q(ShadSidebarMenuButton);
    if (qFuzzyCompare(_iconProgress, progress))
    {
        return;
    }
    _iconProgress = progress;
    if (_subMenu != nullptr)
    {
        _subMenu->setIconProgress(progress);
    }
    updateAvatarGeometry();
    q->updateGeometry();
    q->update();
}

bool ShadSidebarMenuButtonPrivate::isBoxed() const
{
    return _size == ShadSidebarMenuButtonType::Large && !_isSubItem;
}

bool ShadSidebarMenuButtonPrivate::hasLeadingMedia() const
{
    return _icon != ShadIconType::None || (isBoxed() && !_avatar.isNull());
}

void ShadSidebarMenuButtonPrivate::updateAvatarGeometry()
{
    if (_avatar.isNull())
    {
        return;
    }
    if (!isBoxed())
    {
        _avatar->hide();
        return;
    }
    const int left = boxLeft();
    const int top = (buttonHeight() - kIconBoxSize) / 2;
    const QRect boxRect = visualRect(QRect(left, top, kIconBoxSize, kIconBoxSize));
    const int margin = qMax(0, (_avatar->sizeHint().width() - kIconBoxSize) / 2);
    _avatar->setGeometry(boxRect.adjusted(-margin, -margin, margin, margin));
    _avatar->show();
}

QFont ShadSidebarMenuButtonPrivate::titleFont() const
{
    Q_Q(const ShadSidebarMenuButton);
    QFont font = q->font();
    font.setPixelSize(_size == ShadSidebarMenuButtonType::Small && !_isSubItem ? 12 : 14);
    font.setWeight((_isActive || _size == ShadSidebarMenuButtonType::Large) ? QFont::Medium : QFont::Normal);
    return font;
}

QFont ShadSidebarMenuButtonPrivate::descriptionFont() const
{
    Q_Q(const ShadSidebarMenuButton);
    QFont font = q->font();
    font.setPixelSize(12);
    font.setWeight(QFont::Normal);
    return font;
}

bool ShadSidebarMenuButtonPrivate::isActionVisible() const
{
    Q_Q(const ShadSidebarMenuButton);
    if (_actionIcon == ShadIconType::None || _isIconCollapsed)
    {
        return false;
    }
    return _isActionAlwaysVisible || _isHovered || _isActive || q->hasFocus();
}

int ShadSidebarMenuButtonPrivate::endReserve() const
{
    if (_isIconCollapsed)
    {
        return 0;
    }
    if (_actionIcon != ShadIconType::None)
    {
        return kEndItemSize + kEndInset + kGap - kPadding;
    }
    if (!_badgeText.isEmpty())
    {
        const int badgeWidth = qMax(kEndItemSize, ShadStyleHelper::textWidth(descriptionFont(), _badgeText) + 8);
        return badgeWidth + kEndInset + kGap - kPadding;
    }
    if (_subMenu != nullptr)
    {
        return kIconSize + kGap;
    }
    return 0;
}

QRect ShadSidebarMenuButtonPrivate::logicalActionRect() const
{
    Q_Q(const ShadSidebarMenuButton);
    const int top = (buttonHeight() - kEndItemSize) / 2;
    return QRect(q->width() - kEndInset - kEndItemSize, top, kEndItemSize, kEndItemSize);
}

QRect ShadSidebarMenuButtonPrivate::visualRect(const QRect& logicalRect) const
{
    Q_Q(const ShadSidebarMenuButton);
    return ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), logicalRect);
}

ShadSidebar* ShadSidebarMenuButtonPrivate::sidebar() const
{
    Q_Q(const ShadSidebarMenuButton);
    for (QWidget* widget = q->parentWidget(); widget != nullptr; widget = widget->parentWidget())
    {
        if (ShadSidebar* sidebar = qobject_cast<ShadSidebar*>(widget))
        {
            return sidebar;
        }
    }
    return nullptr;
}

void ShadSidebarMenuButtonPrivate::onClicked()
{
    Q_Q(ShadSidebarMenuButton);
    if (_subMenu != nullptr)
    {
        q->setSubMenuExpanded(!_subMenu->isExpanded());
        return;
    }
    if (!_isSelectable)
    {
        return;
    }
    if (ShadSidebar* owner = sidebar())
    {
        ShadSidebarMenuButton* previous = owner->getActiveButton();
        owner->setActiveButton(q);
        owner->d_func()->startMarkTransition(previous, q);
        // 只有用户点击才播放动画并发出 itemActivated
        Q_EMIT owner->itemActivated(q);
        return;
    }
    q->setActive(true);
}

void ShadSidebarMenuButtonPrivate::setMark(bool visible, qreal topInset, qreal bottomInset)
{
    Q_Q(ShadSidebarMenuButton);
    _isMarkVisible = visible;
    _markTop = topInset;
    _markBottom = bottomInset;
    q->update();
}

ShadSidebarMenuSub* ShadSidebarMenuButtonPrivate::ownerSubMenu() const
{
    Q_Q(const ShadSidebarMenuButton);
    for (QWidget* widget = q->parentWidget(); widget != nullptr; widget = widget->parentWidget())
    {
        if (ShadSidebarMenuSub* subMenu = qobject_cast<ShadSidebarMenuSub*>(widget))
        {
            return subMenu;
        }
    }
    return nullptr;
}

void ShadSidebarMenuButtonPrivate::setIconCollapsed(bool collapsed, ShadPlacementType::Side tooltipSide)
{
    Q_Q(ShadSidebarMenuButton);
    // 侧栏位置变化时状态不变，提示方向与文字仍需更新
    ShadTooltip::setSide(q, tooltipSide);
    q->setToolTip(collapsed && !_isSubItem ? q->text() : QString());
    if (_isIconCollapsed == collapsed)
    {
        return;
    }
    _isIconCollapsed = collapsed;
    q->setMaximumWidth(collapsed ? kCollapsedSize : QWIDGETSIZE_MAX);
    // 收起时贴起始侧对齐，否则过渡期间图标会先跳到中间再滑回
    if (QWidget* container = q->parentWidget())
    {
        if (QLayout* layout = container->layout())
        {
            layout->setAlignment(q, collapsed ? Qt::AlignLeft : Qt::Alignment());
        }
    }
    if (_subMenu != nullptr)
    {
        // 子菜单在图标模式下隐藏，展开侧栏后恢复原来的展开状态
        const bool isSubMenuVisible = !collapsed && _subMenu->isExpanded();
        if (!isSubMenuVisible)
        {
            ShadStyleHelper::clearFocusWithin(_subMenu);
        }
        _subMenu->setVisible(isSubMenuVisible);
    }
    updateAvatarGeometry();
    q->updateGeometry();
    q->update();
}
