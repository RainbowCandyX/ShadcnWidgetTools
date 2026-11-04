#ifndef SHADTHEME_H
#define SHADTHEME_H

#include <QColor>
#include <QImage>
#include <QObject>

#include "ShadDef.h"
#include "ShadProperty.h"

#define shadTheme ShadTheme::getInstance()

class ShadThemePrivate;

class SHADCN_EXPORT ShadTheme : public QObject
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTheme)
    Q_PROPERTY(ShadThemeType::ThemeMode themeMode READ getThemeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(ShadThemeType::BaseColor baseColor READ getBaseColor WRITE setBaseColor NOTIFY baseColorChanged)
    Q_PROPERTY(bool followSystemTheme READ isFollowSystemTheme WRITE setFollowSystemTheme NOTIFY followSystemThemeChanged)
    Q_PROPERTY(int baseRadius READ getBaseRadius WRITE setBaseRadius NOTIFY themeChanged)
    Q_PROPERTY(bool pageTransitionEnabled READ isPageTransitionEnabled WRITE setPageTransitionEnabled NOTIFY pageTransitionChanged)
    Q_PROPERTY(int pageTransitionDuration READ getPageTransitionDuration WRITE setPageTransitionDuration NOTIFY pageTransitionChanged)
    Q_PROPERTY(ShadThemeType::PageTransitionStyle pageTransitionStyle READ getPageTransitionStyle WRITE setPageTransitionStyle NOTIFY pageTransitionChanged)
    Q_PROPERTY(ShadWindowType::Effect windowEffect READ getWindowEffect WRITE setWindowEffect NOTIFY windowEffectChanged)

    public:
        static ShadTheme* getInstance();

        // 手动设置主题模式会关闭跟随系统
        void setThemeMode(ShadThemeType::ThemeMode themeMode);
        ShadThemeType::ThemeMode getThemeMode() const;
        bool isDark() const;

        void setFollowSystemTheme(bool isFollow);
        bool isFollowSystemTheme() const;

        // 切换基色会覆盖之前通过 setColor 设置的颜色
        void setBaseColor(ShadThemeType::BaseColor baseColor);
        ShadThemeType::BaseColor getBaseColor() const;

        // 不含通过 setColor 设置的自定义颜色
        static QColor presetColor(ShadThemeType::BaseColor baseColor, ShadThemeType::ThemeMode themeMode, ShadThemeType::ThemeColor themeColor);

        // 开启窗口特效时底色与边框随组件不透明度变化
        QColor getColor(ShadThemeType::ThemeColor themeColor) const;
        // 不受窗口特效影响
        QColor getColor(ShadThemeType::ThemeMode themeMode, ShadThemeType::ThemeColor themeColor) const;
        void setColor(ShadThemeType::ThemeMode themeMode, ShadThemeType::ThemeColor themeColor, const QColor& color);

        void setBaseRadius(int radius);
        int getBaseRadius() const;
        int getRadius(ShadThemeType::Radius radius) const;

        void setPageTransitionEnabled(bool enabled);
        bool isPageTransitionEnabled() const;
        void setPageTransitionDuration(int msecs);
        int getPageTransitionDuration() const;
        void setPageTransitionStyle(ShadThemeType::PageTransitionStyle style);
        ShadThemeType::PageTransitionStyle getPageTransitionStyle() const;

        // 当前平台不支持的特效会被忽略
        void setWindowEffect(ShadWindowType::Effect effect);
        ShadWindowType::Effect getWindowEffect() const;
        static bool isWindowEffectSupported(ShadWindowType::Effect effect);
        // 窗口底色的不透明度，每种特效单独保存
        void setWindowEffectOpacity(ShadWindowType::Effect effect, qreal opacity);
        qreal getWindowEffectOpacity(ShadWindowType::Effect effect) const;
        // 空图片表示使用系统壁纸
        void setWindowEffectImage(const QImage& image);
        QImage getWindowEffectImage() const;
        // 范围 0 到 200，0 为不模糊
        void setWindowEffectBlurRadius(int radius);
        int getWindowEffectBlurRadius() const;
        void setWindowEffectDesktopOnly(bool desktopOnly);
        bool isWindowEffectDesktopOnly() const;
        // 组件底色的不透明度
        void setWindowEffectSurfaceOpacity(qreal opacity);
        qreal getWindowEffectSurfaceOpacity() const;

    Q_SIGNALS:
        void themeModeChanged(ShadThemeType::ThemeMode themeMode);
        void baseColorChanged(ShadThemeType::BaseColor baseColor);
        void followSystemThemeChanged(bool isFollow);
        void themeChanged();
        void pageTransitionChanged();
        void windowEffectChanged(ShadWindowType::Effect effect);

    private:
        explicit ShadTheme(QObject* parent = nullptr);
        ~ShadTheme() override;
};

#endif // SHADTHEME_H
