#include "ShadDiffContent.h"

#include <QAction>
#include <QChar>
#include <QClipboard>
#include <QColor>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QScrollArea>
#include <QTextCharFormat>
#include <QTextLine>
#include <QTextOption>
#include <QtCore/qnamespace.h>
#include <QtMath>

#include <algorithm>
#include <iterator>
#include <utility>

#include "ShadDropdownMenu.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadKbd.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    using ShadDiffEngine::OpKind;
    using ShadDiffEngine::Range;

    constexpr int kFontPixelSize = 13;
    constexpr int kLineHeight = 20;
    constexpr int kPaddingX = 12;
    constexpr int kPaddingY = 8;
    constexpr int kLineNumberMinWidth = 24;
    constexpr int kLineNumberGap = 12;
    constexpr int kMarkerGap = 8;
    constexpr int kTabSize = 4;
    constexpr qreal kNoWrapLineWidth = 1.0e6;
    constexpr qreal kWrapTolerance = 1.0;
    constexpr int kMinFoldLines = 2;
    constexpr int kFoldIconSize = 14;
    constexpr int kFoldIconGap = 8;
    constexpr int kFoldFontPixelSize = 12;
    constexpr QRgb kAddedRgb = qRgb(0x22, 0xc5, 0x5e);
    constexpr qreal kRowAlpha = 0.12;
    constexpr qreal kInlineAlpha = 0.25;
    constexpr qreal kBlankAlpha = 0.5;
    constexpr int kAutoScrollMargin = 4;

    QStringList splitLines(const QString& text)
    {
        if (text.isEmpty())
        {
            return QStringList();
        }
        QString body = text;
        if (body.endsWith(u'\n'))
        {
            body.chop(1);
        }
        QStringList lines = body.split(u'\n');
        for (QString& line : lines)
        {
            if (line.endsWith(u'\r'))
            {
                line.chop(1);
            }
        }
        return lines;
    }

    QString expandTabs(const QString& line)
    {
        if (!line.contains(u'\t'))
        {
            return line;
        }
        QString result;
        int column = 0;
        for (const QChar ch : line)
        {
            if (ch != u'\t')
            {
                result.append(ch);
                ++column;
                continue;
            }
            const int spaces = kTabSize - column % kTabSize;
            result.append(QString(spaces, u' '));
            column += spaces;
        }
        return result;
    }

    bool isWordCharacter(QChar ch)
    {
        return ch.isLetterOrNumber() || ch == u'_';
    }
} // namespace

