#include "ShadWindowButton.h"

#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>

#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    constexpr int kIconSize = 14;
    constexpr int kDefaultHeight = 40;
    // 按下时背景透明度，与 hover:bg-xxx/80 一致
    constexpr qreal kPressedAlpha = 0.8;
} // namespace

ShadWindowButton::ShadWindowButton(Kind kind, QWidget* parent)
    : QAbstractButton(parent), _kind(kind)
{
    _hoverTransition = new ShadTransition(this);
    setFocusPolicy(Qt::NoFocus);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadWindowButton::~ShadWindowButton()
{
}

void ShadWindowButton::setKind(Kind kind)
{
    if (_kind == kind)
    {
        return;
    }
    _kind = kind;
    update();
}

ShadWindowButton::Kind ShadWindowButton::getKind() const
{
    return _kind;
}

void ShadWindowButton::setNativeHovered(bool isHovered)
{
    if (_isNativeHovered == isHovered)
    {
        return;
    }
    _isNativeHovered = isHovered;
    updateHoverTarget();
}

void ShadWindowButton::setNativePressed(bool isPressed)
{
    if (_isNativePressed == isPressed)
    {
        return;
    }
    _isNativePressed = isPressed;
    update();
}

bool ShadWindowButton::isNativePressed() const
{
    return _isNativePressed;
}

void ShadWindowButton::setWindowActive(bool isActive)
{
    if (_isWindowActive == isActive)
    {
        return;
    }
    _isWindowActive = isActive;
    update();
}

QSize ShadWindowButton::sizeHint() const
{
    return QSize(kButtonWidth, kDefaultHeight);
}

void ShadWindowButton::enterEvent(QEnterEvent* event)
{
    _isMouseOver = true;
    updateHoverTarget();
    QAbstractButton::enterEvent(event);
}

void ShadWindowButton::leaveEvent(QEvent* event)
{
    _isMouseOver = false;
    updateHoverTarget();
    QAbstractButton::leaveEvent(event);
}

void ShadWindowButton::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange)
    {
        _hoverTransition->jumpTo(isEnabled() && (_isMouseOver || _isNativeHovered) ? 1.0 : 0.0);
    }
    QAbstractButton::changeEvent(event);
}

void ShadWindowButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const qreal progress = _hoverTransition->value();
    const bool isClose = _kind == Kind::Close;
    const bool isPressed = isDown() || _isNativePressed;

    const QColor transparent(0, 0, 0, 0);
    QColor hoverBackground = shadTheme->getColor(isClose ? ShadThemeType::Destructive : ShadThemeType::Accent);
    if (isPressed)
    {
        hoverBackground = withAlpha(hoverBackground, kPressedAlpha);
    }
    const QColor normalIconColor = shadTheme->getColor(_isWindowActive ? ShadThemeType::Foreground : ShadThemeType::MutedForeground);
    const QColor hoverIconColor = shadTheme->getColor(isClose ? ShadThemeType::DestructiveForeground : ShadThemeType::AccentForeground);

    // 置顶时常驻 accent 背景，与悬停态相同
    const qreal backgroundProgress = isChecked() ? 1.0 : progress;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    painter.fillRect(rect(), mix(transparent, hoverBackground, backgroundProgress));
    ShadIcon::paint(&painter, QRectF(rect()), iconName(), mix(normalIconColor, hoverIconColor, backgroundProgress), kIconSize);
}

ShadIconType::IconName ShadWindowButton::iconName() const
{
    switch (_kind)
    {
        case Kind::StayOnTop:
            // 未置顶时显示图钉，置顶后显示取消图钉，点击即可取消
            return isChecked() ? ShadIconType::PinOff : ShadIconType::Pin;
        case Kind::Minimize:
            return ShadIconType::Minus;
        case Kind::Maximize:
            return ShadIconType::Square;
        case Kind::Restore:
            return ShadIconType::Copy;
        case Kind::Close:
            return ShadIconType::X;
    }
    return ShadIconType::None;
}

void ShadWindowButton::updateHoverTarget()
{
    _hoverTransition->animateTo(isEnabled() && (_isMouseOver || _isNativeHovered) ? 1.0 : 0.0);
}
