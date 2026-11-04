#ifndef SHADAVATARGROUPPRIVATE_H
#define SHADAVATARGROUPPRIVATE_H

#include <QList>

#include "ShadDef.h"
#include "ShadProperty.h"

class ShadAvatar;
class ShadAvatarGroup;
class ShadAvatarGroupCount;

class ShadAvatarGroupPrivate
{
    SHAD_D_CREATE(ShadAvatarGroup)

    public:
        ShadAvatarGroupPrivate();
        ~ShadAvatarGroupPrivate();

        // 相邻头像的重叠宽度，对应 -space-x-2
        static constexpr int kOverlap = 8;

        // 设置头像在组内的外观：统一尺寸并显示 ring；移出头像组时恢复
        void attachAvatar(ShadAvatar* avatar);
        static void detachAvatar(ShadAvatar* avatar);
        // 计数文字或图标为空时隐藏计数
        void updateCountVisibility();
        // 头像与计数的数量（计数可见时计入）
        int itemCount() const;
        // 按顺序放置头像与计数，后面的盖在前面的上方
        void updateLayout();

        QList<ShadAvatar*> _avatars;
        ShadAvatarGroupCount* _count{nullptr};
        ShadAvatarType::Size _size{ShadAvatarType::Medium};
};

#endif // SHADAVATARGROUPPRIVATE_H
