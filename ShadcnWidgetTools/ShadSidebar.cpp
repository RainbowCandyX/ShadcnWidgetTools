#include "ShadSidebar.h"

#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QFont>
#include <QList>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadFrameAnimation.h"
#include "ShadScrollArea.h"
#include "ShadSidebarGroup.h"
#include "ShadSidebarMenuButton.h"
#include "ShadSidebarPrivate.h"
#include "ShadSidebarRail.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadSidebar::ShadSidebar(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadSidebarPrivate())
{
    Q_D(ShadSidebar);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);

    d->_panel = new QWidget(this);
    QVBoxLayout* panelLayout = new QVBoxLayout(d->_panel);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0);
    using Private = ShadSidebarPrivate;
    d->_header = new QWidget(d->_panel);
    d->_headerLayout = new QVBoxLayout(d->_header);
    d->_headerLayout->setContentsMargins(Private::kSectionPadding, Private::kSectionPadding, Private::kSectionPadding, Private::kSectionPadding);
    d->_headerLayout->setSpacing(Private::kSectionGap);
    d->_scrollArea = new ShadScrollArea(d->_panel);
    QWidget* content = new QWidget(d->_scrollArea);
    d->_contentLayout = new QVBoxLayout(content);
    d->_contentLayout->setContentsMargins(0, 0, 0, 0);
    d->_contentLayout->setSpacing(Private::kSectionGap);
    d->_contentLayout->setAlignment(Qt::AlignTop);
    d->_scrollArea->setWidget(content);
    d->_footer = new QWidget(d->_panel);
    d->_footerLayout = new QVBoxLayout(d->_footer);
    d->_footerLayout->setContentsMargins(Private::kSectionPadding, Private::kSectionPadding, Private::kSectionPadding, Private::kSectionPadding);
    d->_footerLayout->setSpacing(Private::kSectionGap);
    panelLayout->addWidget(d->_header);
    panelLayout->addWidget(d->_scrollArea, 1);
    panelLayout->addWidget(d->_footer);
    d->_header->installEventFilter(this);
    d->_footer->installEventFilter(this);
    d->_panel->installEventFilter(this);
    // 内容区在滚动区中，变化不会传到面板，单独过滤
    content->installEventFilter(this);

    d->_rail = new ShadSidebarRail(this);
    connect(d->_rail, &ShadSidebarRail::clicked, this, [this]() { toggle(); });
    connect(d->_rail, &ShadSidebarRail::dragStarted, this, [d]() { d->_dragStartWidth = d->_expandedWidth; });
    connect(d->_rail, &ShadSidebarRail::dragMoved, this, [this, d](int deltaX)
    {
        const bool isOnLeft = (d->_side == ShadSidebarType::Start) == (layoutDirection() == Qt::LeftToRight);
        setExpandedWidth(d->_dragStartWidth + (isOnLeft ? deltaX : -deltaX));
    });

    d->_animation = new ShadFrameAnimation(this);
    d->_animation->setDuration(Private::kAnimationDurationMs);
    QEasingCurve easingCurve(QEasingCurve::BezierSpline);
    easingCurve.addCubicBezierSegment(QPointF(0.4, 0.0), QPointF(0.2, 1.0), QPointF(1.0, 1.0));
    d->_animation->setEasingCurve(easingCurve);
    connect(d->_animation, &ShadFrameAnimation::valueChanged, this, [d](qreal value) { d->setProgress(value); });
    connect(d->_animation, &ShadFrameAnimation::finished, this, [d]()
    {
        d->refreshIconState();
        d->layoutChildren();
    });
    d->_markAnimation = new ShadFrameAnimation(this);
    d->_markAnimation->setEasingCurve(QEasingCurve::InOutSine);
    connect(d->_markAnimation, &ShadFrameAnimation::valueChanged, this, [d](qreal value) { d->applyMark(value); });
    connect(d->_markAnimation, &ShadFrameAnimation::finished, this, [d]() { d->finishMarkPhase(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });

    d->updateSections();
    d->applyWidth();
}

ShadSidebar::~ShadSidebar()
{
    Q_D(ShadSidebar);
    // 私有数据先于子对象释放，断开捕获 d 的连接并移除事件过滤器
    QObject::disconnect(d->_animation, nullptr, this, nullptr);
    QObject::disconnect(d->_markAnimation, nullptr, this, nullptr);
    d->_header->removeEventFilter(this);
    d->_footer->removeEventFilter(this);
    d->_panel->removeEventFilter(this);
    if (QWidget* content = d->_scrollArea->widget())
    {
        content->removeEventFilter(this);
    }
}

void ShadSidebar::setSide(ShadSidebarType::Side side)
{
    Q_D(ShadSidebar);
    if (d->_side == side)
    {
        return;
    }
    d->_side = side;
    d->layoutChildren();
    d->refreshIconState();
    update();
    Q_EMIT sideChanged();
}

