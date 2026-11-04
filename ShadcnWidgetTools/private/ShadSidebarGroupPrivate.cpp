#include "ShadSidebarGroupPrivate.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include "ShadSidebarCollapseArea.h"
#include "ShadSidebarGroup.h"
#include "ShadStyleHelper.h"

ShadSidebarGroupPrivate::ShadSidebarGroupPrivate()
{
}

ShadSidebarGroupPrivate::~ShadSidebarGroupPrivate()
{
}

bool ShadSidebarGroupPrivate::isLabelVisible() const
{
    return !_label.isEmpty() && !_isIconCollapsed;
}

bool ShadSidebarGroupPrivate::isActionVisible() const
{
    return _actionIcon != ShadIconType::None && isLabelVisible();
}

QRect ShadSidebarGroupPrivate::logicalLabelRect() const
{
    Q_Q(const ShadSidebarGroup);
    return QRect(kPadding, kPadding, q->width() - 2 * kPadding, kLabelHeight);
}

QRect ShadSidebarGroupPrivate::logicalActionRect() const
{
    Q_Q(const ShadSidebarGroup);
    return QRect(q->width() - kActionEnd - kActionSize, kActionTop, kActionSize, kActionSize);
}

QRect ShadSidebarGroupPrivate::visualRect(const QRect& logicalRect) const
{
    Q_Q(const ShadSidebarGroup);
    return ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), logicalRect);
}

void ShadSidebarGroupPrivate::updateMargins()
{
    Q_Q(ShadSidebarGroup);
    // 标题行由分组自绘，布局上边距让出标题行的高度
    const int top = kPadding + (isLabelVisible() ? qRound(kLabelHeight * _labelProgress) : 0);
    _mainLayout->setContentsMargins(kPadding, top, kPadding, kPadding);
    q->setFocusPolicy(_isCollapsible && isLabelVisible() ? Qt::StrongFocus : Qt::NoFocus);
    q->update();
}

void ShadSidebarGroupPrivate::setProgress(qreal progress)
{
    Q_Q(ShadSidebarGroup);
    _progress = progress;
    // 裁剪容器按进度显示菜单区，菜单内容（例如子菜单）增减时容器高度自动跟随
    _menuClip->setProgress(progress);
    if (progress <= 0.0)
    {
        ShadStyleHelper::clearFocusWithin(_menuClip);
    }
    _menuClip->setVisible(progress > 0.0);
    q->update();
}

void ShadSidebarGroupPrivate::setIconCollapsed(bool collapsed)
{
    if (_isIconCollapsed == collapsed)
    {
        return;
    }
    _isIconCollapsed = collapsed;
    updateMargins();
}

void ShadSidebarGroupPrivate::setLabelProgress(qreal progress)
{
    if (qFuzzyCompare(_labelProgress, progress))
    {
        return;
    }
    _labelProgress = progress;
    updateMargins();
}
