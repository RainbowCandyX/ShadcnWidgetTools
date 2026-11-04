#ifndef SHADTHEMEPRIVATE_H
#define SHADTHEMEPRIVATE_H

#include <QColor>
#include <QImage>
#include <QtCore/qnamespace.h>

#include <array>

#include "ShadDef.h"

class ShadTheme;

class ShadThemePrivate
{
    SHAD_D_CREATE(ShadTheme)

    public:
        ShadThemePrivate();
        ~ShadThemePrivate();

        using ColorTable = std::array<QColor, ShadThemeType::ColorCount>;

        ShadThemeType::ThemeMode _themeMode{ShadThemeType::Light};
        ShadThemeType::BaseColor _baseColor{ShadThemeType::Neutral};
        bool _isFollowSystemTheme{false};
        ColorTable _lightColors;
        ColorTable _darkColors;
        int _baseRadius{10};
        bool _isPageTransitionEnabled{true};
        int _pageTransitionDuration{300};
        ShadThemeType::PageTransitionStyle _pageTransitionStyle{ShadThemeType::Slide};
        static constexpr int kWindowEffectCount = ShadWindowType::MicaAlt + 1;
        static constexpr qreal kMutedTextOpacityThreshold = 0.5;

        ShadWindowType::Effect _windowEffect{ShadWindowType::NoEffect};
        std::array<qreal, kWindowEffectCount> _windowEffectOpacities{};
        QImage _windowEffectImage;
        int _windowEffectBlurRadius{0};
        bool _isWindowEffectDesktopOnly{true};
        qreal _windowEffectSurfaceOpacity{0.7};

        ColorTable& colorTable(ShadThemeType::ThemeMode themeMode);
        const ColorTable& colorTable(ShadThemeType::ThemeMode themeMode) const;
        void applyBaseColor(ShadThemeType::BaseColor baseColor);
        // 不修改当前主题
        static void buildColorTables(ShadThemeType::BaseColor baseColor, ColorTable& light, ColorTable& dark);
        // 无法判断时视为亮色
        static ShadThemeType::ThemeMode systemThemeMode();
        void onSystemColorSchemeChanged(Qt::ColorScheme colorScheme);
        bool isEffectActive() const;
        bool isEffectBackgroundTranslucent() const;
        // 随组件不透明度变化的颜色
        static bool isSurfaceColor(ShadThemeType::ThemeColor themeColor);
};

#endif // SHADTHEMEPRIVATE_H