ShadSidebarType::Side ShadSidebar::getSide() const
{
    Q_D(const ShadSidebar);
    return d->_side;
}

void ShadSidebar::setVariant(ShadSidebarType::Variant variant)
{
    Q_D(ShadSidebar);
    if (d->_variant == variant)
    {
        return;
    }
    d->_variant = variant;
    d->applyWidth();
    update();
    Q_EMIT variantChanged();
}

ShadSidebarType::Variant ShadSidebar::getVariant() const
{
    Q_D(const ShadSidebar);
    return d->_variant;
}

void ShadSidebar::setCollapsible(ShadSidebarType::Collapsible collapsible)
{
    Q_D(ShadSidebar);
    if (d->_collapsible == collapsible)
    {
        return;
    }
    d->_collapsible = collapsible;
    if (collapsible == ShadSidebarType::NotCollapsible && !d->_isExpanded)
    {
        d->_isExpanded = true;
        d->_animation->stop();
        d->_progress = 1.0;
        Q_EMIT expandedChanged(true);
    }
    d->refreshIconState();
    d->applyWidth();
    update();
    Q_EMIT collapsibleChanged();
}

ShadSidebarType::Collapsible ShadSidebar::getCollapsible() const
{
    Q_D(const ShadSidebar);
    return d->_collapsible;
}

void ShadSidebar::setExpanded(bool expanded)
{
    Q_D(ShadSidebar);
    const bool value = expanded || d->_collapsible == ShadSidebarType::NotCollapsible;
    if (d->_isExpanded == value)
    {
        return;
    }
    d->_isExpanded = value;
    d->_animation->stop();
    if (isVisible())
    {
        // 收起时等动画结束再切换为图标模式
        if (value)
        {
            d->refreshIconState();
        }
        d->_animation->setStartValue(d->_progress);
        d->_animation->setEndValue(value ? 1.0 : 0.0);
        d->_animation->start();
    }
    else
    {
        d->refreshIconState();
        d->setProgress(value ? 1.0 : 0.0);
    }
    Q_EMIT expandedChanged(value);
}

bool ShadSidebar::isExpanded() const
{
    Q_D(const ShadSidebar);
    return d->_isExpanded;
}

void ShadSidebar::toggle()
{
    setExpanded(!isExpanded());
}

bool ShadSidebar::isIconCollapsed() const
{
    Q_D(const ShadSidebar);
    return d->_collapsible == ShadSidebarType::Icon && !d->_isExpanded;
}

void ShadSidebar::setExpandedWidth(int width)
{
    Q_D(ShadSidebar);
    const int bounded = d->boundedExpandedWidth(width);
    if (d->_expandedWidth == bounded)
    {
        return;
    }
    d->_expandedWidth = bounded;
    d->applyWidth();
    Q_EMIT expandedWidthChanged(bounded);
}

int ShadSidebar::getExpandedWidth() const
{
    Q_D(const ShadSidebar);
    return d->_expandedWidth;
}

void ShadSidebar::setExpandedWidthRange(int minimum, int maximum)
{
    Q_D(ShadSidebar);
    d->_minimumExpandedWidth = minimum;
    d->_maximumExpandedWidth = qMax(minimum, maximum);
    setExpandedWidth(d->_expandedWidth);
}

int ShadSidebar::getMinimumExpandedWidth() const
{
    Q_D(const ShadSidebar);
    return d->_minimumExpandedWidth;
}

int ShadSidebar::getMaximumExpandedWidth() const
{
    Q_D(const ShadSidebar);
    return d->_maximumExpandedWidth;
}

void ShadSidebar::setResizable(bool resizable)
{
    Q_D(ShadSidebar);
    d->_isResizable = resizable;
    d->layoutChildren();
}

bool ShadSidebar::isResizable() const
{
    Q_D(const ShadSidebar);
    return d->_isResizable;
}

void ShadSidebar::setSubMenuExclusive(bool exclusive)
{
    Q_D(ShadSidebar);
    if (d->_isSubMenuExclusive == exclusive)
    {
        return;
    }
    d->_isSubMenuExclusive = exclusive;
    if (!exclusive)
    {
        return;
    }
    ShadSidebarMenuButton* keep = nullptr;
    const QList<ShadSidebarMenuButton*> buttons = findChildren<ShadSidebarMenuButton*>();
    for (ShadSidebarMenuButton* button : buttons)
    {
        if (!button->isSubMenuExpanded())
        {
            continue;
        }
        if (keep == nullptr)
        {
            keep = button;
        }
        // 子菜单项与按钮同在菜单项容器中，容器是按钮的父组件
        QWidget* menuItem = button->parentWidget();
        if (!d->_activeButton.isNull() && menuItem != nullptr && menuItem->isAncestorOf(d->_activeButton.data()))
        {
            keep = button;
            break;
        }
    }
    for (ShadSidebarMenuButton* button : buttons)
    {
        if (button != keep && button->hasSubItems())
        {
            button->setSubMenuExpanded(false);
        }
    }
}

