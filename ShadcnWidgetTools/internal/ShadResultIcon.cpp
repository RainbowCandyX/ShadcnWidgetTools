#include "ShadResultIcon.h"

#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kCircleSize = 72;
    constexpr int kIconSize = 36;
    constexpr qreal kTintOpacity = 0.12;

    ShadIconType::IconName statusIcon(ShadResultType::Status status)
    {
        switch (status)
        {
            case ShadResultType::Success:
                return ShadIconType::CircleCheck;
            case ShadResultType::Error:
                return ShadIconType::CircleX;
            case ShadResultType::Warning:
                return ShadIconType::TriangleAlert;
            case ShadResultType::Info:
                return ShadIconType::Info;
            case ShadResultType::Forbidden:
                return ShadIconType::ShieldAlert;
            case ShadResultType::NotFound:
                return ShadIconType::SearchX;
            case ShadResultType::ServerError:
                return ShadIconType::ServerCrash;
        }
        return ShadIconType::Info;
    }

    QColor statusColor(ShadResultType::Status status)
    {
        const bool isDark = shadTheme->isDark();
        switch (status)
        {
            case ShadResultType::Success:
                return isDark ? QColor(0x4a, 0xde, 0x80) : QColor(0x16, 0xa3, 0x4a);
            case ShadResultType::Warning:
                return isDark ? QColor(0xfb, 0xbf, 0x24) : QColor(0xd9, 0x77, 0x06);
            case ShadResultType::Info:
                return isDark ? QColor(0x60, 0xa5, 0xfa) : QColor(0x25, 0x63, 0xeb);
            case ShadResultType::Error:
            case ShadResultType::ServerError:
                return shadTheme->getColor(ShadThemeType::Destructive);
            case ShadResultType::Forbidden:
            case ShadResultType::NotFound:
                break;
        }
        return QColor();
    }
} // namespace

ShadResultIcon::ShadResultIcon(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadResultIcon::~ShadResultIcon()
{
}

void ShadResultIcon::setStatus(ShadResultType::Status status)
{
    if (_status == status)
    {
        return;
    }
    _status = status;
    update();
}

ShadResultType::Status ShadResultIcon::getStatus() const
{
    return _status;
}

void ShadResultIcon::setIcon(ShadIconType::IconName icon)
{
    if (_icon == icon)
    {
        return;
    }
    _icon = icon;
    update();
}

ShadIconType::IconName ShadResultIcon::getIcon() const
{
    return _icon;
}

void ShadResultIcon::setColor(const QColor& color)
{
    if (_color == color)
    {
        return;
    }
    _color = color;
    update();
}

QColor ShadResultIcon::getColor() const
{
    return _color;
}

QSize ShadResultIcon::sizeHint() const
{
    return QSize(kCircleSize, kCircleSize);
}

QSize ShadResultIcon::minimumSizeHint() const
{
    return sizeHint();
}

void ShadResultIcon::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QColor color = _color.isValid() ? _color : statusColor(_status);
    const QRectF circleRect(rect());
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    fillRoundedRect(&painter, circleRect, circleRect.height() / 2.0, color.isValid() ? withAlpha(color, kTintOpacity) : shadTheme->getColor(ShadThemeType::Muted));
    ShadIcon::paint(&painter, circleRect, _icon != ShadIconType::None ? _icon : statusIcon(_status), color.isValid() ? color : shadTheme->getColor(ShadThemeType::Foreground), kIconSize);
}
