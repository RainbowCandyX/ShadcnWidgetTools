#include "ShadToolbarButton.h"

#include <QAction>
#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFont>
#include <QIcon>
#include <QKeySequence>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtMath>

#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTooltip.h"
#include "ShadTransition.h"

namespace
{
    constexpr int kHeight = 32;
    constexpr int kIconSize = 16;
    constexpr int kIconPaddingX = 8;
    constexpr int kTextPaddingX = 10;
    constexpr int kContentGap = 6;
    constexpr int kChevronSize = 12;
    constexpr int kChevronGap = 2;
} // namespace

ShadToolbarButton::ShadToolbarButton(QAction* action, ShadIconType::IconName icon, ShadDropdownMenu* menu, QWidget* parent)
    : QAbstractButton(parent), _action(action), _menu(menu), _icon(icon)
{
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover);
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);
    _hoverTransition = new ShadTransition(this);
    if (_action != nullptr)
    {
        connect(_action, &QAction::changed, this, &ShadToolbarButton::syncAction);
    }
    if (_menu != nullptr)
    {
        connect(_menu, &ShadDropdownMenu::aboutToHide, this, [this]()
        {
            _isMenuOpen = false;
            if (!underMouse())
            {
                _hoverTransition->jumpTo(0.0);
            }
            update();
        });
    }
    connect(this, &QAbstractButton::clicked, this, [this]()
    {
        if (!_menu.isNull())
        {
            openMenu();
        }
        else if (!_action.isNull())
        {
            _action->trigger();
        }
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    syncAction();
}

ShadToolbarButton::~ShadToolbarButton()
{
}

QAction* ShadToolbarButton::getAction() const
{
    return _action.data();
}

ShadDropdownMenu* ShadToolbarButton::getMenu() const
{
    return _menu.data();
}

void ShadToolbarButton::setShadIcon(ShadIconType::IconName icon)
{
    if (_icon == icon)
    {
        return;
    }
    _icon = icon;
    updateGeometry();
    update();
}

ShadIconType::IconName ShadToolbarButton::getShadIcon() const
{
    return _icon;
}

void ShadToolbarButton::setButtonStyle(ShadToolbarType::ButtonStyle style)
{
    if (_style == style)
    {
        return;
    }
    _style = style;
    syncAction();
}

ShadToolbarType::ButtonStyle ShadToolbarButton::getButtonStyle() const
{
    return _style;
}

void ShadToolbarButton::setStartAligned(bool startAligned)
{
    if (_isStartAligned == startAligned)
    {
        return;
    }
    _isStartAligned = startAligned;
    update();
}

void ShadToolbarButton::setMenuSide(ShadPlacementType::Side side)
{
    _menuSide = side;
}

void ShadToolbarButton::setChevronVisible(bool visible)
{
    if (_isChevronVisible == visible)
    {
        return;
    }
    _isChevronVisible = visible;
    updateGeometry();
    update();
}

QSize ShadToolbarButton::sizeHint() const
{
    const bool showIcon = isIconShown();
    const bool showText = isTextShown();
    int width = qCeil(ShadStyleHelper::iconTextWidth(font(), showText ? text() : QString(), showIcon, kContentGap, kIconSize));
    if (hasChevron())
    {
        width += kChevronGap + kChevronSize;
    }
    width += 2 * (showText ? kTextPaddingX : kIconPaddingX);
    return QSize(qMax(width, kHeight), kHeight);
}

QSize ShadToolbarButton::minimumSizeHint() const
{
    return sizeHint();
}

void ShadToolbarButton::enterEvent(QEnterEvent* event)
{
    if (isEnabled())
    {
        _hoverTransition->animateTo(1.0);
    }
    QAbstractButton::enterEvent(event);
}

void ShadToolbarButton::leaveEvent(QEvent* event)
{
    _hoverTransition->animateTo(0.0);
    QAbstractButton::leaveEvent(event);
}

void ShadToolbarButton::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange && !isEnabled())
    {
        _hoverTransition->jumpTo(0.0);
    }
    else if (event->type() == QEvent::LayoutDirectionChange)
    {
        update();
    }
    QAbstractButton::changeEvent(event);
}

void ShadToolbarButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const bool isChecked = !_action.isNull() && _action->isCheckable() && _action->isChecked();
    const bool isActive = _isMenuOpen || isDown();
    const qreal hover = isActive ? 1.0 : _hoverTransition->value();
    const QColor transparent(0, 0, 0, 0);
    const QColor background = isChecked ? selectionColor(ShadThemeType::Accent) : mix(transparent, shadTheme->getColor(ShadThemeType::Accent), hover);
    const QColor textColor = isChecked ? shadTheme->getColor(ShadThemeType::AccentForeground) : mix(shadTheme->getColor(ShadThemeType::Foreground), shadTheme->getColor(ShadThemeType::AccentForeground), hover);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    const QRectF bounds(rect());
    fillRoundedRect(&painter, bounds, shadTheme->getRadius(ShadThemeType::Medium), background);

    const bool showIcon = isIconShown();
    const bool showText = isTextShown();
    const qreal paddingX = showText ? kTextPaddingX : kIconPaddingX;
    QRectF contentRect = bounds.adjusted(paddingX, 0, -paddingX, 0);
    if (hasChevron())
    {
        const QRectF chevronRect(contentRect.right() - kChevronSize, (bounds.height() - kChevronSize) / 2.0, kChevronSize, kChevronSize);
        ShadIcon::paint(&painter, visualRect(layoutDirection(), bounds, chevronRect), ShadIconType::ChevronDown, textColor, kChevronSize);
        contentRect.setRight(chevronRect.left() - kChevronGap);
    }
    const Qt::Alignment alignment = _isStartAligned || hasChevron() ? (Qt::AlignLeft | Qt::AlignVCenter) : Qt::AlignCenter;
    drawIconText(&painter, contentRect, showIcon ? _icon : ShadIconType::None, QIcon(), showText ? text() : QString(), font(), textColor, kContentGap, alignment, kIconSize, 0.0, layoutDirection());
}

void ShadToolbarButton::syncAction()
{
    if (_action.isNull())
    {
        return;
    }
    setText(_action->iconText());
    setEnabled(_action->isEnabled());
    const bool showTooltip = !isTextShown();
    setToolTip(showTooltip ? _action->toolTip() : QString());
    const QKeySequence shortcut = _action->shortcut();
    if (showTooltip && !shortcut.isEmpty())
    {
        ShadTooltip::setShortcut(this, shortcut);
    }
    else
    {
        ShadTooltip::setShortcut(this, QStringList());
    }
    updateGeometry();
    update();
    Q_EMIT contentChanged();
}

void ShadToolbarButton::openMenu()
{
    if (_menu.isNull())
    {
        return;
    }
    Q_EMIT aboutToShowMenu();
    _isMenuOpen = true;
    update();
    _menu->popup(this, _menuSide, ShadPlacementType::Start);
}

bool ShadToolbarButton::isIconShown() const
{
    if (_icon == ShadIconType::None)
    {
        return false;
    }
    return _style != ShadToolbarType::TextOnly || text().isEmpty();
}

bool ShadToolbarButton::isTextShown() const
{
    if (text().isEmpty())
    {
        return false;
    }
    return _style != ShadToolbarType::IconOnly || _icon == ShadIconType::None;
}

bool ShadToolbarButton::hasChevron() const
{
    return !_menu.isNull() && _isChevronVisible;
}