bool ShadSidebar::isSubMenuExclusive() const
{
    Q_D(const ShadSidebar);
    return d->_isSubMenuExclusive;
}

void ShadSidebar::setIndicatorDuration(int msecs)
{
    Q_D(ShadSidebar);
    d->_markDuration = qBound(0, msecs, ShadSidebarPrivate::kMaxMarkDurationMs);
}

int ShadSidebar::getIndicatorDuration() const
{
    Q_D(const ShadSidebar);
    return d->_markDuration;
}

void ShadSidebar::setRailVisible(bool visible)
{
    Q_D(ShadSidebar);
    d->_isRailVisible = visible;
    d->layoutChildren();
}

bool ShadSidebar::isRailVisible() const
{
    Q_D(const ShadSidebar);
    return d->_isRailVisible;
}

QVBoxLayout* ShadSidebar::headerLayout() const
{
    Q_D(const ShadSidebar);
    return d->_headerLayout;
}

QVBoxLayout* ShadSidebar::footerLayout() const
{
    Q_D(const ShadSidebar);
    return d->_footerLayout;
}

QVBoxLayout* ShadSidebar::contentLayout() const
{
    Q_D(const ShadSidebar);
    return d->_contentLayout;
}

ShadSidebarGroup* ShadSidebar::addGroup(const QString& label)
{
    Q_D(ShadSidebar);
    ShadSidebarGroup* group = new ShadSidebarGroup(label, d->_scrollArea->widget());
    d->_contentLayout->addWidget(group);
    return group;
}

void ShadSidebar::setActiveButton(ShadSidebarMenuButton* button)
{
    Q_D(ShadSidebar);
    d->stopMarkTransition();
    const QList<ShadSidebarMenuButton*> buttons = findChildren<ShadSidebarMenuButton*>();
    for (ShadSidebarMenuButton* item : buttons)
    {
        item->setActive(item == button);
    }
    d->_activeButton = button;
}

ShadSidebarMenuButton* ShadSidebar::getActiveButton() const
{
    Q_D(const ShadSidebar);
    return d->_activeButton.data();
}

QSize ShadSidebar::sizeHint() const
{
    return QSize(width(), QWidget::sizeHint().height());
}

QSize ShadSidebar::minimumSizeHint() const
{
    return QSize(width(), 0);
}

bool ShadSidebar::event(QEvent* event)
{
    Q_D(ShadSidebar);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->layoutChildren();
        update();
    }
    return QWidget::event(event);
}

bool ShadSidebar::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadSidebar);
    const bool isChildChange = event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildRemoved;
    if ((watched == d->_header || watched == d->_footer) && isChildChange)
    {
        // 隐藏的页头不会收到 LayoutRequest，按子组件的增减判断是否有内容
        d->updateSections();
    }
    else if (event->type() == QEvent::LayoutRequest)
    {
        const bool isPanelOrContent = watched == d->_panel || watched == d->_scrollArea->widget();
        if (isPanelOrContent && isIconCollapsed() && d->_isIconApplied)
        {
            d->refreshIconState();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadSidebar::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadSidebar);
    QWidget::resizeEvent(event);
    d->layoutChildren();
}

void ShadSidebar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadSidebar);
    using namespace ShadStyleHelper;
    if (width() <= 0)
    {
        return;
    }
    const QColor background = shadTheme->getColor(ShadThemeType::Sidebar);
    const QColor border = shadTheme->getColor(ShadThemeType::SidebarBorder);
    const QRect panelRect = d->visualRect(d->logicalPanelRect());
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    switch (d->_variant)
    {
        case ShadSidebarType::Floating:
            {
                const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
                drawShadowSm(&painter, QRectF(panelRect), radius);
                fillRoundedRect(&painter, QRectF(panelRect), radius, background);
                strokeRoundedRect(&painter, QRectF(panelRect), radius, border);
                break;
            }
        case ShadSidebarType::Inset:
            {
                // 外层由 ShadSidebarLayout 铺满背景，单独使用时同样铺满
                painter.fillRect(rect(), windowSurfaceColor(this, ShadThemeType::Sidebar, 0.0));
                break;
            }
        case ShadSidebarType::Sidebar:
        default:
            {
                painter.fillRect(rect(), windowSurfaceColor(this, ShadThemeType::Sidebar, kSidebarEffectOpacity));
                const int borderWidth = ShadSidebarPrivate::kBorderWidth;
                const QRect borderRect(d->_side == ShadSidebarType::Start ? width() - borderWidth : 0, 0, borderWidth, height());
                painter.fillRect(d->visualRect(borderRect), border);
                break;
            }
    }
}