ShadDiffContent::ShadDiffContent(QWidget* parent)
    : QWidget(parent)
{
    setLayoutDirection(Qt::LeftToRight);
    setFocusPolicy(Qt::ClickFocus);
    setMouseTracking(true);
    updateFontMetrics();
    updateCursor();
    relayout();
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadDiffContent::~ShadDiffContent()
{
}

void ShadDiffContent::setTexts(const QString& oldText, const QString& newText)
{
    if (_oldText == oldText && _newText == newText)
    {
        return;
    }
    _oldText = oldText;
    _newText = newText;
    rebuildDiff();
}

QString ShadDiffContent::getOldText() const
{
    return _oldText;
}

QString ShadDiffContent::getNewText() const
{
    return _newText;
}

void ShadDiffContent::setMode(ShadDiffViewType::Mode mode)
{
    if (_mode == mode)
    {
        return;
    }
    _mode = mode;
    rebuildRows();
}

ShadDiffViewType::Mode ShadDiffContent::getMode() const
{
    return _mode;
}

void ShadDiffContent::setLanguage(const QString& language)
{
    if (_language == language)
    {
        return;
    }
    _language = language;
    _languageType = ShadCodeHighlighter::languageFromName(language);
    // 不解析终端转义码
    if (_languageType == ShadCodeHighlighter::Language::Ansi)
    {
        _languageType = ShadCodeHighlighter::Language::PlainText;
    }
    rebuildHighlight();
    update();
}

QString ShadDiffContent::getLanguage() const
{
    return _language;
}

void ShadDiffContent::setLineNumbersVisible(bool visible)
{
    if (_isLineNumbersVisible == visible)
    {
        return;
    }
    _isLineNumbersVisible = visible;
    relayout();
}

bool ShadDiffContent::isLineNumbersVisible() const
{
    return _isLineNumbersVisible;
}

void ShadDiffContent::setContextLines(int lines)
{
    const int value = qMax(-1, lines);
    if (_contextLines == value)
    {
        return;
    }
    _contextLines = value;
    _expandedFolds.clear();
    rebuildRows();
}

int ShadDiffContent::getContextLines() const
{
    return _contextLines;
}

void ShadDiffContent::setWordWrap(bool wrap)
{
    if (_isWordWrap == wrap)
    {
        return;
    }
    _isWordWrap = wrap;
    relayout();
}

bool ShadDiffContent::isWordWrap() const
{
    return _isWordWrap;
}

void ShadDiffContent::setSelectable(bool selectable)
{
    if (_isSelectable == selectable)
    {
        return;
    }
    _isSelectable = selectable;
    setFocusPolicy(selectable ? Qt::ClickFocus : Qt::NoFocus);
    if (!selectable)
    {
        clearSelection();
    }
    updateCursor();
    update();
}

bool ShadDiffContent::isSelectable() const
{
    return _isSelectable;
}

void ShadDiffContent::setViewportWidth(int width)
{
    if (_viewportWidth == width)
    {
        return;
    }
    _viewportWidth = width;
    if (_isWordWrap)
    {
        relayout();
        return;
    }
    updateContentSize();
    update();
}

int ShadDiffContent::contentHeight() const
{
    return (_rowTops.isEmpty() ? kPaddingY : _rowTops.last()) + kPaddingY;
}

int ShadDiffContent::addedLineCount() const
{
    return _addedLineCount;
}

int ShadDiffContent::removedLineCount() const
{
    return _removedLineCount;
}

QString ShadDiffContent::selectedText() const
{
    const Position start = selectionStart();
    const Position end = selectionEnd();
    if (!isBefore(start, end))
    {
        return QString();
    }
    QStringList parts;
    for (int index = start.row; index <= end.row && index < _rows.size(); ++index)
    {
        const Cell cell = cellAt(index, _selectionSide);
        if (cell.line < 0)
        {
            continue;
        }
        const QString& text = cellText(cell);
        const int from = index == start.row ? start.column : 0;
        const int to = index == end.row ? end.column : int(text.size());
        parts.append(text.mid(from, qMax(0, to - from)));
    }
    return parts.join(u'\n');
}

bool ShadDiffContent::hasSelectedText() const
{
    return isBefore(selectionStart(), selectionEnd());
}

void ShadDiffContent::selectAll()
{
    if (!_isSelectable || _rows.isEmpty())
    {
        return;
    }
    const int lastRow = int(_rows.size()) - 1;
    const Cell lastCell = cellAt(lastRow, _selectionSide);
    _anchor = Position();
    _cursor.row = lastRow;
    _cursor.column = lastCell.line < 0 ? 0 : int(cellText(lastCell).size());
    update();
    updateSelectionClipboard();
}

void ShadDiffContent::copySelection()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard != nullptr && hasSelectedText())
    {
        clipboard->setText(selectedText());
    }
}

QSize ShadDiffContent::sizeHint() const
{
    return QSize(width(), contentHeight());
}

bool ShadDiffContent::event(QEvent* event)
{
    // 由本组件处理复制与全选，不触发窗口快捷键
    if (event->type() == QEvent::ShortcutOverride && _isSelectable)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->matches(QKeySequence::Copy) || keyEvent->matches(QKeySequence::SelectAll))
        {
            event->accept();
            return true;
        }
    }
    // 右键菜单首次打开时创建，语言切换后丢弃重建
    if (event->type() == QEvent::LanguageChange && _contextMenu != nullptr)
    {
        _contextMenu->deleteLater();
        _contextMenu = nullptr;
        _copyAction = nullptr;
    }
    return QWidget::event(event);
}

void ShadDiffContent::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange || event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadDiffContent::paintEvent(QPaintEvent* event)
{
    if (_rows.isEmpty())
    {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
    const qreal originX = -x();
    const QRect exposed = event->rect();
    const int lastRow = int(_rows.size()) - 1;
    const auto clampedRowAt = [this, lastRow](int y)
    {
        if (y < _rowTops.first())
        {
            return 0;
        }
        return y >= _rowTops.last() ? lastRow : rowAt(y);
    };
    const int first = clampedRowAt(exposed.top());
    const int last = clampedRowAt(exposed.bottom());
    const bool isSplit = _mode == ShadDiffViewType::Split;
    const Pane left = leftPane();
    const Pane right = rightPane();
    if (isSplit)
    {
        painter.fillRect(QRectF(originX + left.width, exposed.top(), 1.0, exposed.height()), shadTheme->getColor(ShadThemeType::Border));
    }
    for (int index = first; index <= last; ++index)
    {
        const Row& row = _rows.at(index);
        const qreal top = _rowTops.at(index);
        const qreal height = _rowTops.at(index + 1) - top;
        if (row.foldCount > 0)
        {
            paintFold(&painter, originX, top, row, index == _hoveredRow);
            continue;
        }
        paintCell(&painter, originX, left, top, height, cellAt(index, Side::Left), selectionOf(index, Side::Left));
        if (isSplit)
        {
            paintCell(&painter, originX, right, top, height, cellAt(index, Side::Right), selectionOf(index, Side::Right));
        }
    }
}

void ShadDiffContent::moveEvent(QMoveEvent* event)
{
    QWidget::moveEvent(event);
    if (event->pos().x() != event->oldPos().x())
    {
        update();
    }
}

void ShadDiffContent::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    const int row = rowAt(event->position().y());
    if (row >= 0 && _rows.at(row).foldCount > 0)
    {
        event->accept();
        return;
    }
    if (!_isSelectable)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    const Side side = sideAt(event->position().x() + x());
    const Position position = positionAt(event->position(), side);
    if (!(event->modifiers() & Qt::ShiftModifier) || side != _selectionSide)
    {
        _selectionSide = side;
        _anchor = position;
    }
    _cursor = position;
    _isSelecting = true;
    update();
    event->accept();
}

