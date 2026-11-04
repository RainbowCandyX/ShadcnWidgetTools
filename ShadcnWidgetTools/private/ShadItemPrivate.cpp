#include "ShadItemPrivate.h"

#include <QBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadItem.h"
#include "ShadItemContent.h"
#include "ShadItemMedia.h"
#include "ShadStyleHelper.h"

ShadItemPrivate::ShadItemPrivate()
{
}

ShadItemPrivate::~ShadItemPrivate()
{
}

int ShadItemPrivate::gap(ShadItemType::Size size)
{
    switch (size)
    {
        case ShadItemType::Small:
            return 10;
        case ShadItemType::ExtraSmall:
            return 8;
        case ShadItemType::Medium:
            break;
    }
    return 14;
}

int ShadItemPrivate::horizontalPadding(ShadItemType::Size size)
{
    switch (size)
    {
        case ShadItemType::Small:
            return 12;
        case ShadItemType::ExtraSmall:
            return 10;
        case ShadItemType::Medium:
            break;
    }
    return 16;
}

int ShadItemPrivate::verticalPadding(ShadItemType::Size size)
{
    switch (size)
    {
        case ShadItemType::Small:
            return 10;
        case ShadItemType::ExtraSmall:
            return 8;
        case ShadItemType::Medium:
            break;
    }
    return 14;
}

void ShadItemPrivate::applySize()
{
    // 边距 = 焦点环边距 + 边框 + 内边距
    const int edge = ShadStyleHelper::kFocusRingWidth + kBorderWidth;
    const int horizontal = edge + horizontalPadding(_size);
    const int vertical = edge + verticalPadding(_size);
    _layout->setContentsMargins(horizontal, vertical, horizontal, vertical);
    _layout->setSpacing(gap(_size));
    _rowLayout->setSpacing(gap(_size));
    _content->setItemSize(_size);
    _media->setItemSize(_size);
}

void ShadItemPrivate::updateMediaAlignment()
{
    // group-has-[item-description]:self-start translate-y-0.5
    const bool hasDescription = !_content->getDescription().isEmpty();
    _media->setTopOffset(hasDescription ? kMediaOffset : 0);
    _rowLayout->setAlignment(_media, hasDescription ? Qt::AlignTop : Qt::AlignVCenter);
    _media->setVisible(_media->hasContent());
}

void ShadItemPrivate::replaceEdgeWidget(QPointer<QWidget>& slot, QWidget* widget, bool isHeader)
{
    Q_Q(ShadItem);
    if (slot.data() == widget)
    {
        return;
    }
    if (!slot.isNull())
    {
        QWidget* oldWidget = slot.data();
        _layout->removeWidget(oldWidget);
        ShadStyleHelper::clearFocusWithin(oldWidget);
        oldWidget->hide();
        oldWidget->deleteLater();
    }
    slot = widget;
    if (widget == nullptr)
    {
        return;
    }
    widget->setParent(q);
    // 页眉位于主行之前，页脚位于主行之后
    if (isHeader)
    {
        _layout->insertWidget(0, widget);
    }
    else
    {
        _layout->addWidget(widget);
    }
    widget->show();
}
