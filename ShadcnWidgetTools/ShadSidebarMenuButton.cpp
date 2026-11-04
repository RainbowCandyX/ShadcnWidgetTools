#include "ShadSidebarMenuButton.h"

#include <QBoxLayout>
#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QFontMetricsF>
#include <QKeyEvent>
#include <QLayout>
#include <QList>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QSizeF>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>
#include <QtMath>

#include "ShadAvatar.h"
#include "ShadIcon.h"
#include "ShadSidebar.h"
#include "ShadSidebarMenuButtonPrivate.h"
#include "ShadSidebarMenuSub.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadSidebarMenuButton::ShadSidebarMenuButton(QWidget* parent)
    : QAbstractButton(parent), d_ptr(new ShadSidebarMenuButtonPrivate())
{
    Q_D(ShadSidebarMenuButton);
    d->q_ptr = this;
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    connect(this, &ShadSidebarMenuButton::clicked, this, [d]() { d->onClicked(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadSidebarMenuButton::ShadSidebarMenuButton(ShadIconType::IconName icon, const QString& text, QWidget* parent)
    : ShadSidebarMenuButton(parent)
{
    setShadIcon(icon);
    setText(text);
}

ShadSidebarMenuButton::~ShadSidebarMenuButton()
{
    Q_D(ShadSidebarMenuButton);
    // 私有数据先于基类释放，断开捕获 d 的连接
    QObject::disconnect(this, &ShadSidebarMenuButton::clicked, this, nullptr);
    // 子菜单与按钮同属菜单项容器，一并删除
    if (d->_menuItem != nullptr)
    {
        d->_menuItem->deleteLater();
    }
}

void ShadSidebarMenuButton::setShadIcon(ShadIconType::IconName icon)
{
    Q_D(ShadSidebarMenuButton);
    d->_icon = icon;
    updateGeometry();
    update();
}

ShadIconType::IconName ShadSidebarMenuButton::getShadIcon() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_icon;
}

void ShadSidebarMenuButton::setDescription(const QString& description)
{
    Q_D(ShadSidebarMenuButton);
    d->_description = description;
    updateGeometry();
    update();
}

QString ShadSidebarMenuButton::getDescription() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_description;
}

void ShadSidebarMenuButton::setButtonSize(ShadSidebarMenuButtonType::Size size)
{
    Q_D(ShadSidebarMenuButton);
    d->_size = size;
    d->updateAvatarGeometry();
    updateGeometry();
    update();
}

ShadSidebarMenuButtonType::Size ShadSidebarMenuButton::getButtonSize() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_size;
}

void ShadSidebarMenuButton::setVariant(ShadSidebarMenuButtonType::Variant variant)
{
    Q_D(ShadSidebarMenuButton);
    d->_variant = variant;
    update();
}

ShadSidebarMenuButtonType::Variant ShadSidebarMenuButton::getVariant() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_variant;
}

void ShadSidebarMenuButton::setActive(bool active)
{
    Q_D(ShadSidebarMenuButton);
    if (d->_isActive == active)
    {
        return;
    }
    d->_isActive = active;
    d->setMark(active, ShadSidebarMenuButtonPrivate::kMarkInset, ShadSidebarMenuButtonPrivate::kMarkInset);
    updateGeometry();
    update();
    Q_EMIT activeChanged(active);
}

bool ShadSidebarMenuButton::isActive() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_isActive;
}

void ShadSidebarMenuButton::setSelectable(bool selectable)
{
    Q_D(ShadSidebarMenuButton);
    d->_isSelectable = selectable;
}

bool ShadSidebarMenuButton::isSelectable() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_isSelectable;
}

void ShadSidebarMenuButton::setOpen(bool open)
{
    Q_D(ShadSidebarMenuButton);
    if (d->_isOpen == open)
    {
        return;
    }
    d->_isOpen = open;
    update();
}

bool ShadSidebarMenuButton::isOpen() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_isOpen;
}

