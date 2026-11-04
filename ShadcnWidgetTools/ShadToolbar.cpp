#include "ShadToolbar.h"

#include <QAction>
#include <QEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QResizeEvent>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadToolbarButton.h"
#include "ShadToolbarPrivate.h"

namespace
{
    ShadIconType::IconName overflowIcon(Qt::Orientation orientation)
    {
        return orientation == Qt::Horizontal ? ShadIconType::Ellipsis : ShadIconType::EllipsisVertical;
    }
} // namespace

ShadToolbar::ShadToolbar(QWidget* parent)
    : ShadToolbar(Qt::Horizontal, parent)
{
}

ShadToolbar::ShadToolbar(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent), d_ptr(new ShadToolbarPrivate())
{
    Q_D(ShadToolbar);
    d->q_ptr = this;
    d->_orientation = orientation;
    d->_overflowAction = new QAction(tr("更多"), this);
    d->_overflowMenu = new ShadDropdownMenu(this);
    d->_overflowButton = new ShadToolbarButton(d->_overflowAction, overflowIcon(orientation), d->_overflowMenu, this);
    d->_overflowButton->setChevronVisible(false);
    d->_overflowButton->setMenuSide(orientation == Qt::Horizontal ? ShadPlacementType::Bottom : ShadPlacementType::Right);
    d->_overflowButton->hide();
    connect(d->_overflowButton, &ShadToolbarButton::aboutToShowMenu, this, [d]() { d->rebuildOverflowMenu(); });
    setSizePolicy(orientation == Qt::Horizontal ? QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed) : QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadToolbar::~ShadToolbar()
{
    Q_D(ShadToolbar);
    d->disconnectItems();
}

QAction* ShadToolbar::addAction(ShadIconType::IconName icon, const QString& text)
{
    Q_D(ShadToolbar);
    QAction* action = new QAction(text, this);
    d->addActionItem(action, icon, true);
    return action;
}

void ShadToolbar::addAction(QAction* action, ShadIconType::IconName icon)
{
    Q_D(ShadToolbar);
    if (action == nullptr || d->indexOfAction(action) >= 0)
    {
        return;
    }
    d->addActionItem(action, icon, false);
}

ShadDropdownMenu* ShadToolbar::addMenu(ShadIconType::IconName icon, const QString& text)
{
    Q_D(ShadToolbar);
    ShadToolbarPrivate::Item item;
    item.kind = ShadToolbarPrivate::ItemKind::Menu;
    item.action = new QAction(text, this);
    item.isOwnedAction = true;
    item.menu = new ShadDropdownMenu(this);
    item.button = d->createButton(item.action, icon, item.menu);
    d->appendItem(item);
    return item.menu;
}

void ShadToolbar::setMenuTitle(ShadDropdownMenu* menu, const QString& title)
{
    Q_D(ShadToolbar);
    for (const ShadToolbarPrivate::Item& item : std::as_const(d->_items))
    {
        if (item.kind == ShadToolbarPrivate::ItemKind::Menu && item.menu == menu && !item.action.isNull())
        {
            item.action->setText(title);
            return;
        }
    }
}

void ShadToolbar::addWidget(QWidget* widget)
{
    Q_D(ShadToolbar);
    if (widget == nullptr)
    {
        return;
    }
    for (const ShadToolbarPrivate::Item& item : std::as_const(d->_items))
    {
        if (item.widget == widget)
        {
            return;
        }
    }
    widget->setParent(this);
    connect(widget, &QObject::destroyed, this, [d]() { d->removeDeadItems(); });
    ShadToolbarPrivate::Item item;
    item.kind = ShadToolbarPrivate::ItemKind::Widget;
    item.widget = widget;
    d->appendItem(item);
}

void ShadToolbar::addSeparator()
{
    Q_D(ShadToolbar);
    ShadToolbarPrivate::Item item;
    item.kind = ShadToolbarPrivate::ItemKind::Separator;
    d->appendItem(item);
}

void ShadToolbar::addStretch()
{
    Q_D(ShadToolbar);
    ShadToolbarPrivate::Item item;
    item.kind = ShadToolbarPrivate::ItemKind::Stretch;
    d->appendItem(item);
}

void ShadToolbar::removeAction(QAction* action)
{
    Q_D(ShadToolbar);
    d->removeItemAt(d->indexOfAction(action));
}

void ShadToolbar::clear()
{
    Q_D(ShadToolbar);
    while (!d->_items.isEmpty())
    {
        d->removeItemAt(d->_items.size() - 1);
    }
}

void ShadToolbar::setOrientation(Qt::Orientation orientation)
{
    Q_D(ShadToolbar);
    if (d->_orientation == orientation)
    {
        return;
    }
    d->_orientation = orientation;
    for (const ShadToolbarPrivate::Item& item : std::as_const(d->_items))
    {
        if (item.button != nullptr)
        {
            d->configureButton(item.button);
        }
    }
    d->_overflowButton->setShadIcon(overflowIcon(orientation));
    d->_overflowButton->setMenuSide(orientation == Qt::Horizontal ? ShadPlacementType::Bottom : ShadPlacementType::Right);
    setSizePolicy(orientation == Qt::Horizontal ? QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed) : QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));
    d->requestLayout();
}

