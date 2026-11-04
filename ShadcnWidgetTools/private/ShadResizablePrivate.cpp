#include "ShadResizablePrivate.h"

#include <QSplitterHandle>

#include "ShadResizable.h"

ShadResizablePrivate::ShadResizablePrivate()
{
}

ShadResizablePrivate::~ShadResizablePrivate()
{
}

void ShadResizablePrivate::applyHandleWidth()
{
    Q_Q(ShadResizable);
    q->setHandleWidth(_isGripVisible ? kGripHandleWidth : kHandleWidth);
    for (int i = 0; i < q->count(); ++i)
    {
        if (QSplitterHandle* splitterHandle = q->handle(i))
        {
            splitterHandle->update();
        }
    }
}
