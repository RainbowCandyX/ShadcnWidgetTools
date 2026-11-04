#ifndef SHADITEMPRIVATE_H
#define SHADITEMPRIVATE_H

#include <QPointer>

#include "ShadDef.h"
#include "ShadProperty.h"

class QBoxLayout;
class QHBoxLayout;
class QWidget;
class ShadItem;
class ShadItemContent;
class ShadItemMedia;
class ShadTransition;

class ShadItemPrivate
{
    SHAD_D_CREATE(ShadItem)

    public:
        ShadItemPrivate();
        ~ShadItemPrivate();

        // border
        static constexpr int kBorderWidth = 1;
        // ItemActions 的 gap-2
        static constexpr int kActionGap = 8;
        // 有描述时媒体区的 translate-y-0.5
        static constexpr int kMediaOffset = 2;
        // transition-colors duration-100
        static constexpr int kHoverDurationMs = 100;

        // gap-3.5 / gap-2.5 / gap-2
        static int gap(ShadItemType::Size size);
        // px-4 / px-3 / px-2.5
        static int horizontalPadding(ShadItemType::Size size);
        // py-3.5 / py-2.5 / py-2
        static int verticalPadding(ShadItemType::Size size);

        // 按尺寸设置边距、间距与子组件尺寸
        void applySize();
        // 媒体区有内容时显示；有描述时媒体区顶部对齐并下移，否则垂直居中
        void updateMediaAlignment();
        // 替换页眉或页脚：旧组件被删除，新组件插入到主行之前或之后
        void replaceEdgeWidget(QPointer<QWidget>& slot, QWidget* widget, bool isHeader);

        QBoxLayout* _layout{nullptr};
        QHBoxLayout* _rowLayout{nullptr};
        QHBoxLayout* _actionsLayout{nullptr};
        ShadItemMedia* _media{nullptr};
        ShadItemContent* _content{nullptr};
        QPointer<QWidget> _header;
        QPointer<QWidget> _footer;
        ShadTransition* _hoverTransition{nullptr};
        ShadItemType::Variant _variant{ShadItemType::Default};
        ShadItemType::Size _size{ShadItemType::Medium};
        bool _isClickable{false};
        bool _isPressed{false};
        bool _isFocusVisible{false};
};

#endif // SHADITEMPRIVATE_H
