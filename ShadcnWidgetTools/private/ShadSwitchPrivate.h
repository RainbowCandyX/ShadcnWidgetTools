#ifndef SHADSWITCHPRIVATE_H
#define SHADSWITCHPRIVATE_H

#include "ShadProperty.h"

class ShadSwitch;
class ShadTransition;

class ShadSwitchPrivate
{
    SHAD_D_CREATE(ShadSwitch)

    public:
        ShadSwitchPrivate();
        ~ShadSwitchPrivate();

        bool _isFocusVisible{false};
        // 0 为关闭位置，1 为打开位置
        ShadTransition* _checkTransition{nullptr};
};

#endif // SHADSWITCHPRIVATE_H
