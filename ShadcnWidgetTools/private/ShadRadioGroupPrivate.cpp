#include "ShadRadioGroupPrivate.h"

#include <QBoxLayout>

#include "ShadStyleHelper.h"

ShadRadioGroupPrivate::ShadRadioGroupPrivate()
{
}

ShadRadioGroupPrivate::~ShadRadioGroupPrivate()
{
}

void ShadRadioGroupPrivate::applyOrientation()
{
    // 每个选项四周已预留焦点环边距，间距需扣除两侧边距：纵向 gap-3，横向 gap-4
    const int ringMargins = 2 * ShadStyleHelper::kFocusRingWidth;
    if (_orientation == Qt::Horizontal)
    {
        _layout->setDirection(QBoxLayout::LeftToRight);
        _layout->setSpacing(16 - ringMargins);
    }
    else
    {
        _layout->setDirection(QBoxLayout::TopToBottom);
        _layout->setSpacing(12 - ringMargins);
    }
}
