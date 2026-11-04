#ifndef SHADITEMGROUPPRIVATE_H
#define SHADITEMGROUPPRIVATE_H

#include <QList>

#include "ShadDef.h"
#include "ShadProperty.h"

class QBoxLayout;
class QWidget;
class ShadItemGroup;

class ShadItemGroupPrivate
{
    SHAD_D_CREATE(ShadItemGroup)

    public:
        ShadItemGroupPrivate();
        ~ShadItemGroupPrivate();

        // ItemSeparator 的 my-2
        static constexpr int kSeparatorMargin = 8;

        // gap-4 / gap-2.5 / gap-2
        static int gap(ShadItemType::Size size);
        // 组内最小的列表项尺寸，没有列表项时为 Medium
        ShadItemType::Size smallestSize() const;

        QBoxLayout* _layout{nullptr};
        // 列表项与分隔线，按显示顺序
        QList<QWidget*> _entries;
        QList<QWidget*> _separators;
};

#endif // SHADITEMGROUPPRIVATE_H
