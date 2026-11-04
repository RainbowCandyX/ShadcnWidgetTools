#ifndef SHADRADIALPROGRESSPRIVATE_H
#define SHADRADIALPROGRESSPRIVATE_H

#include <QColor>
#include <QString>

#include "ShadDef.h"
#include "ShadProperty.h"

class ShadRadialProgress;
class ShadTransition;

class ShadRadialProgressPrivate
{
    SHAD_D_CREATE(ShadRadialProgress)

    public:
        ShadRadialProgressPrivate();
        ~ShadRadialProgressPrivate();

        QColor statusColor() const;
        QString displayText() const;

        int _value{0};
        ShadRadialProgressType::Variant _variant{ShadRadialProgressType::Circle};
        ShadRadialProgressType::Status _status{ShadRadialProgressType::Default};
        int _diameter{120};
        int _thickness{10};
        bool _textVisible{true};
        QString _text;
        ShadTransition* _valueTransition{nullptr};
};

#endif