Qt::Orientation ShadToolbar::getOrientation() const
{
    Q_D(const ShadToolbar);
    return d->_orientation;
}

void ShadToolbar::setButtonStyle(ShadToolbarType::ButtonStyle style)
{
    Q_D(ShadToolbar);
    if (d->_buttonStyle == style)
    {
        return;
    }
    d->_buttonStyle = style;
    for (const ShadToolbarPrivate::Item& item : std::as_const(d->_items))
    {
        if (item.button != nullptr)
        {
            d->configureButton(item.button);
        }
    }
    d->requestLayout();
}

ShadToolbarType::ButtonStyle ShadToolbar::getButtonStyle() const
{
    Q_D(const ShadToolbar);
    return d->_buttonStyle;
}

void ShadToolbar::setBordered(bool bordered)
{
    Q_D(ShadToolbar);
    if (d->_isBordered == bordered)
    {
        return;
    }
    d->_isBordered = bordered;
    update();
}

bool ShadToolbar::isBordered() const
{
    Q_D(const ShadToolbar);
    return d->_isBordered;
}

QSize ShadToolbar::sizeHint() const
{
    Q_D(const ShadToolbar);
    int length = 0;
    int count = 0;
    for (const ShadToolbarPrivate::Item& item : d->_items)
    {
        if (!d->isItemShown(item) || item.kind == ShadToolbarPrivate::ItemKind::Stretch)
        {
            continue;
        }
        length += d->itemLength(item);
        ++count;
    }
    length += ShadToolbarPrivate::kSpacing * qMax(0, count - 1) + 2 * ShadToolbarPrivate::kPadding;
    const int cross = d->crossLength() + 2 * ShadToolbarPrivate::kPadding;
    return d->isHorizontal() ? QSize(length, cross) : QSize(cross, length);
}

QSize ShadToolbar::minimumSizeHint() const
{
    Q_D(const ShadToolbar);
    const QSize overflowHint = d->_overflowButton->sizeHint();
    const int cross = d->crossLength() + 2 * ShadToolbarPrivate::kPadding;
    if (d->isHorizontal())
    {
        return QSize(overflowHint.width() + 2 * ShadToolbarPrivate::kPadding, cross);
    }
    return QSize(cross, overflowHint.height() + 2 * ShadToolbarPrivate::kPadding);
}

bool ShadToolbar::event(QEvent* event)
{
    Q_D(ShadToolbar);
    if (event->type() == QEvent::LayoutRequest)
    {
        updateGeometry();
        d->layoutItems();
    }
    else if (event->type() == QEvent::Show)
    {
        d->layoutItems();
    }
    return QWidget::event(event);
}

void ShadToolbar::changeEvent(QEvent* event)
{
    Q_D(ShadToolbar);
    if (event->type() == QEvent::LanguageChange)
    {
        d->_overflowAction->setText(tr("更多"));
    }
    else if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->layoutItems();
    }
    QWidget::changeEvent(event);
}

void ShadToolbar::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadToolbar);
    QWidget::resizeEvent(event);
    d->layoutItems();
}

void ShadToolbar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadToolbar);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (d->_isBordered)
    {
        const QRectF bounds(rect());
        const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
        ShadStyleHelper::fillRoundedRect(&painter, bounds, radius, shadTheme->getColor(ShadThemeType::Background));
        ShadStyleHelper::strokeRoundedRect(&painter, bounds, radius, shadTheme->getColor(ShadThemeType::Border));
    }
    painter.setRenderHint(QPainter::Antialiasing, false);
    for (const ShadToolbarPrivate::Item& item : std::as_const(d->_items))
    {
        if (item.kind == ShadToolbarPrivate::ItemKind::Separator && !item.separatorRect.isNull())
        {
            painter.fillRect(item.separatorRect, shadTheme->getColor(ShadThemeType::Border));
        }
    }
}
