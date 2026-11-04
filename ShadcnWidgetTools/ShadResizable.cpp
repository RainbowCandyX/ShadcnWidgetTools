#include "ShadResizable.h"

#include <QSplitterHandle>

#include "ShadResizableHandle.h"
#include "ShadResizablePrivate.h"

ShadResizable::ShadResizable(QWidget* parent)
    : ShadResizable(Qt::Horizontal, parent)
{
}

ShadResizable::ShadResizable(Qt::Orientation orientation, QWidget* parent)
    : QSplitter(orientation, parent), d_ptr(new ShadResizablePrivate())
{
    Q_D(ShadResizable);
    d->q_ptr = this;
    setChildrenCollapsible(false);
    // 拖动时实时调整面板大小
    setOpaqueResize(true);
    d->applyHandleWidth();
}

ShadResizable::~ShadResizable()
{
}

void ShadResizable::setGripVisible(bool visible)
{
    Q_D(ShadResizable);
    if (d->_isGripVisible == visible)
    {
        return;
    }
    d->_isGripVisible = visible;
    d->applyHandleWidth();
}

bool ShadResizable::isGripVisible() const
{
    Q_D(const ShadResizable);
    return d->_isGripVisible;
}

QSplitterHandle* ShadResizable::createHandle()
{
    return new ShadResizableHandle(orientation(), this);
}
