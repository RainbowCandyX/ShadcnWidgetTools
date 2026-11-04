#include "ShadSidebarGroup.h"

#include <QColor>
#include <QCursor>
#include <QEasingCurve>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSizeF>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QtGlobal>

#include "ShadFrameAnimation.h"
#include "ShadIcon.h"
#include "ShadSidebarCollapseArea.h"
#include "ShadSidebarGroupPrivate.h"
#include "ShadSidebarMenuButton.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // text-sidebar-foreground/70
    constexpr qreal kLabelAlpha = 0.7;
} // namespace

ShadSidebarGroup::ShadSidebarGroup(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadSidebarGroupPrivate())
{
    Q_D(ShadSidebarGroup);
    d->q_ptr = this;
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->_mainLayout = new QVBoxLayout(this);
    d->_mainLayout->setSpacing(0);
    d->_menuClip = new ShadSidebarCollapseArea(this);
    d->_menuArea = d->_menuClip->contentWidget();
    d->_menuLayout = new QVBoxLayout(d->_menuArea);
    d->_menuLayout->setContentsMargins(0, 0, 0, 0);
    d->_menuLayout->setSpacing(ShadSidebarGroupPrivate::kMenuGap);
    d->_mainLayout->addWidget(d->_menuClip);
    // 鼠标从标题行直接移入菜单区时分组收不到 Leave，在菜单区的 Enter 中清除标题行的悬停状态
    d->_menuArea->installEventFilter(this);
    d->_animation = new ShadFrameAnimation(this);
    d->_animation->setDuration(ShadSidebarGroupPrivate::kAnimationDurationMs);
    d->_animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_animation, &ShadFrameAnimation::valueChanged, this, [d](qreal value) { d->setProgress(value); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    d->updateMargins();
}

ShadSidebarGroup::ShadSidebarGroup(const QString& label, QWidget* parent)
    : ShadSidebarGroup(parent)
{
    setLabel(label);
}

ShadSidebarGroup::~ShadSidebarGroup()
{
    Q_D(ShadSidebarGroup);
    // 私有数据先于子对象释放，断开捕获 d 的连接并移除事件过滤器
    QObject::disconnect(d->_animation, nullptr, this, nullptr);
    d->_menuArea->removeEventFilter(this);
}

void ShadSidebarGroup::setLabel(const QString& label)
{
    Q_D(ShadSidebarGroup);
    d->_label = label;
    d->updateMargins();
}

QString ShadSidebarGroup::getLabel() const
{
    Q_D(const ShadSidebarGroup);
    return d->_label;
}

void ShadSidebarGroup::setCollapsible(bool collapsible)
{
    Q_D(ShadSidebarGroup);
    d->_isCollapsible = collapsible;
    if (!collapsible && !d->_isExpanded)
    {
        setExpanded(true);
    }
    d->updateMargins();
}

bool ShadSidebarGroup::isCollapsible() const
{
    Q_D(const ShadSidebarGroup);
    return d->_isCollapsible;
}

void ShadSidebarGroup::setExpanded(bool expanded)
{
    Q_D(ShadSidebarGroup);
    if (d->_isExpanded == expanded)
    {
        return;
    }
    d->_isExpanded = expanded;
    d->_animation->stop();
    if (isVisible())
    {
        d->_animation->setStartValue(d->_progress);
        d->_animation->setEndValue(expanded ? 1.0 : 0.0);
        d->_animation->start();
    }
    else
    {
        d->setProgress(expanded ? 1.0 : 0.0);
    }
    Q_EMIT expandedChanged(expanded);
}

bool ShadSidebarGroup::isExpanded() const
{
    Q_D(const ShadSidebarGroup);
    return d->_isExpanded;
}

void ShadSidebarGroup::setActionIcon(ShadIconType::IconName icon)
{
    Q_D(ShadSidebarGroup);
    d->_actionIcon = icon;
    update();
}

ShadIconType::IconName ShadSidebarGroup::getActionIcon() const
{
    Q_D(const ShadSidebarGroup);
    return d->_actionIcon;
}

QRect ShadSidebarGroup::getActionRect() const
{
    Q_D(const ShadSidebarGroup);
    return d->isActionVisible() ? d->visualRect(d->logicalActionRect()) : QRect();
}

ShadSidebarMenuButton* ShadSidebarGroup::addItem(ShadIconType::IconName icon, const QString& text)
{
    Q_D(ShadSidebarGroup);
    ShadSidebarMenuButton* button = new ShadSidebarMenuButton(icon, text, d->_menuArea);
    d->_menuLayout->addWidget(button);
    return button;
}

void ShadSidebarGroup::addWidget(QWidget* widget)
{
    Q_D(ShadSidebarGroup);
    if (widget == nullptr)
    {
        return;
    }
    widget->setParent(d->_menuArea);
    d->_menuLayout->addWidget(widget);
}

QVBoxLayout* ShadSidebarGroup::menuLayout() const
{
    Q_D(const ShadSidebarGroup);
    return d->_menuLayout;
}

bool ShadSidebarGroup::event(QEvent* event)
{
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        update();
    }
    return QWidget::event(event);
}

bool ShadSidebarGroup::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadSidebarGroup);
    if (watched == d->_menuArea && event->type() == QEvent::Enter && (d->_isLabelHovered || d->_isActionHovered))
    {
        d->_isLabelHovered = false;
        d->_isActionHovered = false;
        unsetCursor();
        update();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadSidebarGroup::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadSidebarGroup);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    update();
    QWidget::focusInEvent(event);
}

void ShadSidebarGroup::focusOutEvent(QFocusEvent* event)
{
    update();
    QWidget::focusOutEvent(event);
}

