#ifndef SHADINPUTGROUPPRIVATE_H
#define SHADINPUTGROUPPRIVATE_H

#include <QList>
#include <QPointer>
#include <QSize>

#include "ShadDef.h"
#include "ShadProperty.h"

class QWidget;
class ShadInputGroup;

class ShadInputGroupPrivate
{
    SHAD_D_CREATE(ShadInputGroup)

    public:
        ShadInputGroupPrivate();
        ~ShadInputGroupPrivate();

        // 附加区中的一项：组件或弹性空白
        struct Addon
        {
            ShadInputGroupType::AddonPosition position{ShadInputGroupType::InlineStart};
            QPointer<QWidget> widget;
            bool isStretch{false};
        };

        // h-9 外框，减去上下 1px 边框后单行输入框的高度
        static constexpr int kInlineHeight = 34;
        // 有上下附加区时输入框保持 h-9
        static constexpr int kBlockInputHeight = 36;
        // 附加区内边距 px-3、项间距 gap-2；含按钮时向外收 0.45rem（约 7px）
        static constexpr int kAddonPadding = 12;
        static constexpr int kAddonGap = 8;
        static constexpr int kButtonPullIn = 7;
        // 有行内附加区时输入框在该侧的内边距 pl-2 / pr-2，没有时为 px-3
        static constexpr int kInputPaddingNearAddon = 8;
        // 上下附加区与外框的距离：单行输入框 pt-2.5 / pb-2.5，多行 pt-3 / pb-3
        static constexpr int kBlockPaddingLine = 10;
        static constexpr int kBlockPaddingText = 12;

        QWidget* control() const;
        bool isLineEdit() const;
        // 某个附加区中仍然存在且未隐藏的项（弹性空白保留）
        QList<Addon> addons(ShadInputGroupType::AddonPosition position) const;
        bool hasAddons(ShadInputGroupType::AddonPosition position) const;
        // 附加区一项的可视尺寸（按钮等可聚焦组件扣除焦点环边距）
        static QSize itemSize(const QWidget* widget);
        static bool hasFocusMargin(const QWidget* widget);
        // 附加区一行的高度
        int rowHeight(ShadInputGroupType::AddonPosition position) const;
        // 一行附加区的总宽度（不含弹性空白）
        int rowWidth(ShadInputGroupType::AddonPosition position) const;
        int controlRowHeight() const;
        int blockPadding() const;
        // 外框内部（不含边框）的高度
        int innerHeight() const;
        void relayout();
        // widget 为空时添加弹性空白
        void addAddon(ShadInputGroupType::AddonPosition position, QWidget* widget);
        void pruneAddons();

        ShadInputGroupType::Control _controlType{ShadInputGroupType::LineEdit};
        QPointer<QWidget> _control;
        QList<Addon> _addons;
        bool _isInvalid{false};
};

#endif // SHADINPUTGROUPPRIVATE_H