void ShadDiffContent::mouseMoveEvent(QMouseEvent* event)
{
    if (_isSelecting && (event->buttons() & Qt::LeftButton))
    {
        _cursor = positionAt(event->position(), _selectionSide);
        ensurePointVisible(event->position());
        update();
        event->accept();
        return;
    }
    const int row = rowAt(event->position().y());
    setHoveredRow(row >= 0 && _rows.at(row).foldCount > 0 ? row : -1);
    QWidget::mouseMoveEvent(event);
}

void ShadDiffContent::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    if (_isSelecting)
    {
        _isSelecting = false;
        updateSelectionClipboard();
        event->accept();
        return;
    }
    const int row = rowAt(event->position().y());
    if (row >= 0 && _rows.at(row).foldCount > 0 && rect().contains(event->position().toPoint()))
    {
        _expandedFolds.insert(_rows.at(row).foldStart);
        rebuildRows();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadDiffContent::mouseDoubleClickEvent(QMouseEvent* event)
{
    const int row = rowAt(event->position().y());
    const Side side = sideAt(event->position().x() + x());
    if (event->button() != Qt::LeftButton || !_isSelectable || row < 0 || cellAt(row, side).line < 0)
    {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }
    const Position position = positionAt(event->position(), side);
    const QString& text = cellText(cellAt(row, side));
    const int length = int(text.size());
    int from = qBound(0, position.column, length);
    int to = from;
    const bool isOnWord = (from < length && isWordCharacter(text.at(from))) || (from > 0 && isWordCharacter(text.at(from - 1)));
    if (isOnWord)
    {
        while (from > 0 && isWordCharacter(text.at(from - 1)))
        {
            --from;
        }
        while (to < length && isWordCharacter(text.at(to)))
        {
            ++to;
        }
    }
    else if (from < length)
    {
        to = from + 1;
    }
    _selectionSide = side;
    _anchor.row = row;
    _anchor.column = from;
    _cursor.row = row;
    _cursor.column = to;
    _isSelecting = false;
    update();
    updateSelectionClipboard();
    event->accept();
}

void ShadDiffContent::keyPressEvent(QKeyEvent* event)
{
    if (_isSelectable && event->matches(QKeySequence::Copy))
    {
        copySelection();
        event->accept();
        return;
    }
    if (_isSelectable && event->matches(QKeySequence::SelectAll))
    {
        selectAll();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadDiffContent::contextMenuEvent(QContextMenuEvent* event)
{
    if (!_isSelectable)
    {
        QWidget::contextMenuEvent(event);
        return;
    }
    if (_contextMenu == nullptr)
    {
        _contextMenu = new ShadDropdownMenu(this);
        _copyAction = _contextMenu->addItem(tr("复制"), ShadIconType::Copy, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Copy)));
        QAction* selectAllAction = _contextMenu->addItem(tr("全选"), ShadIconType::TextSelect, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::SelectAll)));
        connect(_copyAction, &QAction::triggered, this, &ShadDiffContent::copySelection);
        connect(selectAllAction, &QAction::triggered, this, &ShadDiffContent::selectAll);
    }
    _copyAction->setEnabled(hasSelectedText());
    _contextMenu->popupAt(event->globalPos());
    event->accept();
}

void ShadDiffContent::leaveEvent(QEvent* event)
{
    setHoveredRow(-1);
    QWidget::leaveEvent(event);
}

bool ShadDiffContent::isBefore(const Position& first, const Position& second)
{
    return first.row < second.row || (first.row == second.row && first.column < second.column);
}

