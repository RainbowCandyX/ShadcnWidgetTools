#include "ShadTheme.h"

#include <QApplication>
#include <QFont>
#include <QGuiApplication>
#include <QImage>
#include <QMetaObject>
#include <QString>
#include <QStringList>
#include <QStyleHints>
#include <QtGlobal>

#include "ShadStyleHelper.h"
#include "ShadThemePrivate.h"
#include "ShadWindowEffect.h"

namespace
{
    // 字体缺少的字形按 families 顺序回退，把内置 emoji 字体追加到全局字体的末尾，正文字体没有的 emoji 使用它显示
    void appendEmojiFontToApplicationFont()
    {
        const QString emojiFamily = ShadStyleHelper::emojiFontFamily();
        if (emojiFamily.isEmpty())
        {
            return;
        }
        QFont font = QApplication::font();
        QStringList families = font.families();
        if (families.isEmpty())
        {
            families.append(font.family());
        }
        if (families.contains(emojiFamily))
        {
            return;
        }
        families.append(emojiFamily);
        font.setFamilies(families);
        QApplication::setFont(font);
    }
} // namespace

ShadTheme::ShadTheme(QObject* parent)
    : QObject(parent), d_ptr(new ShadThemePrivate())
{
    Q_D(ShadTheme);
    d->q_ptr = this;
    if (qGuiApp != nullptr)
    {
        connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [d](Qt::ColorScheme colorScheme) { d->onSystemColorSchemeChanged(colorScheme); });
        // 主题可能在某个组件的构造函数中首次创建，此时同步修改全局字体会让构造到一半的组件收到 FontChange，排到事件队列中执行
        QMetaObject::invokeMethod(qApp, &appendEmojiFontToApplicationFont, Qt::QueuedConnection);
    }
    else
    {
        qWarning("ShadTheme 在 QGuiApplication 创建前被初始化，跟随系统主题功能不可用");
    }
}

ShadTheme::~ShadTheme()
{
}

ShadTheme* ShadTheme::getInstance()
{
    static ShadTheme instance;
    return &instance;
}

void ShadTheme::setThemeMode(ShadThemeType::ThemeMode themeMode)
{
    Q_D(ShadTheme);
    setFollowSystemTheme(false);
    if (d->_themeMode == themeMode)
    {
        return;
    }
    d->_themeMode = themeMode;
    Q_EMIT themeModeChanged(themeMode);
    Q_EMIT themeChanged();
}

ShadThemeType::ThemeMode ShadTheme::getThemeMode() const
{
    Q_D(const ShadTheme);
    return d->_themeMode;
}

bool ShadTheme::isDark() const
{
    Q_D(const ShadTheme);
    return d->_themeMode == ShadThemeType::Dark;
}

void ShadTheme::setFollowSystemTheme(bool isFollow)
{
    Q_D(ShadTheme);
    if (d->_isFollowSystemTheme == isFollow)
    {
        return;
    }
    d->_isFollowSystemTheme = isFollow;
    Q_EMIT followSystemThemeChanged(isFollow);
    if (!isFollow)
    {
        return;
    }
    const ShadThemeType::ThemeMode systemMode = ShadThemePrivate::systemThemeMode();
    if (d->_themeMode != systemMode)
    {
        d->_themeMode = systemMode;
        Q_EMIT themeModeChanged(systemMode);
        Q_EMIT themeChanged();
    }
}

bool ShadTheme::isFollowSystemTheme() const
{
    Q_D(const ShadTheme);
    return d->_isFollowSystemTheme;
}

void ShadTheme::setBaseColor(ShadThemeType::BaseColor baseColor)
{
    Q_D(ShadTheme);
    if (d->_baseColor == baseColor)
    {
        return;
    }
    d->applyBaseColor(baseColor);
    Q_EMIT baseColorChanged(baseColor);
    Q_EMIT themeChanged();
}

ShadThemeType::BaseColor ShadTheme::getBaseColor() const
{
    Q_D(const ShadTheme);
    return d->_baseColor;
}

QColor ShadTheme::presetColor(ShadThemeType::BaseColor baseColor, ShadThemeType::ThemeMode themeMode, ShadThemeType::ThemeColor themeColor)
{
    if (themeColor < 0 || themeColor >= ShadThemeType::ColorCount)
    {
        qWarning("ShadTheme::presetColor 收到非法颜色枚举值: %d", static_cast<int>(themeColor));
        return QColor();
    }
    ShadThemePrivate::ColorTable light;
    ShadThemePrivate::ColorTable dark;
    ShadThemePrivate::buildColorTables(baseColor, light, dark);
    return themeMode == ShadThemeType::Dark ? dark[themeColor] : light[themeColor];
}

