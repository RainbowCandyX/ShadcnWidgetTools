#include "ShadSidebarPrivate.h"

#include <QLayout>
#include <QList>
#include <QPoint>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadFrameAnimation.h"
#include "ShadScrollArea.h"
#include "ShadSidebar.h"
#include "ShadSidebarGroup.h"
#include "ShadSidebarGroupPrivate.h"
#include "ShadSidebarMenuButton.h"
#include "ShadSidebarMenuButtonPrivate.h"
#include "ShadSidebarRail.h"
#include "ShadStyleHelper.h"

ShadSidebarPrivate::ShadSidebarPrivate()
{
}

ShadSidebarPrivate::~ShadSidebarPrivate()
{
}

int ShadSidebarPrivate::variantPadding() const
{
    return _variant == ShadSidebarType::Sidebar ? 0 : kVariantPadding;
}

int ShadSidebarPrivate::boundedExpandedWidth(int width) const
{
    const int bounded = qMax(_minimumExpandedWidth, qMin(width, _maximumExpandedWidth));
    // 不小于 Floating / Inset 变体收起为图标后的宽度，否则收起反而变宽
    return qMax(kIconWidth + 2 * kVariantPadding, bounded);
}

int ShadSidebarPrivate::collapsedWidth() const
{
    switch (_collapsible)
    {
        case ShadSidebarType::Offcanvas:
            return 0;
        case ShadSidebarType::Icon:
            return kIconWidth + 2 * variantPadding();
        case ShadSidebarType::NotCollapsible:
        default:
            return _expandedWidth;
    }
}

void ShadSidebarPrivate::applyWidth()
{
    Q_Q(ShadSidebar);
    const int collapsed = collapsedWidth();
    const int width = collapsed + qRound((_expandedWidth - collapsed) * _progress);
    q->setFixedWidth(width);
    syncIconTransition();
    // 父布局要到下一轮事件循环才重排，动画期间边缘会闪烁，因此立即重排让主内容区同帧跟上
    if (QWidget* parent = q->parentWidget())
    {
        if (QLayout* parentLayout = parent->layout())
        {
            parentLayout->activate();
        }
    }
    layoutChildren();
}

QRect ShadSidebarPrivate::logicalPanelRect() const
{
    Q_Q(const ShadSidebar);
    const int padding = variantPadding();
    const int height = q->height() - 2 * padding;
    // Rail 会盖住内容区的滚动条，面板在 Rail 一侧让出其宽度；图标模式无需让出
    const bool isRailReserved = _variant == ShadSidebarType::Sidebar && _isRailVisible && _collapsible != ShadSidebarType::NotCollapsible && !_isIconApplied;
    // 收起为图标时随过渡减少
    const qreal reserveProgress = _collapsible == ShadSidebarType::Icon ? _progress : 1.0;
    const int railReserve = isRailReserved ? qRound(ShadSidebarRail::kRailWidth * reserveProgress) : 0;
    const int startOffset = _side == ShadSidebarType::End ? railReserve : 0;
    if (_collapsible == ShadSidebarType::Offcanvas)
    {
        // 面板保持展开宽度，随侧栏变窄向外滑出
        const int panelWidth = _expandedWidth - 2 * padding - railReserve;
        const int left = _side == ShadSidebarType::Start ? q->width() - _expandedWidth + padding : padding + startOffset;
        return QRect(left, padding, panelWidth, height);
    }
    return QRect(padding + startOffset, padding, q->width() - 2 * padding - railReserve, height);
}

QRect ShadSidebarPrivate::visualRect(const QRect& logicalRect) const
{
    Q_Q(const ShadSidebar);
    return ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), logicalRect);
}

void ShadSidebarPrivate::layoutChildren()
{
    Q_Q(ShadSidebar);
    _panel->setGeometry(visualRect(logicalPanelRect()));
    const int railWidth = ShadSidebarRail::kRailWidth;
    const bool isRailAtEnd = _side == ShadSidebarType::Start;
    const QRect railRect(isRailAtEnd ? q->width() - railWidth : 0, 0, railWidth, q->height());
    _rail->setGeometry(visualRect(railRect));
    _rail->setLineAtStart(!isRailAtEnd);
    _rail->setVisible(_isRailVisible && _collapsible != ShadSidebarType::NotCollapsible && q->width() >= railWidth);
    _rail->setDragEnabled(_isResizable && _isExpanded && qFuzzyCompare(_progress, 1.0));
    _rail->raise();
}

