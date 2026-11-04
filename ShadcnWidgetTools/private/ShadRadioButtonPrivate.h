#ifndef SHADRADIOBUTTONPRIVATE_H
#define SHADRADIOBUTTONPRIVATE_H

#include "ShadProperty.h"

class ShadRadioButton;

class ShadRadioButtonPrivate
{
    SHAD_D_CREATE(ShadRadioButton)

    public:
        ShadRadioButtonPrivate();
        ~ShadRadioButtonPrivate();

        bool _isFocusVisible{false};
};

#endif // SHADRADIOBUTTONPRIVATE_H
