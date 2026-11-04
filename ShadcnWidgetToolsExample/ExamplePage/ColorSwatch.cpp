#include "ColorSwatch.h"

#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadTheme.h"

namespace
{
    // 普通模式：色块 132x52，下方两行文字
    constexpr int kSwatchWidth = 132;
    constexpr int kChipHeight = 52;
    constexpr int kSwatchHeight = 96;
    // 紧凑模式：36x36 色块
    constexpr int kCompactSize = 36;
    constexpr qreal kChipRadius = 8.0;
} // namespace

ColorSwatch::ColorSwatch(ShadThemeType::ThemeColor themeColor, bool isCompact, QWidget* parent)
    : QWidget(parent), _themeColor(themeColor), _isCompact(isCompact)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    updateToolTip();
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]()
    {
        updateToolTip();
        update();
    });
}

ColorSwatch::~ColorSwatch()
{
}

void ColorSwatch::setPresetBaseColor(ShadThemeType::BaseColor baseColor)
{
    _hasPresetBaseColor = true;
    _presetBaseColor = baseColor;
    updateToolTip();
    update();
}

QString ColorSwatch::tokenName(ShadThemeType::ThemeColor themeColor)
{
    switch (themeColor)
    {
        case ShadThemeType::Background:
            return QStringLiteral("background");
        case ShadThemeType::Foreground:
            return QStringLiteral("foreground");
        case ShadThemeType::Card:
            return QStringLiteral("card");
        case ShadThemeType::CardForeground:
            return QStringLiteral("card-foreground");
        case ShadThemeType::Popover:
            return QStringLiteral("popover");
        case ShadThemeType::PopoverForeground:
            return QStringLiteral("popover-foreground");
        case ShadThemeType::Primary:
            return QStringLiteral("primary");
        case ShadThemeType::PrimaryForeground:
            return QStringLiteral("primary-foreground");
        case ShadThemeType::Secondary:
            return QStringLiteral("secondary");
        case ShadThemeType::SecondaryForeground:
            return QStringLiteral("secondary-foreground");
        case ShadThemeType::Muted:
            return QStringLiteral("muted");
        case ShadThemeType::MutedForeground:
            return QStringLiteral("muted-foreground");
        case ShadThemeType::Accent:
            return QStringLiteral("accent");
        case ShadThemeType::AccentForeground:
            return QStringLiteral("accent-foreground");
        case ShadThemeType::Destructive:
            return QStringLiteral("destructive");
        case ShadThemeType::DestructiveForeground:
            return QStringLiteral("destructive-foreground");
        case ShadThemeType::Border:
            return QStringLiteral("border");
        case ShadThemeType::Input:
            return QStringLiteral("input");
        case ShadThemeType::Ring:
            return QStringLiteral("ring");
        case ShadThemeType::Sidebar:
            return QStringLiteral("sidebar");
        case ShadThemeType::SidebarForeground:
            return QStringLiteral("sidebar-foreground");
        case ShadThemeType::SidebarPrimary:
            return QStringLiteral("sidebar-primary");
        case ShadThemeType::SidebarPrimaryForeground:
            return QStringLiteral("sidebar-primary-foreground");
        case ShadThemeType::SidebarAccent:
            return QStringLiteral("sidebar-accent");
        case ShadThemeType::SidebarAccentForeground:
            return QStringLiteral("sidebar-accent-foreground");
        case ShadThemeType::SidebarBorder:
            return QStringLiteral("sidebar-border");
        case ShadThemeType::SidebarRing:
            return QStringLiteral("sidebar-ring");
        case ShadThemeType::Chart1:
            return QStringLiteral("chart-1");
        case ShadThemeType::Chart2:
            return QStringLiteral("chart-2");
        case ShadThemeType::Chart3:
            return QStringLiteral("chart-3");
        case ShadThemeType::Chart4:
            return QStringLiteral("chart-4");
        case ShadThemeType::Chart5:
            return QStringLiteral("chart-5");
        case ShadThemeType::ColorCount:
            break;
    }
    return QString();
}

QString ColorSwatch::colorText(const QColor& color)
{
    // 半透明颜色附带透明度，例如暗色下的 border 为 #ffffff / 10%
    if (color.alpha() < 255)
    {
        return QStringLiteral("%1 / %2%").arg(color.name(QColor::HexRgb)).arg(qRound(color.alphaF() * 100));
    }
    return color.name(QColor::HexRgb);
}

QSize ColorSwatch::sizeHint() const
{
    if (_isCompact)
    {
        return QSize(kCompactSize, kCompactSize);
    }
    return QSize(kSwatchWidth, kSwatchHeight);
}

void ColorSwatch::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    const QColor color = currentColor();
    const QRectF chipRect = _isCompact ? QRectF(0, 0, kCompactSize, kCompactSize) : QRectF(0, 0, kSwatchWidth, kChipHeight);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    // 色块描边使用当前主题的 border，保证与背景同色的 token 也能看清边界
    painter.setPen(QPen(shadTheme->getColor(ShadThemeType::Border), 1.0));
    painter.setBrush(color);
    painter.drawRoundedRect(chipRect.adjusted(0.5, 0.5, -0.5, -0.5), kChipRadius - 0.5, kChipRadius - 0.5);
    if (_isCompact)
    {
        return;
    }
    QFont nameFont = font();
    nameFont.setPixelSize(13);
    nameFont.setWeight(QFont::Medium);
    painter.setFont(nameFont);
    painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
    painter.drawText(QRectF(0, kChipHeight + 4, kSwatchWidth, 20), Qt::AlignLeft | Qt::AlignVCenter, tokenName(_themeColor));
    QFont valueFont = font();
    valueFont.setPixelSize(12);
    painter.setFont(valueFont);
    painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
    painter.drawText(QRectF(0, kChipHeight + 24, kSwatchWidth, 18), Qt::AlignLeft | Qt::AlignVCenter, colorText(color));
}

QColor ColorSwatch::currentColor() const
{
    if (_hasPresetBaseColor)
    {
        return ShadTheme::presetColor(_presetBaseColor, shadTheme->getThemeMode(), _themeColor);
    }
    return shadTheme->getColor(_themeColor);
}

void ColorSwatch::updateToolTip()
{
    setToolTip(QStringLiteral("%1: %2").arg(tokenName(_themeColor), colorText(currentColor())));
}
