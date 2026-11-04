#ifndef SHADTRACEPRIVATE_H
#define SHADTRACEPRIVATE_H

#include <QColor>
#include <QFont>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QUrl>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QVariantAnimation;
class QVBoxLayout;
class QWidget;
class ShadTrace;
class ShadTraceCollapsible;
class ShadTraceLine;
class ShadTransition;

class ShadTraceLinePrivate
{
    SHAD_D_CREATE(ShadTraceLine)

    public:
        ShadTraceLinePrivate();
        ~ShadTraceLinePrivate();

        // 图标 size-4，text-sm 行高 20
        static constexpr int kIconSize = 16;
        static constexpr int kLineHeight = 20;
        static constexpr int kFontPixelSize = 14;
        // 相邻元素 gap-2；标题之后的图标 -ml-1，与前一个元素相距 4；触发器中箭头与行之间 gap-1
        static constexpr int kGap = 8;
        static constexpr int kTrailingIconGap = 4;
        static constexpr int kChevronGap = 4;
        // data-[status=pending]:opacity-60，描述 text-muted-foreground/70
        static constexpr qreal kPendingOpacity = 0.6;
        static constexpr qreal kDescriptionAlpha = 0.7;

        // 各部分的逻辑矩形（LTR 坐标），绘制时按布局方向镜像
        struct Layout
        {
            QRectF icon;
            QRectF title;
            QRectF description;
            QRectF indicator;
            QRectF chevron;
            QString elidedTitle;
            // 最后一个元素的结束位置
            qreal right{0.0};
        };

        // 从 left 开始依次排列各部分，垂直中心为 centerY，标题宽度为 titleWidth
        Layout arrange(qreal left, qreal centerY, qreal titleWidth) const;
        // 按组件当前宽度排列，宽度不够时先压缩标题
        Layout computeLayout() const;
        // 标题宽度为 titleWidth 时，内容区（不含焦点环边距）的总宽度
        qreal contentWidth(qreal titleWidth) const;
        QFont titleFont() const;
        QFont descriptionFont() const;
        // 当前文字颜色：muted-foreground，可点击时悬停过渡到 foreground；Error 为 destructive
        QColor lineColor() const;
        // 按图标、状态与可见性启动或停止旋转与扫光动画
        void updateAnimations();
        // 点击、回车或空格触发：打开链接并发出 clicked
        void activate();
        // 由 ShadTraceCollapsible 设置：显示箭头，progress 为展开进度（0 到 1），箭头旋转 90 度
        void setChevronVisible(bool visible);
        void setChevronProgress(qreal progress);

        ShadIconType::IconName _icon{ShadIconType::None};
        QString _title;
        QString _description;
        bool _isDescriptionMonospace{false};
        ShadIconType::IconName _indicatorIcon{ShadIconType::None};
        ShadTraceType::Status _status{ShadTraceType::Done};
        bool _isClickable{false};
        QUrl _url;
        bool _hasChevron{false};
        qreal _chevronProgress{0.0};
        bool _isPressed{false};
        bool _isFocusVisible{false};
        ShadTransition* _hoverTransition{nullptr};
        QVariantAnimation* _spinAnimation{nullptr};
        QVariantAnimation* _shimmerAnimation{nullptr};
        qreal _spinAngle{0.0};
        qreal _shimmerProgress{0.0};
};

class ShadTraceCollapsiblePrivate
{
    SHAD_D_CREATE(ShadTraceCollapsible)

    public:
        ShadTraceCollapsiblePrivate();
        ~ShadTraceCollapsiblePrivate();

        // 详情区高度过渡 duration-200
        static constexpr int kDurationMs = 200;
        // 详情区 mt-2 mb-1；起始侧 ml-[calc(--spacing(2)-1px)]、1px 竖线、pl-4，结束侧 pr-1；内部组件之间 gap-2
        static constexpr int kContentTop = 8;
        static constexpr int kContentBottom = 4;
        static constexpr int kContentIndent = 7;
        static constexpr int kBorderWidth = 1;
        static constexpr int kContentPaddingStart = 16;
        static constexpr int kContentPaddingEnd = 4;
        static constexpr int kContentSpacing = 8;

        // 创建触发行：可点击，行后显示箭头（对应 TraceCollapsibleTrigger）
        void createTrigger(ShadIconType::IconName icon, const QString& title);
        // 按当前布局方向设置详情区布局的边距（QBoxLayout 的左右边距不随方向互换）
        void applyContentMargins();
        // 详情区在宽度 width 下的完整高度
        int contentHeight(int width) const;
        // 按当前宽度与展开进度排列触发行与详情区，并计算组件高度
        void updateLayout();
        void setProgress(qreal progress);

        ShadTraceLine* _trigger{nullptr};
        QWidget* _content{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        QVariantAnimation* _animation{nullptr};
        bool _isExpanded{false};
        qreal _progress{0.0};
        bool _isUpdatingLayout{false};
        // 当前宽度与展开进度下的组件高度
        int _preferredHeight{0};
        QSize _lastSizeHint;
};

class ShadTracePrivate
{
    SHAD_D_CREATE(ShadTrace)

    public:
        ShadTracePrivate();
        ~ShadTracePrivate();

        // 行间距 gap-2
        static constexpr int kSpacing = 8;

        QVBoxLayout* _layout{nullptr};
};

#endif // SHADTRACEPRIVATE_H