void ShadSidebarPrivate::refreshIconState()
{
    Q_Q(ShadSidebar);
    const bool isCollapsed = q->isIconCollapsed();
    _isIconApplied = isCollapsed;
    const ShadPlacementType::Side tooltipSide = _side == ShadSidebarType::Start ? ShadPlacementType::Right : ShadPlacementType::Left;
    const QList<ShadSidebarMenuButton*> buttons = q->findChildren<ShadSidebarMenuButton*>();
    for (ShadSidebarMenuButton* button : buttons)
    {
        button->d_func()->setIconCollapsed(isCollapsed, tooltipSide);
    }
    const QList<ShadSidebarGroup*> groups = q->findChildren<ShadSidebarGroup*>();
    for (ShadSidebarGroup* group : groups)
    {
        group->d_func()->setIconCollapsed(isCollapsed);
    }
    syncIconTransition();
}

void ShadSidebarPrivate::syncIconTransition()
{
    Q_Q(ShadSidebar);
    const qreal progress = _collapsible == ShadSidebarType::Icon ? _progress : 1.0;
    const QList<ShadSidebarGroup*> groups = q->findChildren<ShadSidebarGroup*>();
    for (ShadSidebarGroup* group : groups)
    {
        group->d_func()->setLabelProgress(progress);
    }
    const QList<ShadSidebarMenuButton*> buttons = q->findChildren<ShadSidebarMenuButton*>();
    for (ShadSidebarMenuButton* button : buttons)
    {
        button->d_func()->setIconProgress(progress);
    }
}

void ShadSidebarPrivate::updateSections()
{
    _header->setVisible(!_header->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly).isEmpty());
    _footer->setVisible(!_footer->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly).isEmpty());
}

void ShadSidebarPrivate::setProgress(qreal progress)
{
    Q_Q(ShadSidebar);
    _progress = progress;
    applyWidth();
    q->update();
}

void ShadSidebarPrivate::startMarkTransition(ShadSidebarMenuButton* from, ShadSidebarMenuButton* to)
{
    stopMarkTransition();
    if (_markDuration <= 0 || from == nullptr || to == nullptr || from == to || !from->isVisible() || !to->isVisible())
    {
        return;
    }
    constexpr qreal inset = ShadSidebarMenuButtonPrivate::kMarkInset;
    _markAnimation->setDuration(_markDuration / 2);
    _markFrom = from;
    _markTo = to;
    _isMarkDown = to->mapToGlobal(QPoint(0, 0)).y() > from->mapToGlobal(QPoint(0, 0)).y();
    _isMarkSecondPhase = false;
    from->d_func()->setMark(true, inset, inset);
    to->d_func()->setMark(false, inset, inset);
    _markAnimation->start();
}

void ShadSidebarPrivate::stopMarkTransition()
{
    constexpr qreal inset = ShadSidebarMenuButtonPrivate::kMarkInset;
    _markAnimation->stop();
    for (ShadSidebarMenuButton* button : {_markFrom.data(), _markTo.data()})
    {
        if (button != nullptr)
        {
            button->d_func()->setMark(button->isActive(), inset, inset);
        }
    }
    _markFrom = nullptr;
    _markTo = nullptr;
}

void ShadSidebarPrivate::applyMark(qreal value)
{
    constexpr qreal inset = ShadSidebarMenuButtonPrivate::kMarkInset;
    if (!_isMarkSecondPhase)
    {
        if (!_markFrom.isNull())
        {
            _markFrom->d_func()->setMark(true, _isMarkDown ? inset : inset * (1.0 - value), _isMarkDown ? inset * (1.0 - value) : inset);
        }
        return;
    }
    if (!_markTo.isNull())
    {
        _markTo->d_func()->setMark(true, _isMarkDown ? inset * value : inset, _isMarkDown ? inset : inset * value);
    }
}

void ShadSidebarPrivate::finishMarkPhase()
{
    constexpr qreal inset = ShadSidebarMenuButtonPrivate::kMarkInset;
    if (_isMarkSecondPhase)
    {
        stopMarkTransition();
        return;
    }
    if (!_markFrom.isNull())
    {
        _markFrom->d_func()->setMark(_markFrom->isActive(), inset, inset);
    }
    if (_markTo.isNull() || !_markTo->isActive())
    {
        stopMarkTransition();
        return;
    }
    _isMarkSecondPhase = true;
    applyMark(0.0);
    _markAnimation->start();
}