void ShadSidebarMenuButton::setAvatar(ShadAvatar* avatar)
{
    Q_D(ShadSidebarMenuButton);
    if (d->_avatar.data() == avatar)
    {
        return;
    }
    delete d->_avatar.data();
    d->_avatar = avatar;
    if (avatar != nullptr)
    {
        avatar->setParent(this);
        // 鼠标事件交给按钮处理
        avatar->setAttribute(Qt::WA_TransparentForMouseEvents);
        d->updateAvatarGeometry();
    }
    updateGeometry();
    update();
}

ShadAvatar* ShadSidebarMenuButton::getAvatar() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_avatar.data();
}

void ShadSidebarMenuButton::setBadgeText(const QString& text)
{
    Q_D(ShadSidebarMenuButton);
    d->_badgeText = text;
    updateGeometry();
    update();
}

QString ShadSidebarMenuButton::getBadgeText() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_badgeText;
}

void ShadSidebarMenuButton::setActionIcon(ShadIconType::IconName icon)
{
    Q_D(ShadSidebarMenuButton);
    d->_actionIcon = icon;
    updateGeometry();
    update();
}

ShadIconType::IconName ShadSidebarMenuButton::getActionIcon() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_actionIcon;
}

void ShadSidebarMenuButton::setActionAlwaysVisible(bool alwaysVisible)
{
    Q_D(ShadSidebarMenuButton);
    d->_isActionAlwaysVisible = alwaysVisible;
    update();
}

bool ShadSidebarMenuButton::isActionAlwaysVisible() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_isActionAlwaysVisible;
}

QRect ShadSidebarMenuButton::getActionRect() const
{
    Q_D(const ShadSidebarMenuButton);
    if (d->_actionIcon == ShadIconType::None || d->_isIconCollapsed)
    {
        return QRect();
    }
    return d->visualRect(d->logicalActionRect());
}

ShadSidebarMenuButton* ShadSidebarMenuButton::addSubItem(const QString& text, ShadIconType::IconName icon)
{
    Q_D(ShadSidebarMenuButton);
    if (d->_subMenu == nullptr)
    {
        QWidget* container = parentWidget();
        QBoxLayout* layout = container != nullptr ? qobject_cast<QBoxLayout*>(container->layout()) : nullptr;
        if (layout == nullptr || layout->indexOf(this) < 0)
        {
            qWarning("ShadSidebarMenuButton::addSubItem：按钮不在父组件的 QBoxLayout 中，无法插入子菜单");
            return nullptr;
        }
        // 按钮与子菜单放进同一容器：并列在菜单布局中时，折叠结尾的间距会突然跳动
        QWidget* menuItem = new QWidget(container);
        menuItem->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        QVBoxLayout* menuItemLayout = new QVBoxLayout(menuItem);
        menuItemLayout->setContentsMargins(0, 0, 0, 0);
        menuItemLayout->setSpacing(0);
        delete layout->replaceWidget(this, menuItem);
        menuItemLayout->addWidget(this);
        d->_menuItem = menuItem;
        d->_subMenu = new ShadSidebarMenuSub(menuItem);
        menuItemLayout->addWidget(d->_subMenu);
        connect(d->_subMenu, &ShadSidebarMenuSub::progressChanged, this, [this]() { update(); });
        updateGeometry();
    }
    ShadSidebarMenuButton * item = new ShadSidebarMenuButton(icon, text, d->_subMenu->contentWidget());
    item->d_func()->_isSubItem = true;
    item->updateGeometry();
    d->_subMenu->itemLayout()->addWidget(item);
    update();
    return item;
}

bool ShadSidebarMenuButton::hasSubItems() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_subMenu != nullptr;
}

void ShadSidebarMenuButton::setSubMenuExpanded(bool expanded)
{
    Q_D(ShadSidebarMenuButton);
    if (d->_subMenu == nullptr || d->_subMenu->isExpanded() == expanded)
    {
        return;
    }
    if (d->_isIconCollapsed)
    {
        // 图标模式下只记录状态，侧栏展开后再显示
        d->_subMenu->setExpanded(expanded, false);
        ShadStyleHelper::clearFocusWithin(d->_subMenu);
        d->_subMenu->setVisible(false);
    }
    else
    {
        d->_subMenu->setExpanded(expanded, isVisible());
    }
    update();
    ShadSidebar* owner = d->sidebar();
    if (expanded && owner != nullptr && owner->isSubMenuExclusive())
    {
        const QList<ShadSidebarMenuButton*> buttons = owner->findChildren<ShadSidebarMenuButton*>();
        for (ShadSidebarMenuButton * button : buttons)
        {
            if (button != this && button->hasSubItems())
            {
                button->setSubMenuExpanded(false);
            }
        }
    }
    Q_EMIT subMenuExpandedChanged(expanded);
}