QColor ShadTheme::getColor(ShadThemeType::ThemeColor themeColor) const
{
    Q_D(const ShadTheme);
    // 底色较透明时灰色文字看不清
    if (themeColor == ShadThemeType::MutedForeground && d->isEffectBackgroundTranslucent())
    {
        return getColor(d->_themeMode, ShadThemeType::Foreground);
    }
    QColor color = getColor(d->_themeMode, themeColor);
    if (ShadThemePrivate::isSurfaceColor(themeColor) && d->isEffectActive())
    {
        color.setAlphaF(static_cast<float>(color.alphaF() * d->_windowEffectSurfaceOpacity));
    }
    return color;
}

QColor ShadTheme::getColor(ShadThemeType::ThemeMode themeMode, ShadThemeType::ThemeColor themeColor) const
{
    Q_D(const ShadTheme);
    if (themeColor < 0 || themeColor >= ShadThemeType::ColorCount)
    {
        qWarning("ShadTheme::getColor 收到非法颜色枚举值: %d", static_cast<int>(themeColor));
        return QColor();
    }
    return d->colorTable(themeMode)[themeColor];
}

void ShadTheme::setColor(ShadThemeType::ThemeMode themeMode, ShadThemeType::ThemeColor themeColor, const QColor& color)
{
    Q_D(ShadTheme);
    if (themeColor < 0 || themeColor >= ShadThemeType::ColorCount)
    {
        qWarning("ShadTheme::setColor 收到非法颜色枚举值: %d", static_cast<int>(themeColor));
        return;
    }
    QColor& target = d->colorTable(themeMode)[themeColor];
    if (target == color)
    {
        return;
    }
    target = color;
    if (themeMode == d->_themeMode)
    {
        Q_EMIT themeChanged();
    }
}

void ShadTheme::setBaseRadius(int radius)
{
    Q_D(ShadTheme);
    radius = qMax(0, radius);
    if (d->_baseRadius == radius)
    {
        return;
    }
    d->_baseRadius = radius;
    Q_EMIT themeChanged();
}

int ShadTheme::getBaseRadius() const
{
    Q_D(const ShadTheme);
    return d->_baseRadius;
}

int ShadTheme::getRadius(ShadThemeType::Radius radius) const
{
    Q_D(const ShadTheme);
    switch (radius)
    {
        case ShadThemeType::Small:
            return qMax(0, d->_baseRadius - 4);
        case ShadThemeType::Medium:
            return qMax(0, d->_baseRadius - 2);
        case ShadThemeType::Large:
            return d->_baseRadius;
        case ShadThemeType::ExtraLarge:
            return d->_baseRadius + 4;
    }
    return d->_baseRadius;
}

void ShadTheme::setPageTransitionEnabled(bool enabled)
{
    Q_D(ShadTheme);
    if (d->_isPageTransitionEnabled == enabled)
    {
        return;
    }
    d->_isPageTransitionEnabled = enabled;
    Q_EMIT pageTransitionChanged();
}

bool ShadTheme::isPageTransitionEnabled() const
{
    Q_D(const ShadTheme);
    return d->_isPageTransitionEnabled;
}

void ShadTheme::setPageTransitionDuration(int msecs)
{
    Q_D(ShadTheme);
    msecs = qBound(0, msecs, 1000);
    if (d->_pageTransitionDuration == msecs)
    {
        return;
    }
    d->_pageTransitionDuration = msecs;
    Q_EMIT pageTransitionChanged();
}

int ShadTheme::getPageTransitionDuration() const
{
    Q_D(const ShadTheme);
    return d->_pageTransitionDuration;
}

void ShadTheme::setPageTransitionStyle(ShadThemeType::PageTransitionStyle style)
{
    Q_D(ShadTheme);
    if (d->_pageTransitionStyle == style)
    {
        return;
    }
    d->_pageTransitionStyle = style;
    Q_EMIT pageTransitionChanged();
}

ShadThemeType::PageTransitionStyle ShadTheme::getPageTransitionStyle() const
{
    Q_D(const ShadTheme);
    return d->_pageTransitionStyle;
}

