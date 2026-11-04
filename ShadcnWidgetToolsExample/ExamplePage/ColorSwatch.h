#ifndef COLORSWATCH_H
#define COLORSWATCH_H

#include <QColor>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"

class QPaintEvent;

// 颜色色块：展示一个主题颜色 token
// 默认读取当前主题的颜色；调用 setPresetBaseColor 后改为读取该基色在当前明暗模式下的预设颜色
class ColorSwatch : public QWidget
{
    public:
        // 紧凑模式只显示色块，名称与色值放在悬停提示中
        ColorSwatch(ShadThemeType::ThemeColor themeColor, bool isCompact, QWidget* parent = nullptr);
        ~ColorSwatch() override;

        void setPresetBaseColor(ShadThemeType::BaseColor baseColor);

        // token 对应的 CSS 变量名，例如 muted-foreground
        static QString tokenName(ShadThemeType::ThemeColor themeColor);
        static QString colorText(const QColor& color);

        QSize sizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        QColor currentColor() const;
        void updateToolTip();

        ShadThemeType::ThemeColor _themeColor;
        bool _isCompact{false};
        bool _hasPresetBaseColor{false};
        ShadThemeType::BaseColor _presetBaseColor{ShadThemeType::Neutral};
};

#endif // COLORSWATCH_H
