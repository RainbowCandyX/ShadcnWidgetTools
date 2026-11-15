#ifndef SHADMARQUEEPRIVATE_H
#define SHADMARQUEEPRIVATE_H

#include <QString>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadProperty.h"

class QVariantAnimation;
class ShadMarquee;

class ShadMarqueePrivate
{
    SHAD_D_CREATE(ShadMarquee)

    public:
        ShadMarqueePrivate();
        ~ShadMarqueePrivate();

        static constexpr int kDefaultSpeed = 40;
        static constexpr int kDefaultGap = 32;

        void updateTextWidth();
        void configureAnimation();
        void updateAnimation();
        bool isScrollable() const;

        QVariantAnimation* _animation{nullptr};
        QString _text;
        int _speed{kDefaultSpeed};
        int _gap{kDefaultGap};
        int _fadeWidth{0};
        ShadThemeType::ThemeColor _colorRole{ShadThemeType::Foreground};
        bool _isReversed{false};
        bool _isPauseOnHover{false};
        bool _isOverflowOnly{false};
        bool _isRunning{true};
        bool _isHovered{false};
        int _textWidth{0};
        qreal _offset{0.0};
};

#endif // SHADMARQUEEPRIVATE_H
