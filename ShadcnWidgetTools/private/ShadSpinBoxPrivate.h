#ifndef SHADSPINBOXPRIVATE_H
#define SHADSPINBOXPRIVATE_H

#include "ShadProperty.h"

class ShadSpinBox;
class ShadSpinBoxStyle;

class ShadSpinBoxPrivate
{
    SHAD_D_CREATE(ShadSpinBox)

    public:
        ShadSpinBoxPrivate();
        ~ShadSpinBoxPrivate();

        // 父对象为输入框，随输入框释放
        ShadSpinBoxStyle* _style{nullptr};
};

#endif // SHADSPINBOXPRIVATE_H
