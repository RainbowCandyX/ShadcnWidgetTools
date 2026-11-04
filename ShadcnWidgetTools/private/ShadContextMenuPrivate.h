#ifndef SHADCONTEXTMENUPRIVATE_H
#define SHADCONTEXTMENUPRIVATE_H

#include <QList>
#include <QPointer>

#include "ShadProperty.h"

class QObject;
class QWidget;
class ShadContextMenu;

class ShadContextMenuPrivate
{
    SHAD_D_CREATE(ShadContextMenu)

    public:
        ShadContextMenuPrivate();
        ~ShadContextMenuPrivate();

        // 滚动区域的右键事件发给 viewport，返回对应的目标
        QWidget* targetOf(QObject* watched) const;
        void removeTarget(QWidget* target);

        QList<QPointer<QWidget>> _targets;
};

#endif // SHADCONTEXTMENUPRIVATE_H
