#ifndef SHADCODEBLOCKVIEW_H
#define SHADCODEBLOCKVIEW_H

#include <QColor>
#include <QFont>
#include <QList>
#include <QPointF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTextLayout>
#include <QWidget>

#include <memory>
#include <vector>

#include "ShadCodeHighlighter.h"
#include "ShadDef.h"

class QAction;
class QContextMenuEvent;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class ShadDropdownMenu;

// ShadCodeBlock 的代码区（对应 CodeBlockContent 的 pre），作为 ShadScrollArea 的内容组件，尺寸等于全部代码的排版尺寸
// 每行用 QTextLayout 排版：leading-relaxed 行高、行号栏、行背景与 +/− 标记都在这里绘制，内边距随内容一起滚动
// 内容始终从左到右排列（dir="ltr"），不随父组件的布局方向镜像；支持拖动选择、双击选词、全选与复制
class ShadCodeBlockView : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadCodeBlockView(QWidget* parent = nullptr);
        ~ShadCodeBlockView() override;

        // 设置全部代码；与上次相比只有末尾变化时（流式追加），只重新解析和排版变化的行
        void setCode(const QString& code);
        QString getCode() const;
        // 显示的文字（ANSI 已去掉转义码），用于复制
        QString plainText() const;
        int lineCount() const;

        void setLanguage(const QString& language);
        QString getLanguage() const;

        // lineIndex 从 0 开始
        void setLineVariant(int lineIndex, ShadCodeBlockType::LineVariant variant);
        ShadCodeBlockType::LineVariant getLineVariant(int lineIndex) const;
        void clearLineVariants();

        void setLineNumbersVisible(bool visible);
        bool isLineNumbersVisible() const;
        void setStartLine(int line);
        int getStartLine() const;

        void setWordWrap(bool wrap);
        bool isWordWrap() const;

        // 代码字号：text-sm 为 14px，text-xs 为 12px
        void setFontPixelSize(int pixelSize);
        // Destructive 变体：普通文字为 destructive 色
        void setDestructive(bool destructive);
        // 内边距：horizontal 为 px，top / bottom 为 pt / pb；行背景横跨左右内边距（-mx-4 px-4）
        void setPadding(int horizontal, int top, int bottom);
        // 滚动视口宽度：换行模式下决定换行宽度，内容宽度不小于它
        void setViewportWidth(int width);

        // 当前视口宽度下的内容高度（含上下内边距）
        int contentHeight() const;
        // 不换行时一行放下最长代码所需的宽度（含内边距与行号栏）
        int naturalWidth() const;

        QString selectedText() const;
        bool hasSelectedText() const;
        void selectAll();
        void copySelection();

        QSize sizeHint() const override;

    Q_SIGNALS:
        // 内容高度或自然宽度变化
        void contentSizeChanged();

    protected:
        bool event(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;

    private:
        // 选择位置：行与行内字符位置
        struct Position
        {
            int line{0};
            int column{0};
        };

        static bool isBefore(const Position& first, const Position& second);
        // 从 first 行开始重新解析代码，并重新排版
        void rebuildFrom(int first);
        // 从 first 行开始重新排版；换行宽度变化时从第一行开始
        void relayoutFrom(int first);
        // 颜色、字号或内边距变化后重新排版全部行
        void relayoutAll();
        std::unique_ptr<QTextLayout> createLayout(int lineIndex) const;
        QList<QTextLayout::FormatRange> lineFormats(int lineIndex) const;
        QColor textColor() const;
        // 按字号重新计算字体、行高与行号栏宽度
        void updateFontMetrics();
        int gutterWidth() const;
        qreal textLeft() const;
        // 换行宽度，不换行时为 0
        qreal wrapWidth() const;
        void updateContentSize();
        int lineIndexAt(qreal y) const;
        qreal lineHeightOf(int lineIndex) const;
        Position positionAt(const QPointF& point) const;
        Position selectionStart() const;
        Position selectionEnd() const;
        void ensurePointVisible(const QPointF& point);
        void updateSelectionClipboard();

        QString _code;
        QString _language;
        ShadCodeHighlighter::Language _languageType{ShadCodeHighlighter::Language::PlainText};
        // 原始行（ANSI 含转义码）与显示行
        QStringList _rawLines;
        QStringList _lines;
        QList<QList<ShadCodeHighlighter::Span>> _spans;
        // 每行结束时的解析状态，用于从中间一行继续解析
        QList<int> _endStates;
        QList<ShadCodeBlockType::LineVariant> _variants;
        std::vector<std::unique_ptr<QTextLayout>> _layouts;
        // 每行的顶部位置（内容坐标）
        QList<qreal> _lineTops;
        qreal _contentBottom{0.0};
        qreal _maximumLineWidth{0.0};
        // 排版时使用的换行宽度，变化时需要重新排版全部行
        qreal _layoutWrapWidth{-1.0};

        QFont _codeFont;
        int _fontPixelSize{14};
        qreal _lineHeight{0.0};
        qreal _halfLeading{0.0};
        qreal _ascent{0.0};
        qreal _spaceWidth{0.0};
        bool _isDestructive{false};
        bool _isLineNumbersVisible{false};
        int _startLine{1};
        bool _isWordWrap{false};
        int _paddingX{16};
        int _paddingTop{16};
        int _paddingBottom{16};
        int _viewportWidth{0};
        int _lastContentHeight{-1};
        int _lastNaturalWidth{-1};

        Position _anchor;
        Position _cursor;
        bool _isSelecting{false};

        ShadDropdownMenu* _contextMenu{nullptr};
        QAction* _copyAction{nullptr};
};

#endif // SHADCODEBLOCKVIEW_H
