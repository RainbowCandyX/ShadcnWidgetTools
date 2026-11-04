#include "ShadRadialProgressPrivate.h"

#include <QColor>
#include <QString>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadRadialProgress.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

ShadRadialProgressPrivate::ShadRadialProgressPrivate()
{
}

ShadRadialProgressPrivate::~ShadRadialProgressPrivate()
{
}

QColor ShadRadialProgressPrivate::statusColor() const
{
    const bool isDark = shadTheme->isDark();
    switch (_status)
    {
        case ShadRadialProgressType::Success:
            return isDark ? QColor(0x4a, 0xde, 0x80) : QColor(0x16, 0xa3, 0x4a);
        case ShadRadialProgressType::Warning:
            return isDark ? QColor(0xfb, 0xbf, 0x24) : QColor(0xd9, 0x77, 0x06);
        case ShadRadialProgressType::Destructive:
            return shadTheme->getColor(ShadThemeType::Destructive);
        case ShadRadialProgressType::Default:
            break;
    }
    return shadTheme->getColor(ShadThemeType::Primary);
}

QString ShadRadialProgressPrivate::displayText() const
{
    if (!_text.isEmpty())
    {
        return _text;
    }
    const int percent = qRound(qBound<qreal>(0.0, _valueTransition->value(), 1.0) * 100.0);
    return QString::number(percent) + QStringLiteral("%");
}