void ShadDiffContent::rebuildDiff()
{
    const QStringList oldRawLines = splitLines(_oldText);
    const QStringList newRawLines = splitLines(_newText);
    _ops = ShadDiffEngine::diffLines(oldRawLines, newRawLines);
    _oldLines.clear();
    _newLines.clear();
    for (const QString& line : oldRawLines)
    {
        _oldLines.append(expandTabs(line));
    }
    for (const QString& line : newRawLines)
    {
        _newLines.append(expandTabs(line));
    }

    _oldInlineRanges = QList<QList<Range>>(_oldLines.size());
    _newInlineRanges = QList<QList<Range>>(_newLines.size());
    _addedLineCount = 0;
    _removedLineCount = 0;
    const int count = int(_ops.size());
    int index = 0;
    while (index < count)
    {
        if (_ops.at(index).kind == OpKind::Equal)
        {
            ++index;
            continue;
        }
        int removedEnd = index;
        while (removedEnd < count && _ops.at(removedEnd).kind == OpKind::Removed)
        {
            ++removedEnd;
        }
        int addedEnd = removedEnd;
        while (addedEnd < count && _ops.at(addedEnd).kind == OpKind::Added)
        {
            ++addedEnd;
        }
        const int removedCount = removedEnd - index;
        const int addedCount = addedEnd - removedEnd;
        _removedLineCount += removedCount;
        _addedLineCount += addedCount;
        for (int pair = 0; pair < qMin(removedCount, addedCount); ++pair)
        {
            const int oldLine = _ops.at(index + pair).oldIndex;
            const int newLine = _ops.at(removedEnd + pair).newIndex;
            ShadDiffEngine::diffCharacters(_oldLines.at(oldLine), _newLines.at(newLine), _oldInlineRanges[oldLine], _newInlineRanges[newLine]);
        }
        index = addedEnd;
    }

    const QFontMetricsF metrics(_codeFont);
    _oldLineWidths.clear();
    _newLineWidths.clear();
    _maximumTextWidth = 0.0;
    for (const QString& line : std::as_const(_oldLines))
    {
        _oldLineWidths.append(metrics.horizontalAdvance(line));
        _maximumTextWidth = qMax(_maximumTextWidth, _oldLineWidths.last());
    }
    for (const QString& line : std::as_const(_newLines))
    {
        _newLineWidths.append(metrics.horizontalAdvance(line));
        _maximumTextWidth = qMax(_maximumTextWidth, _newLineWidths.last());
    }
    updateNumberWidth();
    _expandedFolds.clear();
    rebuildHighlight();
    rebuildRows();
}

void ShadDiffContent::rebuildHighlight()
{
    // 新旧文本分别解析，多行状态在各自文本中延续
    const auto highlight = [this](const QStringList& lines)
    {
        QList<QList<ShadCodeHighlighter::Span>> spans;
        spans.reserve(lines.size());
        int state = 0;
        for (const QString& line : lines)
        {
            spans.append(_languageType == ShadCodeHighlighter::Language::PlainText ? QList<ShadCodeHighlighter::Span>() : ShadCodeHighlighter::highlightLine(_languageType, line, state));
        }
        return spans;
    };
    _oldSpans = highlight(_oldLines);
    _newSpans = highlight(_newLines);
}

void ShadDiffContent::rebuildRows()
{
    _rows.clear();
    const bool isSplit = _mode == ShadDiffViewType::Split;
    const int count = int(_ops.size());
    int index = 0;
    while (index < count)
    {
        int end = index;
        if (_ops.at(index).kind == OpKind::Equal)
        {
            while (end < count && _ops.at(end).kind == OpKind::Equal)
            {
                ++end;
            }
            const int length = end - index;
            int head = length;
            int tail = 0;
            if (_contextLines >= 0 && !_expandedFolds.contains(index))
            {
                const int headCount = index == 0 ? 0 : _contextLines;
                const int tailCount = end == count ? 0 : _contextLines;
                if (length - headCount - tailCount >= kMinFoldLines)
                {
                    head = headCount;
                    tail = tailCount;
                }
            }
            for (int position = index; position < index + head; ++position)
            {
                _rows.append(Row{_ops.at(position).oldIndex, _ops.at(position).newIndex, false, -1, 0});
            }
            if (head + tail < length)
            {
                _rows.append(Row{-1, -1, false, index, length - head - tail});
            }
            for (int position = end - tail; position < end; ++position)
            {
                _rows.append(Row{_ops.at(position).oldIndex, _ops.at(position).newIndex, false, -1, 0});
            }
            index = end;
            continue;
        }
        int removedEnd = index;
        while (removedEnd < count && _ops.at(removedEnd).kind == OpKind::Removed)
        {
            ++removedEnd;
        }
        end = removedEnd;
        while (end < count && _ops.at(end).kind == OpKind::Added)
        {
            ++end;
        }
        const int removedCount = removedEnd - index;
        const int addedCount = end - removedEnd;
        if (isSplit)
        {
            for (int position = 0; position < qMax(removedCount, addedCount); ++position)
            {
                const int oldLine = position < removedCount ? _ops.at(index + position).oldIndex : -1;
                const int newLine = position < addedCount ? _ops.at(removedEnd + position).newIndex : -1;
                _rows.append(Row{oldLine, newLine, true, -1, 0});
            }
        }
        else
        {
            for (int position = index; position < removedEnd; ++position)
            {
                _rows.append(Row{_ops.at(position).oldIndex, -1, true, -1, 0});
            }
            for (int position = removedEnd; position < end; ++position)
            {
                _rows.append(Row{-1, _ops.at(position).newIndex, true, -1, 0});
            }
        }
        index = end;
    }
    // 行下标变化后，原选择与悬停失效
    _hoveredRow = -1;
    clearSelection();
    updateCursor();
    relayout();
}

