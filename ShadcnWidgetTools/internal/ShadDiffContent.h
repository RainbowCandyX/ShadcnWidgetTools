#ifndef SHADDIFFCONTENT_H
#define SHADDIFFCONTENT_H

#include <QFont>
#include <QList>
#include <QPointF>
#include <QSet>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTextLayout>
#include <QWidget>
#include <QtGlobal>

#include <memory>

#include "ShadCodeHighlighter.h"
#include "ShadDef.h"
#include "ShadDiffEngine.h"

class QAction;
class QColor;
class QContextMenuEvent;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QMoveEvent;
class QPaintEvent;
class QPainter;
class ShadDropdownMenu;

// ShadDiffView 的内容区：比较两段文本并绘制差异
class ShadDiffContent : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadDiffContent(QWidget* parent = nullptr);
        ~ShadDiffContent() override;

        void setTexts(const QString& oldText, const QString& newText);
        QString getOldText() const;
        QString getNewText() const;

        void setMode(ShadDiffViewType::Mode mode);
        ShadDiffViewType::Mode getMode() const;

        void setLanguage(const QString& language);
        QString getLanguage() const;

        void setLineNumbersVisible(bool visible);
        bool isLineNumbersVisible() const;

        // 小于 0 时显示全部行
        void setContextLines(int lines);
        int getContextLines() const;

        void setWordWrap(bool wrap);
        bool isWordWrap() const;

        void setSelectable(bool selectable);
        bool isSelectable() const;

        void setViewportWidth(int width);

        int contentHeight() const;
        int addedLineCount() const;
        int removedLineCount() const;

        QString selectedText() const;
        bool hasSelectedText() const;
        void selectAll();
        void copySelection();

        QSize sizeHint() const override;

    Q_SIGNALS:
        void contentSizeChanged();

    protected:
        bool event(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
        void moveEvent(QMoveEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        void leaveEvent(QEvent* event) override;

    private:
        // 显示的一行，分栏视图缺少的一侧为 -1
        struct Row
        {
            int oldLine{-1};
            int newLine{-1};
            bool isChanged{false};
            int foldStart{-1};
            int foldCount{0};
        };

        struct Pane
        {
            qreal left{0.0};
            qreal width{0.0};
        };

        enum class Side
        {
            Left = 0,
            Right,
        };

        enum class CellKind
        {
            Context = 0,
            Removed,
            Added,
            Blank,
        };

        struct Cell
        {
            CellKind kind{CellKind::Context};
            int numberColumns{1};
            int numbers[2]{-1, -1};
            bool isOld{false};
            int line{-1};
        };

        struct Position
        {
            int row{0};
            int column{0};
        };

        struct CellSelection
        {
            bool isActive{false};
            int from{0};
            int to{0};
            bool continues{false};
        };

        static bool isBefore(const Position& first, const Position& second);
        void rebuildDiff();
        void rebuildHighlight();
        void rebuildRows();
        void relayout();
        void updateFontMetrics();
        void updateContentSize();
        void updateNumberWidth();
        void updateCursor();
        qreal gutterWidth(int numberColumns) const;
        Pane leftPane() const;
        Pane rightPane() const;
        Pane paneOf(Side side) const;
        Side sideAt(qreal viewportX) const;
        Cell cellAt(int rowIndex, Side side) const;
        const QString& cellText(const Cell& cell) const;
        // 换行宽度，不换行时为 0
        qreal wrapWidth(const Pane& pane, int numberColumns) const;
        std::unique_ptr<QTextLayout> createLayout(const Cell& cell, const Pane& pane, bool isColored) const;
        int visualLineCount(const Cell& cell, const Pane& pane) const;
        // y 位置的行下标，不在任何一行上时返回 -1
        int rowAt(qreal y) const;
        void setHoveredRow(int row);
        Position positionAt(const QPointF& point, Side side) const;
        Position selectionStart() const;
        Position selectionEnd() const;
        CellSelection selectionOf(int rowIndex, Side side) const;
        void clearSelection();
        void ensurePointVisible(const QPointF& point);
        void updateSelectionClipboard();
        void paintCell(QPainter* painter, qreal originX, const Pane& pane, qreal top, qreal height, const Cell& cell, const CellSelection& selection) const;
        void paintFold(QPainter* painter, qreal originX, qreal top, const Row& row, bool isHovered) const;
        void fillTextRange(QPainter* painter, const QTextLayout& layout, qreal textLeft, qreal top, int from, int to, const QColor& color) const;

        QString _oldText;
        QString _newText;
        QStringList _oldLines;
        QStringList _newLines;
        QList<qreal> _oldLineWidths;
        QList<qreal> _newLineWidths;
        QList<QList<ShadCodeHighlighter::Span>> _oldSpans;
        QList<QList<ShadCodeHighlighter::Span>> _newSpans;
        QList<QList<ShadDiffEngine::Range>> _oldInlineRanges;
        QList<QList<ShadDiffEngine::Range>> _newInlineRanges;
        QList<ShadDiffEngine::Op> _ops;
        QSet<int> _expandedFolds;
        QList<Row> _rows;
        QList<int> _rowTops;
        int _addedLineCount{0};
        int _removedLineCount{0};
        qreal _maximumTextWidth{0.0};

        QString _language;
        ShadCodeHighlighter::Language _languageType{ShadCodeHighlighter::Language::PlainText};
        ShadDiffViewType::Mode _mode{ShadDiffViewType::Unified};
        bool _isLineNumbersVisible{true};
        bool _isWordWrap{false};
        bool _isSelectable{true};
        int _contextLines{-1};
        int _viewportWidth{0};
        int _hoveredRow{-1};
        int _lastContentHeight{-1};

        Side _selectionSide{Side::Left};
        Position _anchor;
        Position _cursor;
        bool _isSelecting{false};

        QFont _codeFont;
        int _numberWidth{0};
        qreal _halfLeading{0.0};
        qreal _ascent{0.0};
        qreal _spaceWidth{0.0};

        ShadDropdownMenu* _contextMenu{nullptr};
        QAction* _copyAction{nullptr};
};

#endif // SHADDIFFCONTENT_H
