#ifndef SHADPASSKEYBUTTONPRIVATE_H
#define SHADPASSKEYBUTTONPRIVATE_H

class ShadPasskey;

class ShadPasskeyButtonPrivate
{
    public:
        ShadPasskeyButtonPrivate();
        ~ShadPasskeyButtonPrivate();

        ShadPasskey* _passkey{nullptr};
};

#endif
