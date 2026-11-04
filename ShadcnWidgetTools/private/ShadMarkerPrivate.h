#ifndef SHADMARKERPRIVATE_H
#define SHADMARKERPRIVATE_H

#include <QColor>
#include <QFont>
#include <QList>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QVariantAnimation;
class QWidget;
class ShadMarker;
class ShadTransition;

class ShadMarkerPrivate
{
    SHAD_D_CREATE(ShadMarker)

    public:
        ShadMarkerPrivate();
        ~ShadMarkerPrivate();

        struct Layout
        {
            QRectF icon;
            QList<QRectF> lineRects;
            QStringList lines;
            QRectF leadingRule;
            QRectF trailingRule;
            QRectF border;
            int height{0};
        };

        static constexpr int kFontPixelSize = 14;
        static constexpr int kLineHeight = 20;
        static constexpr int kIconSize = 16;
        static constexpr int kGap = 8;
        static constexpr int kRuleGap = 12;
        static constexpr int kBorderPadding = 8;
        static constexpr int kBorderWidth = 1;
        static constexpr int kDefaultTextWidth = 320;

        bool hasIcon() const;
        QFont textFont() const;
        QColor textColor() const;
        Layout computeGeometry(int width) const;
        QStringList wrapText(qreal width) const;
        int naturalWidth() const;
        void placeIconWidget();
        void updateAnimations();
        void activate();

        ShadMarkerType::Variant _variant{ShadMarkerType::Default};
        QString _text;
        ShadIconType::IconName _icon{ShadIconType::None};
        QPointer<QWidget> _iconWidget;
        bool _isShimmer{false};
        bool _isClickable{false};
        QUrl _url;
        bool _isPressed{false};
        bool _isFocusVisible{false};
        ShadTransition* _hoverTransition{nullptr};
        QVariantAnimation* _shimmerAnimation{nullptr};
        qreal _shimmerProgress{0.0};
        QVariantAnimation* _spinAnimation{nullptr};
        qreal _spinAngle{0.0};
};

#endif // SHADMARKERPRIVATE_H