void ShadTheme::setWindowEffect(ShadWindowType::Effect effect)
{
    Q_D(ShadTheme);
    if (d->_windowEffect == effect)
    {
        return;
    }
    if (!isWindowEffectSupported(effect))
    {
        qWarning("当前平台不支持窗口特效: %d", static_cast<int>(effect));
        return;
    }
    d->_windowEffect = effect;
    Q_EMIT windowEffectChanged(effect);
    Q_EMIT themeChanged();
}

ShadWindowType::Effect ShadTheme::getWindowEffect() const
{
    Q_D(const ShadTheme);
    return d->_windowEffect;
}

bool ShadTheme::isWindowEffectSupported(ShadWindowType::Effect effect)
{
    return ShadWindowEffect::isSupported(effect);
}

void ShadTheme::setWindowEffectOpacity(ShadWindowType::Effect effect, qreal opacity)
{
    Q_D(ShadTheme);
    if (effect <= ShadWindowType::NoEffect || effect >= ShadThemePrivate::kWindowEffectCount)
    {
        return;
    }
    qreal& target = d->_windowEffectOpacities[static_cast<size_t>(effect)];
    opacity = qBound<qreal>(0.0, opacity, 1.0);
    if (qFuzzyCompare(target, opacity))
    {
        return;
    }
    target = opacity;
    if (effect == d->_windowEffect)
    {
        Q_EMIT themeChanged();
    }
}

qreal ShadTheme::getWindowEffectOpacity(ShadWindowType::Effect effect) const
{
    Q_D(const ShadTheme);
    if (effect <= ShadWindowType::NoEffect || effect >= ShadThemePrivate::kWindowEffectCount)
    {
        return 1.0;
    }
    return d->_windowEffectOpacities[static_cast<size_t>(effect)];
}

void ShadTheme::setWindowEffectImage(const QImage& image)
{
    Q_D(ShadTheme);
    d->_windowEffectImage = image;
    ShadWindowEffect::setWallpaperSource(image);
    if (d->_windowEffect == ShadWindowType::Wallpaper)
    {
        Q_EMIT themeChanged();
    }
}

QImage ShadTheme::getWindowEffectImage() const
{
    Q_D(const ShadTheme);
    return d->_windowEffectImage;
}

void ShadTheme::setWindowEffectBlurRadius(int radius)
{
    Q_D(ShadTheme);
    radius = qBound(0, radius, ShadWindowEffect::kMaxBlurRadius);
    if (d->_windowEffectBlurRadius == radius)
    {
        return;
    }
    d->_windowEffectBlurRadius = radius;
    ShadWindowEffect::setBlurRadius(radius);
    if (ShadWindowEffect::isBlurRadiusSupported(d->_windowEffect))
    {
        Q_EMIT themeChanged();
    }
}

int ShadTheme::getWindowEffectBlurRadius() const
{
    Q_D(const ShadTheme);
    return d->_windowEffectBlurRadius;
}

void ShadTheme::setWindowEffectDesktopOnly(bool desktopOnly)
{
    Q_D(ShadTheme);
    if (d->_isWindowEffectDesktopOnly == desktopOnly)
    {
        return;
    }
    d->_isWindowEffectDesktopOnly = desktopOnly;
    if (desktopOnly)
    {
        ShadWindowEffect::requestDesktopCaptureAccess();
    }
    if (d->_windowEffect == ShadWindowType::Blur)
    {
        Q_EMIT themeChanged();
    }
}

bool ShadTheme::isWindowEffectDesktopOnly() const
{
    Q_D(const ShadTheme);
    return d->_isWindowEffectDesktopOnly;
}

void ShadTheme::setWindowEffectSurfaceOpacity(qreal opacity)
{
    Q_D(ShadTheme);
    opacity = qBound<qreal>(0.0, opacity, 1.0);
    if (qFuzzyCompare(d->_windowEffectSurfaceOpacity, opacity))
    {
        return;
    }
    d->_windowEffectSurfaceOpacity = opacity;
    if (d->_windowEffect != ShadWindowType::NoEffect)
    {
        Q_EMIT themeChanged();
    }
}

qreal ShadTheme::getWindowEffectSurfaceOpacity() const
{
    Q_D(const ShadTheme);
    return d->_windowEffectSurfaceOpacity;
}
