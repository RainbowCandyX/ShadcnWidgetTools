#ifndef SHADSKELETONPRIVATE_H
#define SHADSKELETONPRIVATE_H

#include <QtGlobal>

#include "ShadProperty.h"

class ShadSkeleton;

class ShadSkeletonPrivate
{
    SHAD_D_CREATE(ShadSkeleton)

    public:
        ShadSkeletonPrivate();
        ~ShadSkeletonPrivate();

        void updateAnimation();
        qreal pulseOpacity() const;

        int _radius{-1};
        bool _isCircular{false};
        bool _isAnimated{true};
        bool _isPulsing{false};
};

#endif // SHADSKELETONPRIVATE_H
