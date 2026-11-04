#ifndef SHADBUTTONPRIVATE_H
#define SHADBUTTONPRIVATE_H

#include <QColor>
#include <QFont>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadIconType.h"

class QVariantAnimation;
class ShadButton;
class ShadTransition;

class ShadButtonPrivate
{
    SHAD_D_CREATE(ShadButton)

    public:
        ShadButtonPrivate();
        ~ShadButtonPrivate();

        // 各尺寸的可视高度与内边距（不含焦点环边距）
        struct SizeMetrics
        {
            int height;
            int paddingX;
            int paddingXWithIcon;
            int gap;
            bool iconOnly;
            // 图标边长，对应 [&_svg]:size-*
            int iconSize;
            // 文字像素大小，0 表示使用组件字体（text-sm）
            int fontPixelSize;
        };

        // 各变体在当前主题下的配色
        struct VariantColors
        {
            QColor background;
            QColor hoverBackground;
            QColor text;
            QColor hoverText;
            QColor border;
            bool hasShadow;
            bool underlineOnHover;
        };

        static SizeMetrics sizeMetrics(ShadButtonType::Size size);
        // 当前尺寸下绘制文字使用的字体
        QFont contentFont() const;
        static VariantColors variantColors(ShadButtonType::Variant variant);
        // 加载中视为有图标（显示加载图标）
        bool hasIcon() const;
        // 当前应绘制的 Lucide 图标：加载中为 LoaderCircle
        ShadIconType::IconName displayIcon() const;
        // 加载中的键盘点击键：Space / Enter / Return / Select
        static bool isActivationKey(int key);

        ShadButtonType::Variant _variant{ShadButtonType::Default};
        ShadButtonType::Size _size{ShadButtonType::Medium};
        ShadIconType::IconName _shadIcon{ShadIconType::None};
        Qt::Alignment _contentAlignment{Qt::AlignCenter};
        bool _isFocusVisible{false};
        bool _isLoading{false};
        bool _isRoundedFull{false};
        // 加载图标旋转角度，对应 animate-spin（1s 匀速一圈）
        qreal _spinAngle{0.0};
        ShadTransition* _hoverTransition{nullptr};
        QVariantAnimation* _spinAnimation{nullptr};
};

#endif // SHADBUTTONPRIVATE_H
