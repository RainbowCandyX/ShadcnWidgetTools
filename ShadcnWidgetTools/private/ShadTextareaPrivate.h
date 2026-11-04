#ifndef SHADTEXTAREAPRIVATE_H
#define SHADTEXTAREAPRIVATE_H

#include "ShadProperty.h"
#include "ShadStyleHelper.h"

class ShadTextarea;

class ShadTextareaPrivate
{
    SHAD_D_CREATE(ShadTextarea)

    public:
        ShadTextareaPrivate();
        ~ShadTextareaPrivate();

        // min-h-16 可视最小高度
        static constexpr int kMinVisualHeight = 64;
        // 1px 边框 + 焦点环边距，作为 QFrame 线宽让滚动条和视口都位于边框内侧
        static constexpr int kFrameInset = 1 + ShadStyleHelper::kFocusRingWidth;
        // px-3 py-2 内边距
        static constexpr int kPaddingX = 12;
        static constexpr int kPaddingY = 8;

        // 嵌入 ShadInputGroup 时的上下内边距 py-3
        static constexpr int kEmbeddedPaddingY = 12;

        void applyThemeColors();
        void paintFrame();
        // 嵌入模式：没有边框与焦点环边距，不绘制外框
        bool isEmbedded() const;
        int frameInset() const;
        int paddingY() const;
        int focusMargin() const;
        // 嵌入状态变化后更新线宽、视口边距与自动高度
        void applyEmbedded();
        // 按文档内容重新计算自动高度，变化时通知布局
        void updateAutoHeight();
        // 自动增高且不限最大高度时不需要垂直滚动条，关闭以免换行瞬间闪现
        void updateScrollBarPolicy();

        bool _isInvalid{false};
        bool _isAutoResize{true};
        int _maximumAutoHeight{0};
        // 自动增长时的组件总高度（含焦点环边距）
        int _autoHeight{kMinVisualHeight + 2 * ShadStyleHelper::kFocusRingWidth};
};

#endif // SHADTEXTAREAPRIVATE_H