void ShadSidebarGroup::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadSidebarGroup);
    const bool isActivationKey = event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
    if (d->_isCollapsible && d->isLabelVisible() && isActivationKey)
    {
        d->_isFocusVisible = true;
        setExpanded(!d->_isExpanded);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadSidebarGroup::leaveEvent(QEvent* event)
{
    Q_D(ShadSidebarGroup);
    d->_isLabelHovered = false;
    d->_isActionHovered = false;
    update();
    QWidget::leaveEvent(event);
}

void ShadSidebarGroup::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadSidebarGroup);
    const QPoint pos = event->position().toPoint();
    const bool isActionHovered = d->isActionVisible() && d->visualRect(d->logicalActionRect()).contains(pos);
    const bool isLabelHovered = !isActionHovered && d->_isCollapsible && d->isLabelVisible() && d->visualRect(d->logicalLabelRect()).contains(pos);
    if (d->_isActionHovered != isActionHovered || d->_isLabelHovered != isLabelHovered)
    {
        d->_isActionHovered = isActionHovered;
        d->_isLabelHovered = isLabelHovered;
        setCursor((isActionHovered || isLabelHovered) ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ShadSidebarGroup::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadSidebarGroup);
    const QPoint pos = event->position().toPoint();
    if (event->button() == Qt::LeftButton && d->isActionVisible() && d->visualRect(d->logicalActionRect()).contains(pos))
    {
        d->_isActionPressed = true;
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton && d->_isCollapsible && d->isLabelVisible() && d->visualRect(d->logicalLabelRect()).contains(pos))
    {
        d->_isLabelPressed = true;
        d->_isFocusVisible = false;
        setFocus(Qt::MouseFocusReason);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadSidebarGroup::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadSidebarGroup);
    const QPoint pos = event->position().toPoint();
    if (d->_isActionPressed)
    {
        d->_isActionPressed = false;
        if (d->visualRect(d->logicalActionRect()).contains(pos))
        {
            Q_EMIT actionClicked();
        }
        event->accept();
        return;
    }
    if (d->_isLabelPressed)
    {
        d->_isLabelPressed = false;
        if (d->visualRect(d->logicalLabelRect()).contains(pos))
        {
            setExpanded(!d->_isExpanded);
        }
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadSidebarGroup::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadSidebarGroup);
    using namespace ShadStyleHelper;
    using Private = ShadSidebarGroupPrivate;
    if (!d->isLabelVisible())
    {
        return;
    }
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QColor accent = shadTheme->getColor(ShadThemeType::SidebarAccent);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setOpacity((isEnabled() ? 1.0 : kDisabledOpacity) * d->_labelProgress);
    painter.setClipRect(QRect(0, 0, width(), Private::kPadding + qRound(Private::kLabelHeight * d->_labelProgress)));

    const QRect labelRect = d->logicalLabelRect();
    if (d->_isLabelHovered)
    {
        fillRoundedRect(&painter, QRectF(d->visualRect(labelRect)), radius, accent);
    }
    const QColor labelColor = d->_isLabelHovered ? shadTheme->getColor(ShadThemeType::SidebarAccentForeground) : withAlpha(shadTheme->getColor(ShadThemeType::SidebarForeground), kLabelAlpha);
    QFont labelFont = font();
    labelFont.setPixelSize(12);
    labelFont.setWeight(QFont::Medium);
    int textRight = labelRect.right() + 1 - Private::kLabelPaddingX;
    if (d->_isCollapsible)
    {
        // 展开时箭头旋转 180°（group-data-[open]:rotate-180）
        const QRect chevronRect(textRight - Private::kIconSize, labelRect.top() + (Private::kLabelHeight - Private::kIconSize) / 2, Private::kIconSize, Private::kIconSize);
        const QRectF visualChevron(d->visualRect(chevronRect));
        painter.save();
        painter.translate(visualChevron.center());
        painter.rotate(180.0 * d->_progress);
        painter.translate(-visualChevron.center());
        ShadIcon::paint(&painter, visualChevron, ShadIconType::ChevronDown, labelColor, Private::kIconSize);
        painter.restore();
        textRight -= Private::kIconSize + Private::kLabelPaddingX;
    }
    if (d->isActionVisible())
    {
        textRight = qMin(textRight, d->logicalActionRect().left() - Private::kLabelPaddingX);
    }
    const QRect textRect(labelRect.left() + Private::kLabelPaddingX, labelRect.top(), qMax(0, textRight - labelRect.left() - Private::kLabelPaddingX), Private::kLabelHeight);
    painter.setFont(labelFont);
    painter.setPen(labelColor);
    painter.drawText(d->visualRect(textRect), visualAlignment(layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter), elideText(labelFont, d->_label, textRect.width()));

    if (d->isActionVisible())
    {
        const QRectF actionRect(d->visualRect(d->logicalActionRect()));
        if (d->_isActionHovered)
        {
            fillRoundedRect(&painter, actionRect, radius, accent);
        }
        const QColor actionColor = d->_isActionHovered ? shadTheme->getColor(ShadThemeType::SidebarAccentForeground) : shadTheme->getColor(ShadThemeType::SidebarForeground);
        const QRectF iconRect(actionRect.center() - QPointF(Private::kIconSize / 2.0, Private::kIconSize / 2.0), QSizeF(Private::kIconSize, Private::kIconSize));
        ShadIcon::paint(&painter, iconRect, d->_actionIcon, actionColor, Private::kIconSize);
    }

    // 键盘焦点：ring-2 ring-sidebar-ring，绘制在标题行内侧
    if (hasFocus() && d->_isFocusVisible)
    {
        strokeRoundedRect(&painter, QRectF(d->visualRect(labelRect)), radius, shadTheme->getColor(ShadThemeType::SidebarRing), 2.0);
    }
}
