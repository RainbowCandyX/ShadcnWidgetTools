#ifndef SHADRADIOGROUPPRIVATE_H
#define SHADRADIOGROUPPRIVATE_H

#include <QtCore/qnamespace.h>

#include "ShadProperty.h"

class QBoxLayout;
class QButtonGroup;
class ShadRadioGroup;

class ShadRadioGroupPrivate
{
    SHAD_D_CREATE(ShadRadioGroup)

    public:
        ShadRadioGroupPrivate();
        ~ShadRadioGroupPrivate();

        void applyOrientation();

        QButtonGroup* _buttonGroup{nullptr};
        QBoxLayout* _layout{nullptr};
        Qt::Orientation _orientation{Qt::Vertical};
};

#endif // SHADRADIOGROUPPRIVATE_H