bool ShadSidebarMenuButton::isSubMenuExpanded() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_subMenu != nullptr && d->_subMenu->isExpanded();
}

bool ShadSidebarMenuButton::isSubItem() const
{
    Q_D(const ShadSidebarMenuButton);
    return d->_isSubItem;
}

QSize ShadSidebarMenuButton::sizeHint() const
{
    Q_D(const ShadSidebarMenuButton);
    using Private = ShadSidebarMenuButtonPrivate;
    if (d->_isIconCollapsed)
    {
        return QSize(Private::kCollapsedSize, Private::kCollapsedSize);
    }
    int width = 2 * Private::kPadding + d->endReserve();
    if (d->hasLeadingMedia())
    {
        width += (d->isBoxed() ? Private::kIconBoxSize : Private::kIconSize) + Private::kGap;
    }
    width += qMax(ShadStyleHelper::textWidth(d->titleFont(), text()), ShadStyleHelper::textWidth(d->descriptionFont(), d->_description));
    return QSize(width, d->buttonHeight());
}

QSize ShadSidebarMenuButton::minimumSizeHint() const
{
    Q_D(const ShadSidebarMenuButton);
    // 宽度不够时文字省略，最小宽度只保证图标可见
    return QSize(ShadSidebarMenuButtonPrivate::kCollapsedSize, d->buttonHeight());
}

bool ShadSidebarMenuButton::event(QEvent* event)
{
    Q_D(ShadSidebarMenuButton);
    switch (event->type())
    {
        case QEvent::LayoutDirectionChange:
            d->updateAvatarGeometry();
            updateGeometry();
            update();
            break;
        case QEvent::FontChange:
            updateGeometry();
            update();
            break;
        default:
            break;
    }
    return QAbstractButton::event(event);
}

bool ShadSidebarMenuButton::hitButton(const QPoint& pos) const
{
    Q_D(const ShadSidebarMenuButton);
    if (d->isActionVisible() && d->visualRect(d->logicalActionRect()).contains(pos))
    {
        return false;
    }
    return QAbstractButton::hitButton(pos);
}

void ShadSidebarMenuButton::enterEvent(QEnterEvent* event)
{
    Q_D(ShadSidebarMenuButton);
    d->_isHovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void ShadSidebarMenuButton::leaveEvent(QEvent* event)
{
    Q_D(ShadSidebarMenuButton);
    d->_isHovered = false;
    d->_isActionHovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void ShadSidebarMenuButton::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadSidebarMenuButton);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    update();
    QAbstractButton::focusInEvent(event);
}

void ShadSidebarMenuButton::focusOutEvent(QFocusEvent* event)
{
    update();
    QAbstractButton::focusOutEvent(event);
}

void ShadSidebarMenuButton::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadSidebarMenuButton);
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        d->_isFocusVisible = true;
        click();
        event->accept();
        return;
    }
    QAbstractButton::keyPressEvent(event);
}

void ShadSidebarMenuButton::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadSidebarMenuButton);
    if (event->button() == Qt::LeftButton && d->isActionVisible() && d->visualRect(d->logicalActionRect()).contains(event->position().toPoint()))
    {
        d->_isActionPressed = true;
        event->accept();
        return;
    }
    d->_isFocusVisible = false;
    QAbstractButton::mousePressEvent(event);
}

void ShadSidebarMenuButton::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadSidebarMenuButton);
    const bool isActionHovered = d->isActionVisible() && d->visualRect(d->logicalActionRect()).contains(event->position().toPoint());
    if (d->_isActionHovered != isActionHovered)
    {
        d->_isActionHovered = isActionHovered;
        update();
    }
    QAbstractButton::mouseMoveEvent(event);
}

