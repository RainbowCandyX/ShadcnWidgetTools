#ifndef SHADPOPOVERPRIVATE_H
#define SHADPOPOVERPRIVATE_H

#include <QPointer>
#include <QRect>
#include <QSize>

#include "ShadDef.h"
#include "ShadProperty.h"

class QVBoxLayout;
class QVariantAnimation;
class QWidget;
class ShadLabel;
class ShadPopover;

class ShadPopoverPrivate
{
    SHAD_D_CREATE(ShadPopover)

    public:
        ShadPopoverPrivate();
        ~ShadPopoverPrivate();

        // 窗口四周留给 shadow-md 与位移动画的透明边距
        static constexpr int kShadowMargin = 12;
        // 1px 边框 + p-4
        static constexpr int kBorderWidth = 1;
        static constexpr int kDefaultPadding = 16;
        // sideOffset=4
        static constexpr int kSideOffset = 4;
        // 默认宽度 w-72
        static constexpr int kDefaultWidth = 288;

        // 卡片尺寸（含边框与内边距）
        QSize cardSize() const;
        // 卡片在窗口中的区域（含打开动画的位移）
        QRect cardRect() const;
        // 按锚点重新计算窗口位置与尺寸
        void reposition();
        // 卡片内容组件跟随卡片区域
        void layoutCard();
        void updateHeader();
        // 打开时把焦点交给第一个可通过 Tab 聚焦的子组件
        void focusFirstChild();

        QWidget* _card{nullptr};
        QVBoxLayout* _cardLayout{nullptr};
        QWidget* _header{nullptr};
        ShadLabel* _titleLabel{nullptr};
        ShadLabel* _descriptionLabel{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        int _contentWidth{kDefaultWidth};
        int _padding{kDefaultPadding};
        QPointer<QWidget> _anchor;
        ShadPlacementType::Side _side{ShadPlacementType::Bottom};
        ShadPlacementType::Align _align{ShadPlacementType::Center};
        // 实际使用的物理方向，决定滑入方向
        ShadPlacementType::Side _actualSide{ShadPlacementType::Bottom};
        QVariantAnimation* _openAnimation{nullptr};
        qreal _openProgress{1.0};
};

#endif // SHADPOPOVERPRIVATE_H
