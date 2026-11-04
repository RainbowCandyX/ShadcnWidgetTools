#include "ShadNavigationMenuTrigger.h"

#include <QAbstractButton>
#include <QColor>
#include <QCursor>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QString>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // h-9 px-4 text-sm font-medium
    constexpr int kHeight = 36;
    constexpr int kPaddingX = 16;
    // size-3 ml-1，top-[1px]
    constexpr int kChevronSize = 12;
    constexpr int kChevronMargin = 4;
    constexpr int kChevronOffsetY = 1;
    constexpr int kChevronDurationMs = 300;
    constexpr qreal kOpenAlpha = 0.5;
} // namespace

ShadNavigationMenuTrigger::ShadNavigationMenuTrigger(const QString& text, bool hasMenu, QWidget* parent)
    : QAbstractButton(parent), _hasMenu(hasMenu)
{
    setText(text);
    setFocusPolicy(Qt::StrongFocus);
#ifdef Q_OS_MACOS
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);

    _chevronAnimation = new QVariantAnimation(this);
    _chevronAnimation->setDuration(kChevronDurationMs);
    QEasingCurve easingCurve(QEasingCurve::BezierSpline);
    easingCurve.addCubicBezierSegment(QPointF(0.4, 0.0), QPointF(0.2, 1.0), QPointF(1.0, 1.0));
    _chevronAnimation->setEasingCurve(easingCurve);
    connect(_chevronAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        _chevronProgress = value.toReal();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadNavigationMenuTrigger::~ShadNavigationMenuTrigger()
{
}

bool ShadNavigationMenuTrigger::hasMenu() const
{
    return _hasMenu;
}

void ShadNavigationMenuTrigger::setOpen(bool open)
{
    if (_isOpen == open)
    {
        return;
    }
    _isOpen = open;
    _chevronAnimation->stop();
    _chevronAnimation->setStartValue(_chevronProgress);
    _chevronAnimation->setEndValue(open ? 1.0 : 0.0);
    _chevronAnimation->start();
    update();
}

bool ShadNavigationMenuTrigger::isOpen() const
{
    return _isOpen;
}

void ShadNavigationMenuTrigger::syncHover()
{
    const bool hovered = isVisible() && rect().contains(mapFromGlobal(QCursor::pos()));
    if (_isHovered == hovered)
    {
        return;
    }
    _isHovered = hovered;
    update();
}

QSize ShadNavigationMenuTrigger::sizeHint() const
{
    const int chevronWidth = _hasMenu ? chevronGap() + kChevronSize : 0;
    return QSize(ShadStyleHelper::textWidth(font(), text()) + chevronWidth + 2 * kPaddingX, kHeight);
}

QSize ShadNavigationMenuTrigger::minimumSizeHint() const
{
    return sizeHint();
}

void ShadNavigationMenuTrigger::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            event->ignore();
            return;
        default:
            break;
    }
    QAbstractButton::keyPressEvent(event);
}

void ShadNavigationMenuTrigger::enterEvent(QEnterEvent* event)
{
    _isHovered = true;
    update();
    Q_EMIT hoverChanged(true);
    QAbstractButton::enterEvent(event);
}

void ShadNavigationMenuTrigger::leaveEvent(QEvent* event)
{
    _isHovered = false;
    update();
    Q_EMIT hoverChanged(false);
    QAbstractButton::leaveEvent(event);
}

void ShadNavigationMenuTrigger::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QAbstractButton::focusInEvent(event);
    update();
}

void ShadNavigationMenuTrigger::focusOutEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QAbstractButton::focusOutEvent(event);
    update();
}

void ShadNavigationMenuTrigger::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QAbstractButton::changeEvent(event);
}

void ShadNavigationMenuTrigger::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF bounds(rect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const bool isActive = isEnabled() && (_isHovered || hasFocus());
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    // bg-background，hover / focus 为 bg-accent，打开时 bg-accent/50
    QColor background = shadTheme->getColor(ShadThemeType::Background);
    if (isActive)
    {
        background = shadTheme->getColor(ShadThemeType::Accent);
    }
    else if (_isOpen)
    {
        background = withAlpha(shadTheme->getColor(ShadThemeType::Accent), kOpenAlpha);
    }
    fillRoundedRect(&painter, bounds, radius, background);

    const QColor textColor = shadTheme->getColor((isActive || _isOpen) ? ShadThemeType::AccentForeground : ShadThemeType::Foreground);
    const int textWidth = ShadStyleHelper::textWidth(font(), text());
    const int contentWidth = textWidth + (_hasMenu ? chevronGap() + kChevronSize : 0);
    const qreal left = (bounds.width() - contentWidth) / 2.0;
    const QRectF textRect = visualRect(layoutDirection(), bounds, QRectF(left, 0, textWidth, bounds.height()));
    painter.setFont(font());
    painter.setPen(textColor);
    painter.drawText(textRect, Qt::AlignCenter, text());
    if (_hasMenu)
    {
        const QRectF logicalIconRect(left + textWidth + chevronGap(), (bounds.height() - kChevronSize) / 2.0 + kChevronOffsetY, kChevronSize, kChevronSize);
        const QRectF iconRect = visualRect(layoutDirection(), bounds, logicalIconRect);
        painter.save();
        painter.translate(iconRect.center());
        painter.rotate(180.0 * _chevronProgress);
        painter.translate(-iconRect.center());
        ShadIcon::paint(&painter, iconRect, ShadIconType::ChevronDown, textColor, kChevronSize);
        painter.restore();
    }

    if (hasFocus() && _isFocusVisible)
    {
        const QRectF box = bounds.adjusted(kFocusRingWidth, kFocusRingWidth, -kFocusRingWidth, -kFocusRingWidth);
        strokeRoundedRect(&painter, box, radius, shadTheme->getColor(ShadThemeType::Ring));
        drawFocusRing(&painter, box, radius, focusRingColor(false));
    }
}

int ShadNavigationMenuTrigger::chevronGap() const
{
    // 文字后的空格加 ml-1
    return ShadStyleHelper::textWidth(font(), QStringLiteral(" ")) + kChevronMargin;
}
