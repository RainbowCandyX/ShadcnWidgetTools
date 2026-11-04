#include "ShadFieldGroupPrivate.h"

#include <QBoxLayout>
#include <QLayoutItem>
#include <QtGlobal>

#include "ShadFieldGroup.h"
#include "ShadFieldSeparator.h"

ShadFieldGroupPrivate::ShadFieldGroupPrivate()
{
}

ShadFieldGroupPrivate::~ShadFieldGroupPrivate()
{
}

void ShadFieldGroupPrivate::rebuildLayout()
{
    // 先取出全部布局项；删除布局项不会删除其中的组件
    while (QLayoutItem* item = _layout->takeAt(0))
    {
        delete item;
    }
    QWidget* previous = nullptr;
    for (const QPointer<QWidget>& widget : _widgets)
    {
        if (widget.isNull())
        {
            continue;
        }
        if (previous != nullptr)
        {
            const bool isNextToSeparator = qobject_cast<ShadFieldSeparator*>(previous) != nullptr || qobject_cast<ShadFieldSeparator*>(widget.data()) != nullptr;
            _layout->addSpacing(qMax(0, _spacing - (isNextToSeparator ? kSeparatorInset : 0)));
        }
        _layout->addWidget(widget.data());
        previous = widget.data();
    }
}
