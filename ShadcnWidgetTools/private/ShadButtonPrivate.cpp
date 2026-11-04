#include "ShadButtonPrivate.h"

#include <QIcon>

#include "ShadButton.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadButtonPrivate::ShadButtonPrivate()
{
}

ShadButtonPrivate::~ShadButtonPrivate()
{
}

ShadButtonPrivate::SizeMetrics ShadButtonPrivate::sizeMetrics(ShadButtonType::Size size)
{
    // 尺寸对应 default: h-9 px-4 has-[>svg]:px-3 / sm: h-8 px-3 gap-1.5 has-[>svg]:px-2.5 / lg: h-10 px-6 has-[>svg]:px-4
    // icon-xs: size-6 rounded-md [&_svg]:size-3，其余尺寸图标为 size-4
    // xs: h-6 gap-1 px-2 text-xs has-[>svg]:px-1.5 [&_svg]:size-3
    switch (size)
    {
        case ShadButtonType::Small:
            return {32, 12, 10, 6, false, 16, 0};
        case ShadButtonType::Large:
            return {40, 24, 16, 8, false, 16, 0};
        case ShadButtonType::Icon:
            return {36, 0, 0, 8, true, 16, 0};
        case ShadButtonType::IconSmall:
            return {32, 0, 0, 6, true, 16, 0};
        case ShadButtonType::IconLarge:
            return {40, 0, 0, 8, true, 16, 0};
        case ShadButtonType::IconExtraSmall:
            return {24, 0, 0, 4, true, 12, 0};
        case ShadButtonType::ExtraSmall:
            return {24, 8, 6, 4, false, 12, 12};
        case ShadButtonType::Medium:
            break;
    }
    return {36, 16, 12, 8, false, 16, 0};
}

QFont ShadButtonPrivate::contentFont() const
{
    Q_Q(const ShadButton);
    QFont font = q->font();
    const int pixelSize = sizeMetrics(_size).fontPixelSize;
    if (pixelSize > 0)
    {
        font.setPixelSize(pixelSize);
    }
    return font;
}

ShadButtonPrivate::VariantColors ShadButtonPrivate::variantColors(ShadButtonType::Variant variant)
{
    using namespace ShadThemeType;
    using ShadStyleHelper::withAlpha;
    const bool isDark = shadTheme->isDark();
    const QColor transparent(0, 0, 0, 0);
    switch (variant)
    {
        case ShadButtonType::Destructive:
            {
                // 对应 bg-destructive text-white hover:bg-destructive/90 dark:bg-destructive/60
                const QColor destructive = shadTheme->getColor(Destructive);
                const QColor text = shadTheme->getColor(DestructiveForeground);
                return {isDark ? withAlpha(destructive, 0.6) : destructive, isDark ? withAlpha(destructive, 0.5) : withAlpha(destructive, 0.9), text, text, transparent, false, false};
            }
        case ShadButtonType::Outline:
            {
                // 对应 border bg-background shadow-xs hover:bg-accent dark:bg-input/30 dark:border-input dark:hover:bg-input/50
                const QColor input = shadTheme->getColor(Input);
                const QColor background = isDark ? withAlpha(input, 0.3) : shadTheme->getColor(Background);
                const QColor hoverBackground = isDark ? withAlpha(input, 0.5) : shadTheme->getColor(Accent);
                const QColor border = isDark ? input : shadTheme->getColor(Border);
                return {background, hoverBackground, shadTheme->getColor(Foreground), shadTheme->getColor(AccentForeground), border, true, false};
            }
        case ShadButtonType::Secondary:
            {
                // 对应 bg-secondary text-secondary-foreground hover:bg-secondary/80
                const QColor secondary = shadTheme->getColor(Secondary);
                const QColor text = shadTheme->getColor(SecondaryForeground);
                return {secondary, withAlpha(secondary, 0.8), text, text, transparent, false, false};
            }
        case ShadButtonType::Ghost:
            {
                // 对应 hover:bg-accent hover:text-accent-foreground dark:hover:bg-accent/50
                const QColor accent = shadTheme->getColor(Accent);
                return {transparent, isDark ? withAlpha(accent, 0.5) : accent, shadTheme->getColor(Foreground), shadTheme->getColor(AccentForeground), transparent, false, false};
            }
        case ShadButtonType::Link:
            {
                // 对应 text-primary underline-offset-4 hover:underline
                const QColor primary = shadTheme->getColor(Primary);
                return {transparent, transparent, primary, primary, transparent, false, true};
            }
        case ShadButtonType::Default:
            break;
    }
    // 对应 bg-primary text-primary-foreground hover:bg-primary/90
    const QColor primary = shadTheme->getColor(Primary);
    const QColor text = shadTheme->getColor(PrimaryForeground);
    return {primary, withAlpha(primary, 0.9), text, text, transparent, false, false};
}

bool ShadButtonPrivate::hasIcon() const
{
    Q_Q(const ShadButton);
    return _isLoading || _shadIcon != ShadIconType::None || !q->icon().isNull();
}

ShadIconType::IconName ShadButtonPrivate::displayIcon() const
{
    return _isLoading ? ShadIconType::LoaderCircle : _shadIcon;
}

bool ShadButtonPrivate::isActivationKey(int key)
{
    return key == Qt::Key_Space || key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Select;
}
