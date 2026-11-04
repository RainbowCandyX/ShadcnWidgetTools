#include "ShadTogglePrivate.h"

#include <QIcon>
#include <QRect>
#include <QRegion>

#include "ShadStyleHelper.h"
#include "ShadToggle.h"

ShadTogglePrivate::ShadTogglePrivate()
{
}

ShadTogglePrivate::~ShadTogglePrivate()
{
}

ShadTogglePrivate::SizeMetrics ShadTogglePrivate::sizeMetrics(ShadToggleType::Size size)
{
    // 尺寸对应 default: h-9 px-2 min-w-9 / sm: h-8 px-1.5 min-w-8 / lg: h-10 px-2.5 min-w-10
    switch (size)
    {
        case ShadToggleType::Small:
            return {32, 6, 32};
        case ShadToggleType::Large:
            return {40, 10, 40};
        case ShadToggleType::Medium:
            break;
    }
    return {36, 8, 36};
}

bool ShadTogglePrivate::hasIcon() const
{
    Q_Q(const ShadToggle);
    return _shadIcon != ShadIconType::None || !q->icon().isNull();
}

void ShadTogglePrivate::updateGroupMask()
{
    Q_Q(ShadToggle);
    if (_groupPosition == GroupPosition::Standalone)
    {
        q->clearMask();
        return;
    }
    if (q->hasFocus() && _isFocusVisible)
    {
        q->clearMask();
        q->raise();
        return;
    }
    const int margin = ShadStyleHelper::kFocusRingWidth;
    q->setMask(QRegion(q->rect().adjusted(margin, margin, -margin, -margin)));
}
