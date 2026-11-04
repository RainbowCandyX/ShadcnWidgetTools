#include "ShadThemePrivate.h"

#include <QGuiApplication>
#include <QStyleHints>

#include "ShadTheme.h"
#include "ShadWindowEffect.h"

namespace
{
    // Tailwind CSS v4 灰阶色板中 shadcn 用到的色阶（sRGB 近似值）
    struct GrayScale
    {
        QColor c50;
        QColor c100;
        QColor c200;
        QColor c400;
        QColor c500;
        QColor c800;
        QColor c900;
        QColor c950;
    };

    GrayScale grayScale(ShadThemeType::BaseColor baseColor)
    {
        switch (baseColor)
        {
            case ShadThemeType::Zinc:
                return {QColor(0xFA, 0xFA, 0xFA), QColor(0xF4, 0xF4, 0xF5), QColor(0xE4, 0xE4, 0xE7), QColor(0x9F, 0x9F, 0xA9), QColor(0x71, 0x71, 0x7B), QColor(0x27, 0x27, 0x2A), QColor(0x18, 0x18, 0x1B), QColor(0x09, 0x09, 0x0B)};
            case ShadThemeType::Slate:
                return {QColor(0xF8, 0xFA, 0xFC), QColor(0xF1, 0xF5, 0xF9), QColor(0xE2, 0xE8, 0xF0), QColor(0x90, 0xA1, 0xB9), QColor(0x62, 0x74, 0x8E), QColor(0x1D, 0x29, 0x3D), QColor(0x0F, 0x17, 0x2B), QColor(0x02, 0x06, 0x18)};
            case ShadThemeType::Stone:
                return {QColor(0xFA, 0xFA, 0xF9), QColor(0xF5, 0xF5, 0xF4), QColor(0xE7, 0xE5, 0xE4), QColor(0xA6, 0xA0, 0x9B), QColor(0x79, 0x71, 0x6B), QColor(0x29, 0x25, 0x24), QColor(0x1C, 0x19, 0x17), QColor(0x0C, 0x0A, 0x09)};
            case ShadThemeType::Gray:
                return {QColor(0xF9, 0xFA, 0xFB), QColor(0xF3, 0xF4, 0xF6), QColor(0xE5, 0xE7, 0xEB), QColor(0x99, 0xA1, 0xAF), QColor(0x6A, 0x72, 0x82), QColor(0x1E, 0x29, 0x39), QColor(0x10, 0x18, 0x28), QColor(0x03, 0x07, 0x12)};
            case ShadThemeType::Neutral:
                break;
        }
        return {QColor(0xFA, 0xFA, 0xFA), QColor(0xF5, 0xF5, 0xF5), QColor(0xE5, 0xE5, 0xE5), QColor(0xA1, 0xA1, 0xA1), QColor(0x73, 0x73, 0x73), QColor(0x26, 0x26, 0x26), QColor(0x17, 0x17, 0x17), QColor(0x0A, 0x0A, 0x0A)};
    }
} // namespace

ShadThemePrivate::ShadThemePrivate()
{
    applyBaseColor(ShadThemeType::Neutral);
    for (int i = 0; i < kWindowEffectCount; ++i)
    {
        _windowEffectOpacities[static_cast<size_t>(i)] = ShadWindowEffect::defaultOpacity(static_cast<ShadWindowType::Effect>(i));
    }
    _windowEffectBlurRadius = ShadWindowEffect::kDefaultBlurRadius;
}

ShadThemePrivate::~ShadThemePrivate()
{
}

ShadThemePrivate::ColorTable& ShadThemePrivate::colorTable(ShadThemeType::ThemeMode themeMode)
{
    return themeMode == ShadThemeType::Dark ? _darkColors : _lightColors;
}

const ShadThemePrivate::ColorTable& ShadThemePrivate::colorTable(ShadThemeType::ThemeMode themeMode) const
{
    return themeMode == ShadThemeType::Dark ? _darkColors : _lightColors;
}

void ShadThemePrivate::applyBaseColor(ShadThemeType::BaseColor baseColor)
{
    _baseColor = baseColor;
    buildColorTables(baseColor, _lightColors, _darkColors);
}

