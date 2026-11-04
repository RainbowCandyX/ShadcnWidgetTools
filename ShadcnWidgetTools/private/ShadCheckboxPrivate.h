#ifndef SHADCHECKBOXPRIVATE_H
#define SHADCHECKBOXPRIVATE_H

#include "ShadProperty.h"

class ShadCheckbox;

class ShadCheckboxPrivate
{
    SHAD_D_CREATE(ShadCheckbox)

    public:
        ShadCheckboxPrivate();
        ~ShadCheckboxPrivate();

        bool _isInvalid{false};
        bool _isFocusVisible{false};
};

#endif // SHADCHECKBOXPRIVATE_H
