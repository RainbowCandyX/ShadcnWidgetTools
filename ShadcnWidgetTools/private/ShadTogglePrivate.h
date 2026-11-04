#ifndef SHADTOGGLEPRIVATE_H
#define SHADTOGGLEPRIVATE_H

#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadIconType.h"

class ShadToggle;
class ShadTransition;

class ShadTogglePrivate
{
    SHAD_D_CREATE(ShadToggle)

    public:
        ShadTogglePrivate();
        ~ShadTogglePrivate();

        // 各尺寸的可视高度、内边距与最小宽度（不含焦点环边距）
        struct SizeMetrics
        {
            int height;
            int paddingX;
            int minWidth;
        };

        // 在 ShadToggleGroup 中的位置，决定圆角与边框
        enum class GroupPosition
        {
            Standalone,
            First,
            Middle,
            Last,
        };

        static SizeMetrics sizeMetrics(ShadToggleType::Size size);
        bool hasIcon() const;
        // 组内按钮互相重叠焦点环边距：未显示焦点环时遮罩只保留按钮主体，避免盖住相邻按钮的点击区域；
        // 显示焦点环时去掉遮罩并置顶（对应 focus-visible:z-10）
        void updateGroupMask();

        ShadToggleType::Variant _variant{ShadToggleType::Default};
        ShadToggleType::Size _size{ShadToggleType::Medium};
        ShadIconType::IconName _shadIcon{ShadIconType::None};
        Qt::Alignment _contentAlignment{Qt::AlignCenter};
        bool _isFocusVisible{false};
        GroupPosition _groupPosition{GroupPosition::Standalone};
        ShadTransition* _hoverTransition{nullptr};
};

#endif // SHADTOGGLEPRIVATE_H
