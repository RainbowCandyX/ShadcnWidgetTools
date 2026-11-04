#ifndef SHADSEPARATORPRIVATE_H
#define SHADSEPARATORPRIVATE_H

#include <QtCore/qnamespace.h>

#include "ShadProperty.h"

class ShadSeparator;

class ShadSeparatorPrivate
{
    SHAD_D_CREATE(ShadSeparator)

    public:
        ShadSeparatorPrivate();
        ~ShadSeparatorPrivate();

        Qt::Orientation _orientation{Qt::Horizontal};
};

#endif // SHADSEPARATORPRIVATE_H
