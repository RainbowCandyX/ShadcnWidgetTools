#include "ShadLabelPrivate.h"

#include <QColor>
#include <QPalette>

#include <initializer_list>

#include "ShadLabel.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadLabelPrivate::ShadLabelPrivate()
{
}

ShadLabelPrivate::~ShadLabelPrivate()
{
}

void ShadLabelPrivate::applyThemeColors()
{
    Q_Q(ShadLabel);
    using ShadStyleHelper::kDisabledOpacity;
    using ShadStyleHelper::withAlpha;
    const QColor color = shadTheme->getColor(_colorRole);
    // buddy 被禁用时所有状态都使用半透明颜色
    const bool isBuddyDisabled = !_observedBuddy.isNull() && !_observedBuddy->isEnabled();
    QPalette palette = q->palette();
    for (QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled})
    {
        const bool isDimmed = isBuddyDisabled || group == QPalette::Disabled;
        const QColor groupColor = isDimmed ? withAlpha(color, kDisabledOpacity) : color;
        palette.setColor(group, QPalette::WindowText, groupColor);
        palette.setColor(group, QPalette::Text, groupColor);
    }
    q->setPalette(palette);
}
