#ifndef SHADDOUBLESPINBOXPRIVATE_H
#define SHADDOUBLESPINBOXPRIVATE_H

#include "ShadProperty.h"

class ShadDoubleSpinBox;
class ShadSpinBoxStyle;

class ShadDoubleSpinBoxPrivate
{
    SHAD_D_CREATE(ShadDoubleSpinBox)

    public:
        ShadDoubleSpinBoxPrivate();
        ~ShadDoubleSpinBoxPrivate();

        // 父对象为输入框，随输入框释放
        ShadSpinBoxStyle* _style{nullptr};
};

#endif // SHADDOUBLESPINBOXPRIVATE_H