void ShadThemePrivate::buildColorTables(ShadThemeType::BaseColor baseColor, ColorTable& light, ColorTable& dark)
{
    // 各 token 与灰阶色阶的对应关系与 shadcn/ui v4 的 base color 定义一致
    using namespace ShadThemeType;
    const GrayScale scale = grayScale(baseColor);
    const QColor white(0xFF, 0xFF, 0xFF);

    light[Background] = white;
    light[Foreground] = scale.c950;
    light[Card] = white;
    light[CardForeground] = scale.c950;
    light[Popover] = white;
    light[PopoverForeground] = scale.c950;
    light[Primary] = scale.c900;
    light[PrimaryForeground] = scale.c50;
    light[Secondary] = scale.c100;
    light[SecondaryForeground] = scale.c900;
    light[Muted] = scale.c100;
    light[MutedForeground] = scale.c500;
    light[Accent] = scale.c100;
    light[AccentForeground] = scale.c900;
    light[Destructive] = QColor(0xE7, 0x00, 0x0B);
    light[DestructiveForeground] = white;
    light[Border] = scale.c200;
    light[Input] = scale.c200;
    light[Ring] = scale.c400;
    light[Sidebar] = scale.c50;
    light[SidebarForeground] = scale.c950;
    light[SidebarPrimary] = scale.c900;
    light[SidebarPrimaryForeground] = scale.c50;
    light[SidebarAccent] = scale.c100;
    light[SidebarAccentForeground] = scale.c900;
    light[SidebarBorder] = scale.c200;
    light[SidebarRing] = scale.c400;
    // 图表色：oklch(0.646 0.222 41.116)、oklch(0.6 0.118 184.704)、oklch(0.398 0.07 227.392)、oklch(0.828 0.189 84.429)、oklch(0.769 0.188 70.08)
    light[Chart1] = QColor(0xF5, 0x49, 0x00);
    light[Chart2] = QColor(0x00, 0x96, 0x89);
    light[Chart3] = QColor(0x10, 0x4E, 0x64);
    light[Chart4] = QColor(0xFF, 0xB9, 0x00);
    light[Chart5] = QColor(0xFE, 0x9A, 0x00);

    dark[Background] = scale.c950;
    dark[Foreground] = scale.c50;
    dark[Card] = scale.c900;
    dark[CardForeground] = scale.c50;
    dark[Popover] = scale.c900;
    dark[PopoverForeground] = scale.c50;
    dark[Primary] = scale.c200;
    dark[PrimaryForeground] = scale.c900;
    dark[Secondary] = scale.c800;
    dark[SecondaryForeground] = scale.c50;
    dark[Muted] = scale.c800;
    dark[MutedForeground] = scale.c400;
    dark[Accent] = scale.c800;
    dark[AccentForeground] = scale.c50;
    dark[Destructive] = QColor(0xFF, 0x64, 0x67);
    dark[DestructiveForeground] = white;
    // 暗色下 border 为白色 10% 透明度，input 为白色 15% 透明度
    dark[Border] = QColor(0xFF, 0xFF, 0xFF, 26);
    dark[Input] = QColor(0xFF, 0xFF, 0xFF, 38);
    dark[Ring] = scale.c500;
    // 暗色下 sidebar-primary 为 oklch(0.488 0.243 264.376)，约为 #1447E6
    dark[Sidebar] = scale.c900;
    dark[SidebarForeground] = scale.c50;
    dark[SidebarPrimary] = QColor(0x14, 0x47, 0xE6);
    dark[SidebarPrimaryForeground] = scale.c50;
    dark[SidebarAccent] = scale.c800;
    dark[SidebarAccentForeground] = scale.c50;
    dark[SidebarBorder] = QColor(0xFF, 0xFF, 0xFF, 26);
    dark[SidebarRing] = scale.c500;
    // 图表色：oklch(0.488 0.243 264.376)、oklch(0.696 0.17 162.48)、oklch(0.769 0.188 70.08)、oklch(0.627 0.265 303.9)、oklch(0.645 0.246 16.439)
    dark[Chart1] = QColor(0x14, 0x47, 0xE6);
    dark[Chart2] = QColor(0x00, 0xBC, 0x7D);
    dark[Chart3] = QColor(0xFE, 0x9A, 0x00);
    dark[Chart4] = QColor(0xAD, 0x46, 0xFF);
    dark[Chart5] = QColor(0xFF, 0x20, 0x56);
}

ShadThemeType::ThemeMode ShadThemePrivate::systemThemeMode()
{
    if (qGuiApp == nullptr)
    {
        return ShadThemeType::Light;
    }
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark ? ShadThemeType::Dark : ShadThemeType::Light;
}

bool ShadThemePrivate::isEffectActive() const
{
    if (_windowEffect == ShadWindowType::NoEffect)
    {
        return false;
    }
    return _windowEffect != ShadWindowType::Wallpaper || !ShadWindowEffect::wallpaper().isNull();
}

bool ShadThemePrivate::isEffectBackgroundTranslucent() const
{
    return isEffectActive() && _windowEffectOpacities[static_cast<size_t>(_windowEffect)] < kMutedTextOpacityThreshold;
}

bool ShadThemePrivate::isSurfaceColor(ShadThemeType::ThemeColor themeColor)
{
    switch (themeColor)
    {
        case ShadThemeType::Background:
        case ShadThemeType::Card:
        case ShadThemeType::Popover:
        case ShadThemeType::Muted:
        case ShadThemeType::Accent:
        case ShadThemeType::Secondary:
        case ShadThemeType::Sidebar:
        case ShadThemeType::SidebarAccent:
        case ShadThemeType::Border:
        case ShadThemeType::SidebarBorder:
        case ShadThemeType::Input:
            return true;
        default:
            return false;
    }
}

void ShadThemePrivate::onSystemColorSchemeChanged(Qt::ColorScheme colorScheme)
{
    Q_Q(ShadTheme);
    if (!_isFollowSystemTheme)
    {
        return;
    }
    const ShadThemeType::ThemeMode themeMode = colorScheme == Qt::ColorScheme::Dark ? ShadThemeType::Dark : ShadThemeType::Light;
    if (_themeMode == themeMode)
    {
        return;
    }
    _themeMode = themeMode;
    Q_EMIT q->themeModeChanged(themeMode);
    Q_EMIT q->themeChanged();
}
