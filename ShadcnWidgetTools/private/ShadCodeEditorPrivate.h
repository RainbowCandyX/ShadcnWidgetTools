#ifndef SHADCODEEDITORPRIVATE_H
#define SHADCODEEDITORPRIVATE_H

#include <QColor>
#include <QPointF>
#include <QRect>
#include <QString>
#include <QtGlobal>

#include "ShadProperty.h"
#include "ShadStyleHelper.h"

class ShadCodeEditor;
class ShadCodeEditorDocumentLayout;
class ShadCodeEditorGutter;
class ShadCodeEditorHighlighter;

class ShadCodeEditorPrivate
{
    SHAD_D_CREATE(ShadCodeEditor)

    public:
        ShadCodeEditorPrivate();
        ~ShadCodeEditorPrivate();

        // 1px 边框加焦点环边距，作为 QFrame 线宽
        static constexpr int kFrameInset = 1 + ShadStyleHelper::kFocusRingWidth;
        // px-3 py-2
        static constexpr int kPaddingX = 12;
        static constexpr int kPaddingY = 8;
        // text-sm
        static constexpr int kFontPixelSize = 14;
        // leading-relaxed
        static constexpr qreal kLineHeightRatio = 1.625;
        static constexpr int kDefaultTabSize = 4;

        // bg-muted，代码区与行号栏共用
        static QColor currentLineColor();

        void applyFont();
        void applyThemeColors();
        void applyTabStop();
        void updateLineHeight();
        int lineHeight() const;
        // 按整行高度绘制当前行与选区背景
        void paintLineBackgrounds(const QRect& exposed);
        // 把落在行内上下空白中的坐标移到该行文字范围内
        QPointF mapToTextLine(const QPointF& position) const;
        void updateViewportMargins();
        void updateGutterGeometry();
        // 背景跨越整行高度而基类只重绘文字范围，所以整体重绘
        void updateCurrentLine();
        void paintFrame();
        // 文档边距计入内边距，视口边距需扣除
        int documentMargin() const;

        ShadCodeEditorGutter* _gutter{nullptr};
        ShadCodeEditorHighlighter* _highlighter{nullptr};
        ShadCodeEditorDocumentLayout* _documentLayout{nullptr};
        QString _language;
        int _tabSize{kDefaultTabSize};
        bool _isLineNumbersVisible{true};
        bool _isInvalid{false};
};

#endif // SHADCODEEDITORPRIVATE_H
