#ifndef SHADSPINNERPRIVATE_H
#define SHADSPINNERPRIVATE_H

#include <QColor>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QVariantAnimation;
class ShadSpinner;

class ShadSpinnerPrivate
{
    SHAD_D_CREATE(ShadSpinner)

    public:
        ShadSpinnerPrivate();
        ~ShadSpinnerPrivate();

        // size-4
        static constexpr int kDefaultSize = 16;

        // 设置为旋转且控件可见时运行动画，否则停止并回到初始角度
        void updateAnimation();
        QColor iconColor() const;

        QVariantAnimation* _animation{nullptr};
        ShadIconType::IconName _icon{ShadIconType::LoaderCircle};
        int _size{kDefaultSize};
        ShadThemeType::ThemeColor _colorRole{ShadThemeType::Foreground};
        QColor _color;
        bool _isRunning{true};
        qreal _angle{0.0};
};

#endif // SHADSPINNERPRIVATE_H
