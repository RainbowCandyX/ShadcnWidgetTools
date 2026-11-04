#ifndef SHADFIELDSEPARATORPRIVATE_H
#define SHADFIELDSEPARATORPRIVATE_H

#include <QString>

#include "ShadProperty.h"

class ShadFieldSeparator;

class ShadFieldSeparatorPrivate
{
    SHAD_D_CREATE(ShadFieldSeparator)

    public:
        ShadFieldSeparatorPrivate();
        ~ShadFieldSeparatorPrivate();

        QString _text;
};

#endif // SHADFIELDSEPARATORPRIVATE_H
