#ifndef SHADTOGGLEGROUPPRIVATE_H
#define SHADTOGGLEGROUPPRIVATE_H

#include <QList>

#include "ShadDef.h"

class ShadToggle;
class ShadToggleGroup;

class ShadToggleGroupPrivate
{
    SHAD_D_CREATE(ShadToggleGroup)

    public:
        ShadToggleGroupPrivate();
        ~ShadToggleGroupPrivate();

        QList<ShadToggle*> visibleItems() const;
        // 相邻按钮重叠的像素：两侧焦点环边距，outline 变体再加 1px 共用边框
        int itemOverlap() const;
        void onItemToggled(ShadToggle* item, bool checked);
        // 批量修改选中状态，结束后统一发出信号
        void applyCheckedStates(const QList<bool>& states);

        QList<ShadToggle*> _items;
        ShadToggleGroupType::SelectionMode _selectionMode{ShadToggleGroupType::Single};
        ShadToggleType::Variant _variant{ShadToggleType::Default};
        ShadToggleType::Size _size{ShadToggleType::Medium};
        // 内部批量修改选中状态时为 true，屏蔽逐项的信号处理
        bool _isUpdating{false};
        int _lastCurrentIndex{-1};
};

#endif // SHADTOGGLEGROUPPRIVATE_H