void ShadDiffContent::relayout()
{
    const bool isSplit = _mode == ShadDiffViewType::Split;
    const Pane left = leftPane();
    const Pane right = rightPane();
    _rowTops.clear();
    _rowTops.reserve(_rows.size() + 1);
    int top = kPaddingY;
    for (int index = 0; index < _rows.size(); ++index)
    {
        _rowTops.append(top);
        int lineCount = 1;
        if (_rows.at(index).foldCount == 0)
        {
            lineCount = visualLineCount(cellAt(index, Side::Left), left);
            if (isSplit)
            {
                lineCount = qMax(lineCount, visualLineCount(cellAt(index, Side::Right), right));
            }
        }
        top += lineCount * kLineHeight;
    }
    _rowTops.append(top);
    updateContentSize();
    update();
}

void ShadDiffContent::updateFontMetrics()
{
    QFont font = this->font();
    font.setFamilies(ShadStyleHelper::monospaceFontFamilies());
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    font.setPixelSize(kFontPixelSize);
    font.setWeight(QFont::Normal);
    font.setItalic(false);
    _codeFont = font;
    const QFontMetricsF metrics(_codeFont);
    _ascent = metrics.ascent();
    _halfLeading = (kLineHeight - metrics.height()) / 2.0;
    _spaceWidth = metrics.horizontalAdvance(QChar(u' '));
    updateNumberWidth();
}

void ShadDiffContent::updateContentSize()
{
    int overflow = 0;
    if (!_isWordWrap)
    {
        qreal visibleWidth = _viewportWidth - gutterWidth(2) - kPaddingX;
        if (_mode == ShadDiffViewType::Split)
        {
            visibleWidth = qMin(leftPane().width, rightPane().width) - gutterWidth(1) - kPaddingX;
        }
        overflow = qMax(0, qCeil(_maximumTextWidth - visibleWidth));
    }
    const int heightValue = contentHeight();
    const QSize contentSize(qMax(0, _viewportWidth) + overflow, heightValue);
    if (size() != contentSize)
    {
        resize(contentSize);
    }
    if (heightValue != _lastContentHeight)
    {
        _lastContentHeight = heightValue;
        updateGeometry();
        Q_EMIT contentSizeChanged();
    }
}

void ShadDiffContent::updateNumberWidth()
{
    const int lastNumber = qMax(1, int(qMax(_oldLines.size(), _newLines.size())));
    _numberWidth = qMax(kLineNumberMinWidth, ShadStyleHelper::textWidth(_codeFont, QString::number(lastNumber)));
}

void ShadDiffContent::updateCursor()
{
    if (_hoveredRow >= 0)
    {
        setCursor(Qt::PointingHandCursor);
    }
    else if (_isSelectable)
    {
        setCursor(Qt::IBeamCursor);
    }
    else
    {
        unsetCursor();
    }
}

qreal ShadDiffContent::gutterWidth(int numberColumns) const
{
    const int numbersWidth = _isLineNumbersVisible ? numberColumns * (_numberWidth + kLineNumberGap) : 0;
    return kPaddingX + numbersWidth + _spaceWidth + kMarkerGap;
}

ShadDiffContent::Pane ShadDiffContent::leftPane() const
{
    if (_mode != ShadDiffViewType::Split)
    {
        return Pane{0.0, qreal(_viewportWidth)};
    }
    return Pane{0.0, qreal(qMax(0, (_viewportWidth - 1) / 2))};
}

