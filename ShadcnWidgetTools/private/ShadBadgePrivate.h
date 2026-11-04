#ifndef SHADBADGEPRIVATE_H
#define SHADBADGEPRIVATE_H

#include <QString>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"

class QVariantAnimation;
class ShadBadge;

class ShadBadgePrivate
{
    SHAD_D_CREATE(ShadBadge)

    public:
        ShadBadgePrivate();
        ~ShadBadgePrivate();

        // 图标为 LoaderCircle 且徽章可见时旋转（对应 Spinner），其余情况停止并回到初始角度
        void updateSpin();

        QString _text;
        ShadBadgeType::Variant _variant{ShadBadgeType::Default};
        ShadIconType::IconName _shadIcon{ShadIconType::None};
        // 第一次需要旋转时创建
        QVariantAnimation* _spinAnimation{nullptr};
        qreal _spinAngle{0.0};
};

#endif // SHADBADGEPRIVATE_H
