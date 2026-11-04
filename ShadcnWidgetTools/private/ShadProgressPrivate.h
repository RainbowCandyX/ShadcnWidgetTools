#ifndef SHADPROGRESSPRIVATE_H
#define SHADPROGRESSPRIVATE_H

#include "ShadProperty.h"

class ShadProgress;
class ShadTransition;

class ShadProgressPrivate
{
    SHAD_D_CREATE(ShadProgress)

    public:
        ShadProgressPrivate();
        ~ShadProgressPrivate();

        int _value{0};
        // 当前显示比例 0 到 1，对应 transition-all
        ShadTransition* _valueTransition{nullptr};
};

#endif // SHADPROGRESSPRIVATE_H