ShadDiffContent::Pane ShadDiffContent::rightPane() const
{
    const qreal left = leftPane().width + 1.0;
    return Pane{left, qMax(0.0, _viewportWidth - left)};
}

ShadDiffContent::Pane ShadDiffContent::paneOf(Side side) const
{
    return side == Side::Right && _mode == ShadDiffViewType::Split ? rightPane() : leftPane();
}

ShadDiffContent::Side ShadDiffContent::sideAt(qreal viewportX) const
{
    return _mode == ShadDiffViewType::Split && viewportX >= rightPane().left ? Side::Right : Side::Left;
}

ShadDiffContent::Cell ShadDiffContent::cellAt(int rowIndex, Side side) const
{
    Cell cell;
    const Row& row = _rows.at(rowIndex);
    if (row.foldCount > 0)
    {
        cell.kind = CellKind::Blank;
        return cell;
    }
    if (_mode != ShadDiffViewType::Split)
    {
        cell.numberColumns = 2;
        if (!row.isChanged)
        {
            cell.numbers[0] = row.oldLine;
            cell.numbers[1] = row.newLine;
            cell.line = row.newLine;
        }
        else if (row.oldLine >= 0)
        {
            cell.kind = CellKind::Removed;
            cell.numbers[0] = row.oldLine;
            cell.isOld = true;
            cell.line = row.oldLine;
        }
        else
        {
            cell.kind = CellKind::Added;
            cell.numbers[1] = row.newLine;
            cell.line = row.newLine;
        }
        return cell;
    }
    const bool isOld = side == Side::Left;
    const int line = isOld ? row.oldLine : row.newLine;
    cell.isOld = isOld;
    cell.line = line;
    cell.numbers[0] = line;
    if (line < 0)
    {
        cell.kind = CellKind::Blank;
    }
    else if (row.isChanged)
    {
        cell.kind = isOld ? CellKind::Removed : CellKind::Added;
    }
    return cell;
}

const QString& ShadDiffContent::cellText(const Cell& cell) const
{
    return (cell.isOld ? _oldLines : _newLines).at(cell.line);
}

qreal ShadDiffContent::wrapWidth(const Pane& pane, int numberColumns) const
{
    // 视口宽度未知时先不换行排版
    if (!_isWordWrap || _viewportWidth <= 0)
    {
        return 0.0;
    }
    return qMax(1.0, pane.width - gutterWidth(numberColumns) - kPaddingX);
}

