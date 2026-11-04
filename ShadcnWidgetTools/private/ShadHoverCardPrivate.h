#ifndef SHADHOVERCARDPRIVATE_H
#define SHADHOVERCARDPRIVATE_H

#include <QPointer>
#include <QRect>
#include <QSize>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadProperty.h"

class QTimer;
class QVBoxLayout;
class QVariantAnimation;
class QWidget;
class ShadHoverCard;

class ShadHoverCardPrivate
{
    SHAD_D_CREATE(ShadHoverCard)

    public:
        ShadHoverCardPrivate();
        ~ShadHoverCardPrivate();

        // 留给 shadow-md 与位移动画的透明边距
        static constexpr int kShadowMargin = 12;
        // w-64 p-4 border，sideOffset=4
        static constexpr int kBorderWidth = 1;
        static constexpr int kDefaultWidth = 256;
        static constexpr int kDefaultPadding = 16;
        static constexpr int kSideOffset = 4;
        static constexpr int kDefaultOpenDelay = 700;
        static constexpr int kDefaultCloseDelay = 300;

        QSize cardSize() const;
        QRect cardRect() const;
        void reposition();
        void layoutCard();
        void scheduleOpen();
        void scheduleClose();

        QWidget* _card{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        QPointer<QWidget> _trigger;
        ShadPlacementType::Side _side{ShadPlacementType::Bottom};
        ShadPlacementType::Align _align{ShadPlacementType::Center};
        ShadPlacementType::Side _actualSide{ShadPlacementType::Bottom};
        int _contentWidth{kDefaultWidth};
        int _padding{kDefaultPadding};
        int _openDelay{kDefaultOpenDelay};
        int _closeDelay{kDefaultCloseDelay};
        QTimer* _openTimer{nullptr};
        QTimer* _closeTimer{nullptr};
        QVariantAnimation* _openAnimation{nullptr};
        qreal _openProgress{1.0};
};

#endif // SHADHOVERCARDPRIVATE_H