void ShadSidebarMenuButton::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadSidebarMenuButton);
    if (d->_isActionPressed)
    {
        d->_isActionPressed = false;
        if (d->visualRect(d->logicalActionRect()).contains(event->position().toPoint()))
        {
            Q_EMIT actionClicked();
        }
        event->accept();
        return;
    }
    QAbstractButton::mouseReleaseEvent(event);
}

void ShadSidebarMenuButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadSidebarMenuButton);
    using namespace ShadStyleHelper;
    using Private = ShadSidebarMenuButtonPrivate;
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QRectF bodyRect(rect());
    constexpr qreal kHoverAlpha = 0.5;
    const bool isStrong = d->_isActive || d->_isOpen || isDown();
    const bool isHighlighted = isStrong || (d->_isHovered && isEnabled());
    const QColor accent = isStrong ? shadTheme->getColor(ShadThemeType::SidebarAccent) : withAlpha(shadTheme->getColor(ShadThemeType::SidebarAccent), kHoverAlpha);
    const QColor textColor = isHighlighted ? shadTheme->getColor(ShadThemeType::SidebarAccentForeground) : shadTheme->getColor(ShadThemeType::SidebarForeground);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    qreal opacity = isEnabled() ? 1.0 : kDisabledOpacity;
    if (d->_isSubItem)
    {
        if (const ShadSidebarMenuSub* subMenu = d->ownerSubMenu())
        {
            opacity *= subMenu->contentOpacity();
        }
    }
    painter.setOpacity(opacity);

    if (d->_variant == ShadSidebarMenuButtonType::Outline && !d->_isSubItem)
    {
        fillRoundedRect(&painter, bodyRect, radius, isHighlighted ? accent : shadTheme->getColor(ShadThemeType::Background));
        strokeRoundedRect(&painter, bodyRect, radius, isHighlighted ? accent : shadTheme->getColor(ShadThemeType::SidebarBorder));
    }
    else if (isHighlighted)
    {
        fillRoundedRect(&painter, bodyRect, radius, accent);
    }
    const qreal markHeight = bodyRect.height() - d->_markTop - d->_markBottom;
    if (d->_isMarkVisible && markHeight > 0.0)
    {
        const QRectF mark = visualRect(layoutDirection(), bodyRect, QRectF(0.0, d->_markTop, Private::kMarkWidth, markHeight));
        fillRoundedRect(&painter, mark, Private::kMarkWidth / 2.0, shadTheme->getColor(ShadThemeType::SidebarPrimary));
    }

    const bool isBoxed = d->isBoxed();
    // 头像是子组件，自己绘制，这里只留出位置
    const bool hasAvatar = isBoxed && !d->_avatar.isNull();
    const int height = d->buttonHeight();
    const auto drawIconBox = [&](const QRect& logicalRect)
    {
        const QRectF boxRect(d->visualRect(logicalRect));
        fillRoundedRect(&painter, boxRect, shadTheme->getRadius(ShadThemeType::Large), shadTheme->getColor(ShadThemeType::SidebarPrimary));
        const QRectF iconRect(boxRect.center() - QPointF(Private::kIconSize / 2.0, Private::kIconSize / 2.0), QSizeF(Private::kIconSize, Private::kIconSize));
        ShadIcon::paint(&painter, iconRect, d->_icon, shadTheme->getColor(ShadThemeType::SidebarPrimaryForeground), Private::kIconSize);
    };

    if (d->_isIconCollapsed)
    {
        if (isBoxed)
        {
            if (!hasAvatar && d->_icon != ShadIconType::None)
            {
                drawIconBox(QRect(0, 0, Private::kIconBoxSize, Private::kIconBoxSize));
            }
        }
        else if (d->_icon != ShadIconType::None)
        {
            const QRect iconRect((width() - Private::kIconSize) / 2, (height - Private::kIconSize) / 2, Private::kIconSize, Private::kIconSize);
            ShadIcon::paint(&painter, QRectF(iconRect), d->_icon, textColor, Private::kIconSize);
        }
    }
    else
    {
        int left = Private::kPadding;
        if (d->hasLeadingMedia())
        {
            if (isBoxed)
            {
                left = d->boxLeft();
                if (!hasAvatar)
                {
                    drawIconBox(QRect(left, (height - Private::kIconBoxSize) / 2, Private::kIconBoxSize, Private::kIconBoxSize));
                }
                left += Private::kIconBoxSize + Private::kGap;
            }
            else
            {
                const QRect iconRect(left, (height - Private::kIconSize) / 2, Private::kIconSize, Private::kIconSize);
                ShadIcon::paint(&painter, QRectF(d->visualRect(iconRect)), d->_icon, textColor, Private::kIconSize);
                left += Private::kIconSize + Private::kGap;
            }
        }
        const int right = width() - Private::kPadding - d->endReserve();
        const int textWidth = qMax(0, right - left);
        const Qt::Alignment alignment = visualAlignment(layoutDirection(), Qt::AlignLeft | Qt::AlignVCenter);
        const QFont titleFont = d->titleFont();
        painter.setPen(textColor);
        if (d->_description.isEmpty())
        {
            painter.setFont(titleFont);
            painter.drawText(d->visualRect(QRect(left, 0, textWidth, height)), alignment, elideText(titleFont, text(), textWidth));
        }
        else
        {
            const QFont descriptionFont = d->descriptionFont();
            const int titleHeight = qCeil(QFontMetricsF(titleFont).height());
            const int descriptionHeight = qCeil(QFontMetricsF(descriptionFont).height());
            const int top = (height - titleHeight - descriptionHeight) / 2;
            painter.setFont(titleFont);
            painter.drawText(d->visualRect(QRect(left, top, textWidth, titleHeight)), alignment, elideText(titleFont, text(), textWidth));
            painter.setFont(descriptionFont);
            painter.drawText(d->visualRect(QRect(left, top + titleHeight, textWidth, descriptionHeight)), alignment, elideText(descriptionFont, d->_description, textWidth));
        }

        if (d->isActionVisible())
        {
            const QRect actionRect = d->visualRect(d->logicalActionRect());
            if (d->_isActionHovered)
            {
                fillRoundedRect(&painter, QRectF(actionRect), radius, isHighlighted ? shadTheme->getColor(ShadThemeType::Background) : accent);
            }
            const QRectF iconRect(QPointF(actionRect.center()) - QPointF(Private::kIconSize / 2.0 - 0.5, Private::kIconSize / 2.0 - 0.5), QSizeF(Private::kIconSize, Private::kIconSize));
            ShadIcon::paint(&painter, iconRect, d->_actionIcon, textColor, Private::kIconSize);
        }
        else if (d->_actionIcon == ShadIconType::None && !d->_badgeText.isEmpty())
        {
            const QFont badgeFont = [&]()
            {
                QFont font = d->descriptionFont();
                font.setWeight(QFont::Medium);
                return font;
            }();
            const int badgeWidth = qMax(Private::kEndItemSize, ShadStyleHelper::textWidth(badgeFont, d->_badgeText) + 8);
            const QRect badgeRect(width() - Private::kEndInset - badgeWidth, (height - Private::kEndItemSize) / 2, badgeWidth, Private::kEndItemSize);
            painter.setFont(badgeFont);
            painter.drawText(d->visualRect(badgeRect), Qt::AlignCenter, d->_badgeText);
        }
        else if (d->_subMenu != nullptr)
        {
            const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
            const QRect chevronRect(width() - Private::kPadding - Private::kIconSize, (height - Private::kIconSize) / 2, Private::kIconSize, Private::kIconSize);
            const QRectF visualChevron(d->visualRect(chevronRect));
            const qreal angle = (isRightToLeft ? -90.0 : 90.0) * d->_subMenu->expandProgress();
            painter.save();
            painter.translate(visualChevron.center());
            painter.rotate(angle);
            painter.translate(-visualChevron.center());
            ShadIcon::paint(&painter, visualChevron, isRightToLeft ? ShadIconType::ChevronLeft : ShadIconType::ChevronRight, textColor, Private::kIconSize);
            painter.restore();
        }
    }

    if (hasFocus() && d->_isFocusVisible)
    {
        strokeRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(ShadThemeType::SidebarRing), Private::kFocusRingWidth);
    }
}

void ShadSidebarMenuButton::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadSidebarMenuButton);
    QAbstractButton::resizeEvent(event);
    // RTL 下头像位置随宽度变化
    d->updateAvatarGeometry();
}