std::unique_ptr<QTextLayout> ShadDiffContent::createLayout(const Cell& cell, const Pane& pane, bool isColored) const
{
    std::unique_ptr<QTextLayout> layout = std::make_unique<QTextLayout>(cellText(cell), _codeFont);
    const qreal width = wrapWidth(pane, cell.numberColumns);
    QTextOption option;
    option.setWrapMode(width > 0.0 ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    option.setTextDirection(Qt::LeftToRight);
    option.setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    layout->setTextOption(option);
    if (isColored)
    {
        QList<QTextLayout::FormatRange> formats;
        const bool isDark = shadTheme->isDark();
        for (const ShadCodeHighlighter::Span& span : (cell.isOld ? _oldSpans : _newSpans).at(cell.line))
        {
            const QColor color = ShadCodeHighlighter::tokenColor(span.kind, isDark);
            if (!color.isValid())
            {
                continue;
            }
            QTextLayout::FormatRange range;
            range.start = span.start;
            range.length = span.length;
            range.format.setForeground(color);
            formats.append(range);
        }
        layout->setFormats(formats);
    }
    layout->beginLayout();
    int count = 0;
    while (true)
    {
        QTextLine line = layout->createLine();
        if (!line.isValid())
        {
            break;
        }
        line.setLineWidth(width > 0.0 ? width : kNoWrapLineWidth);
        line.setPosition(QPointF(0.0, count * kLineHeight + _halfLeading));
        ++count;
    }
    layout->endLayout();
    return layout;
}

int ShadDiffContent::visualLineCount(const Cell& cell, const Pane& pane) const
{
    if (cell.line < 0)
    {
        return 1;
    }
    const qreal width = wrapWidth(pane, cell.numberColumns);
    if (width <= 0.0 || (cell.isOld ? _oldLineWidths : _newLineWidths).at(cell.line) <= width - kWrapTolerance)
    {
        return 1;
    }
    return qMax(1, createLayout(cell, pane, false)->lineCount());
}

int ShadDiffContent::rowAt(qreal y) const
{
    if (_rows.isEmpty() || y < _rowTops.first() || y >= _rowTops.last())
    {
        return -1;
    }
    const auto it = std::upper_bound(_rowTops.cbegin(), _rowTops.cend(), y);
    const int index = int(std::distance(_rowTops.cbegin(), it)) - 1;
    return qBound(0, index, int(_rows.size()) - 1);
}

void ShadDiffContent::setHoveredRow(int row)
{
    if (_hoveredRow == row)
    {
        return;
    }
    _hoveredRow = row;
    updateCursor();
    update();
}

ShadDiffContent::Position ShadDiffContent::positionAt(const QPointF& point, Side side) const
{
    Position position;
    if (_rows.isEmpty() || point.y() < _rowTops.first())
    {
        return position;
    }
    if (point.y() >= _rowTops.last())
    {
        const Cell lastCell = cellAt(int(_rows.size()) - 1, side);
        position.row = int(_rows.size()) - 1;
        position.column = lastCell.line < 0 ? 0 : int(cellText(lastCell).size());
        return position;
    }
    const int index = rowAt(point.y());
    position.row = index;
    const Cell cell = cellAt(index, side);
    if (cell.line < 0)
    {
        return position;
    }
    const Pane pane = paneOf(side);
    const std::unique_ptr<QTextLayout> layout = createLayout(cell, pane, false);
    if (layout->lineCount() <= 0)
    {
        return position;
    }
    const int visual = qBound(0, int((point.y() - _rowTops.at(index)) / kLineHeight), layout->lineCount() - 1);
    position.column = layout->lineAt(visual).xToCursor(point.x() - pane.left - gutterWidth(cell.numberColumns), QTextLine::CursorBetweenCharacters);
    return position;
}

ShadDiffContent::Position ShadDiffContent::selectionStart() const
{
    return isBefore(_cursor, _anchor) ? _cursor : _anchor;
}

ShadDiffContent::Position ShadDiffContent::selectionEnd() const
{
    return isBefore(_cursor, _anchor) ? _anchor : _cursor;
}

ShadDiffContent::CellSelection ShadDiffContent::selectionOf(int rowIndex, Side side) const
{
    CellSelection selection;
    const Position start = selectionStart();
    const Position end = selectionEnd();
    if (!isBefore(start, end) || rowIndex < start.row || rowIndex > end.row)
    {
        return selection;
    }
    if (_mode == ShadDiffViewType::Split && side != _selectionSide)
    {
        return selection;
    }
    const Cell cell = cellAt(rowIndex, side);
    if (cell.line < 0)
    {
        return selection;
    }
    const int length = int(cellText(cell).size());
    selection.isActive = true;
    selection.from = rowIndex == start.row ? qMin(start.column, length) : 0;
    selection.to = rowIndex == end.row ? qMin(end.column, length) : length;
    selection.continues = rowIndex < end.row;
    return selection;
}

void ShadDiffContent::clearSelection()
{
    _anchor = Position();
    _cursor = Position();
    _isSelecting = false;
}

void ShadDiffContent::ensurePointVisible(const QPointF& point)
{
    QWidget* viewport = parentWidget();
    QScrollArea* scrollArea = qobject_cast<QScrollArea*>(viewport != nullptr ? viewport->parentWidget() : nullptr);
    if (scrollArea != nullptr)
    {
        scrollArea->ensureVisible(qRound(point.x()), qRound(point.y()), kAutoScrollMargin, kAutoScrollMargin);
    }
}

void ShadDiffContent::updateSelectionClipboard()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard != nullptr && clipboard->supportsSelection() && hasSelectedText())
    {
        clipboard->setText(selectedText(), QClipboard::Selection);
    }
}

