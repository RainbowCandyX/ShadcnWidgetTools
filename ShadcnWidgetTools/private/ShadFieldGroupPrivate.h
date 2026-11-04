#ifndef SHADFIELDGROUPPRIVATE_H
#define SHADFIELDGROUPPRIVATE_H

#include <QList>
#include <QPointer>
#include <QWidget>

#include "ShadProperty.h"

class QBoxLayout;
class ShadFieldGroup;

class ShadFieldGroupPrivate
{
    SHAD_D_CREATE(ShadFieldGroup)

    public:
        ShadFieldGroupPrivate();
        ~ShadFieldGroupPrivate();

        // 分隔线上下收紧的距离，对应 -my-2
        static constexpr int kSeparatorInset = 8;

        // 按当前组件顺序重建布局，组件之间插入间距；分隔线两侧的间距收紧
        void rebuildLayout();

        QBoxLayout* _layout{nullptr};
        QList<QPointer<QWidget>> _widgets;
        int _spacing{28};
};

#endif // SHADFIELDGROUPPRIVATE_H
