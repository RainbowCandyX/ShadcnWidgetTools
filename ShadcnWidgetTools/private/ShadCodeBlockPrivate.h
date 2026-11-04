#ifndef SHADCODEBLOCKPRIVATE_H
#define SHADCODEBLOCKPRIVATE_H

#include <QList>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QGraphicsOpacityEffect;
class QHBoxLayout;
class QPaintEvent;
class QTimer;
class QVariantAnimation;
class ShadButton;
class ShadCodeBlock;
class ShadCodeBlockView;
class ShadScrollArea;

// 操作区容器（CodeBlockActions）：悬浮在代码块右上角时，为每个按钮绘制 bg-background 与 ring-1 ring-foreground/10
class ShadCodeBlockActions : public QWidget
{
    public:
        explicit ShadCodeBlockActions(QWidget* parent = nullptr);
        ~ShadCodeBlockActions() override;

        void setFloating(bool floating);
        bool isFloating() const;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        bool _isFloating{false};
};

class ShadCodeBlockPrivate
{
    SHAD_D_CREATE(ShadCodeBlock)

    public:
        ShadCodeBlockPrivate();
        ~ShadCodeBlockPrivate();

        // 1px 边框
        static constexpr int kBorderWidth = 1;
        // 操作按钮之间 gap-1；标题栏中的操作区 -me-1.5，向结束侧伸出 6px
        static constexpr int kActionGap = 4;
        static constexpr int kActionEndOffset = 6;
        // 标题与操作区之间 gap-2，标题图标与文字之间 me-1.5
        static constexpr int kHeaderGap = 8;
        static constexpr int kTitleIconGap = 6;
        // 悬浮操作区的淡入时长（transition-opacity 默认 150ms）
        static constexpr int kFadeDurationMs = 150;
        // 复制成功后显示对勾的时长
        static constexpr int kCopiedResetMs = 2000;

        // 各尺寸的间距与字号
        struct Metrics
        {
            // 标题栏、代码区与底栏的左右内边距 px-4（sm 为 px-3）
            int paddingX;
            // 标题栏顶部、代码区上下与底栏底部的内边距 py-4（sm 为 py-3）
            int paddingY;
            // 有标题栏时代码区顶部为 pt-2（sm 为 pt-1.5），有底栏时代码区底部为 pb-2
            int headerGap;
            int footerGap;
            // 标题行高、字号与图标尺寸：text-sm / size-4，sm 为 text-xs / size-3.5
            int titleHeight;
            int titlePixelSize;
            int titleIconSize;
            // 代码字号
            int codePixelSize;
            // 悬浮操作区距离右上角 top-3 end-3（sm 为 top-2 end-2）
            int floatingInset;
        };

        Metrics metrics() const;

        bool hasHeader() const;
        bool hasFooter() const;
        // 当前显示的操作组件，复制按钮在最后
        QList<QWidget*> actionWidgets() const;
        // 按当前宽度排列标题栏、代码区、底栏与操作区，并计算组件高度
        void updateLayout();
        void layoutActions(const Metrics& metrics, const QRect& innerRect, bool isHeader);
        // 按滚动位置更新代码区四周的渐隐遮罩
        void updateFade();
        // 悬浮操作区在悬停或焦点位于代码块内时显示
        void updateActionsOpacity(bool isAnimated);
        void copyAll();
        void resetCopyButton();
        // 复制按钮的提示文字变化时，鼠标仍在按钮上则立即更新显示的提示
        void updateCopyToolTip(const QString& text);

        ShadScrollArea* _scrollArea{nullptr};
        ShadCodeBlockView* _view{nullptr};
        ShadCodeBlockActions* _actions{nullptr};
        ShadButton* _copyButton{nullptr};
        QTimer* _copyResetTimer{nullptr};
        QList<QPointer<QWidget>> _actionWidgets;
        QWidget* _footer{nullptr};
        QHBoxLayout* _footerLayout{nullptr};
        QGraphicsOpacityEffect* _fadeEffect{nullptr};
        QGraphicsOpacityEffect* _actionsEffect{nullptr};
        QVariantAnimation* _actionsAnimation{nullptr};

        ShadCodeBlockType::Variant _variant{ShadCodeBlockType::Default};
        ShadCodeBlockType::Size _size{ShadCodeBlockType::Medium};
        QString _title;
        ShadIconType::IconName _titleIcon{ShadIconType::None};
        int _maximumContentHeight{0};
        bool _isCopyButtonVisible{false};
        bool _isHovered{false};
        bool _isFocusWithin{false};
        bool _isFocusVisible{false};
        bool _isUpdatingLayout{false};
        // 标题的逻辑矩形（LTR 坐标，绘制时按方向镜像）
        QRect _titleRect;
        // 当前宽度下的组件高度（含焦点环边距）
        int _preferredHeight{0};
        // 上次通知父布局时的 sizeHint
        QSize _lastSizeHint;
};

#endif // SHADCODEBLOCKPRIVATE_H