void ShadDiffContent::paintCell(QPainter* painter, qreal originX, const Pane& pane, qreal top, qreal height, const Cell& cell, const CellSelection& selection) const
{
    using namespace ShadStyleHelper;
    const QRectF rowRect(originX + pane.left, top, pane.width, height);
    if (cell.kind == CellKind::Blank)
    {
        painter->fillRect(rowRect, withAlpha(shadTheme->getColor(ShadThemeType::Muted), kBlankAlpha));
        return;
    }
    const bool isChanged = cell.kind != CellKind::Context;
    const QColor accentColor = cell.kind == CellKind::Removed ? shadTheme->getColor(ShadThemeType::Destructive) : QColor(kAddedRgb);
    if (isChanged)
    {
        painter->fillRect(rowRect, withAlpha(accentColor, kRowAlpha));
    }
    const qreal baseline = top + _halfLeading + _ascent;
    painter->setFont(_codeFont);
    qreal columnLeft = rowRect.left() + kPaddingX;
    if (_isLineNumbersVisible)
    {
        const QFontMetricsF metrics(_codeFont);
        painter->setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
        for (int column = 0; column < cell.numberColumns; ++column)
        {
            if (cell.numbers[column] >= 0)
            {
                const QString number = QString::number(cell.numbers[column] + 1);
                painter->drawText(QPointF(columnLeft + _numberWidth - metrics.horizontalAdvance(number), baseline), number);
            }
            columnLeft += _numberWidth + kLineNumberGap;
        }
    }
    if (isChanged)
    {
        painter->setPen(accentColor);
        painter->drawText(QPointF(columnLeft, baseline), cell.kind == CellKind::Removed ? QStringLiteral("−") : QStringLiteral("+"));
    }

    const qreal gutter = gutterWidth(cell.numberColumns);
    const qreal textLeft = pane.left + gutter;
    const std::unique_ptr<QTextLayout> layout = createLayout(cell, pane, true);
    const int lineCount = layout->lineCount();
    painter->save();
    painter->setClipRect(QRectF(rowRect.left() + gutter, top, qMax(0.0, pane.width - gutter), height), Qt::IntersectClip);
    if (isChanged)
    {
        for (const Range& range : (cell.isOld ? _oldInlineRanges : _newInlineRanges).at(cell.line))
        {
            fillTextRange(painter, *layout, textLeft, top, range.start, range.start + range.length, withAlpha(accentColor, kInlineAlpha));
        }
    }
    QList<QTextLayout::FormatRange> selections;
    if (selection.isActive)
    {
        const QColor selectionColor = shadTheme->getColor(ShadThemeType::Primary);
        fillTextRange(painter, *layout, textLeft, top, selection.from, selection.to, selectionColor);
        if (selection.continues)
        {
            const qreal endX = lineCount > 0 ? layout->lineAt(lineCount - 1).naturalTextWidth() : 0.0;
            painter->fillRect(QRectF(textLeft + endX, top + qMax(0, lineCount - 1) * kLineHeight, _spaceWidth, kLineHeight), selectionColor);
        }
        if (selection.to > selection.from)
        {
            QTextLayout::FormatRange range;
            range.start = selection.from;
            range.length = selection.to - selection.from;
            range.format.setForeground(shadTheme->getColor(ShadThemeType::PrimaryForeground));
            selections.append(range);
        }
    }
    if (lineCount > 0)
    {
        painter->setPen(shadTheme->getColor(ShadThemeType::Foreground));
        layout->draw(painter, QPointF(textLeft, top), selections);
    }
    painter->restore();
}

void ShadDiffContent::paintFold(QPainter* painter, qreal originX, qreal top, const Row& row, bool isHovered) const
{
    const QRectF rowRect(originX, top, _viewportWidth, kLineHeight);
    painter->fillRect(rowRect, shadTheme->getColor(isHovered ? ShadThemeType::Accent : ShadThemeType::Muted));
    const QColor color = shadTheme->getColor(isHovered ? ShadThemeType::AccentForeground : ShadThemeType::MutedForeground);
    const QRectF iconRect(rowRect.left() + kPaddingX, top + (kLineHeight - kFoldIconSize) / 2.0, kFoldIconSize, kFoldIconSize);
    ShadIcon::paint(painter, iconRect, ShadIconType::UnfoldVertical, color, kFoldIconSize);
    QFont labelFont = font();
    labelFont.setPixelSize(kFoldFontPixelSize);
    painter->setFont(labelFont);
    painter->setPen(color);
    const qreal textLeft = iconRect.right() + kFoldIconGap;
    const QRectF textRect(textLeft, top, qMax(0.0, rowRect.right() - kPaddingX - textLeft), kLineHeight);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::AlignAbsolute, QCoreApplication::translate("ShadDiffView", "%1 行未改动，点击展开").arg(row.foldCount));
}

void ShadDiffContent::fillTextRange(QPainter* painter, const QTextLayout& layout, qreal textLeft, qreal top, int from, int to, const QColor& color) const
{
    for (int visual = 0; visual < layout.lineCount(); ++visual)
    {
        const QTextLine line = layout.lineAt(visual);
        const int rangeFrom = qMax(from, line.textStart());
        const int rangeTo = qMin(to, line.textStart() + line.textLength());
        if (rangeTo <= rangeFrom)
        {
            continue;
        }
        const qreal startX = line.cursorToX(rangeFrom);
        const qreal endX = line.cursorToX(rangeTo);
        painter->fillRect(QRectF(textLeft + qMin(startX, endX), top + visual * kLineHeight, qAbs(endX - startX), kLineHeight), color);
    }
}
