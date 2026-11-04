#include "ShadCodeBlockView.h"

#include <QAction>
#include <QClipboard>
#include <QColor>
#include <QContextMenuEvent>
#include <QEvent>
#include <QFont>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QScrollArea>
#include <QTextCharFormat>
#include <QTextLine>
#include <QTextOption>
#include <QtGlobal>
#include <QtMath>

#include <algorithm>
#include <iterator>

#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadKbd.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // leading-relaxed
    constexpr qreal kLineHeightRatio = 1.625;
    // 行号 before:min-w-6，与代码之间 gap-x-3
    constexpr int kLineNumberMinWidth = 24;
    constexpr int kLineNumberGap = 12;
    // +/− 标记 after:start-1
    constexpr int kMarkerInset = 4;
    // 与浏览器默认的 tab-size 一致（Tailwind preflight 为 4）
    constexpr int kTabSize = 4;
    // 不换行时的行宽，足够容纳任意长度的一行
    constexpr qreal kNoWrapLineWidth = 1.0e6;
    // ANSI dim
    constexpr qreal kDimAlpha = 0.5;
    // bg-destructive/10
    constexpr qreal kRemovedLineAlpha = 0.1;
    // 拖动选择到视口边缘时保持的可见边距
    constexpr int kAutoScrollMargin = 4;

    bool isWordCharacter(QChar ch)
    {
        return ch.isLetterOrNumber() || ch == u'_';
    }
} // namespace

ShadCodeBlockView::ShadCodeBlockView(QWidget* parent)
    : QWidget(parent)
{
    // 代码内容始终从左到右，不继承父组件的布局方向
    setLayoutDirection(Qt::LeftToRight);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::IBeamCursor);
    _rawLines.append(QString());
    updateFontMetrics();
    rebuildFrom(0);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { relayoutAll(); });
}

ShadCodeBlockView::~ShadCodeBlockView()
{
}

void ShadCodeBlockView::setCode(const QString& code)
{
    _code = code;
    QString body = code;
    // 与 shadcn 一致：末尾的一个换行不产生额外的空行
    if (body.endsWith(u'\n'))
    {
        body.chop(1);
    }
    QStringList rawLines = body.split(u'\n');
    for (QString& line : rawLines)
    {
        if (line.endsWith(u'\r'))
        {
            line.chop(1);
        }
    }
    // 只重新处理第一处变化之后的行，流式追加时只有最后几行需要处理
    const int common = int(qMin(rawLines.size(), _rawLines.size()));
    int first = 0;
    while (first < common && rawLines.at(first) == _rawLines.at(first))
    {
        ++first;
    }
    if (first == common && rawLines.size() == _rawLines.size())
    {
        return;
    }
    _rawLines = rawLines;
    // 选择范围涉及变化的行时清除选择
    if (selectionEnd().line >= first)
    {
        _anchor = Position();
        _cursor = Position();
    }
    rebuildFrom(first);
}

QString ShadCodeBlockView::getCode() const
{
    return _code;
}

QString ShadCodeBlockView::plainText() const
{
    return _lines.join(u'\n');
}

int ShadCodeBlockView::lineCount() const
{
    return int(_lines.size());
}

void ShadCodeBlockView::setLanguage(const QString& language)
{
    if (_language == language)
    {
        return;
    }
    _language = language;
    _languageType = ShadCodeHighlighter::languageFromName(language);
    _anchor = Position();
    _cursor = Position();
    rebuildFrom(0);
}

QString ShadCodeBlockView::getLanguage() const
{
    return _language;
}

void ShadCodeBlockView::setLineVariant(int lineIndex, ShadCodeBlockType::LineVariant variant)
{
    if (lineIndex < 0)
    {
        return;
    }
    if (lineIndex >= _variants.size())
    {
        if (variant == ShadCodeBlockType::NormalLine)
        {
            return;
        }
        _variants.resize(lineIndex + 1, ShadCodeBlockType::NormalLine);
    }
    _variants[lineIndex] = variant;
    update();
}

ShadCodeBlockType::LineVariant ShadCodeBlockView::getLineVariant(int lineIndex) const
{
    if (lineIndex < 0 || lineIndex >= _variants.size())
    {
        return ShadCodeBlockType::NormalLine;
    }
    return _variants.at(lineIndex);
}

void ShadCodeBlockView::clearLineVariants()
{
    _variants.clear();
    update();
}

void ShadCodeBlockView::setLineNumbersVisible(bool visible)
{
    if (_isLineNumbersVisible == visible)
    {
        return;
    }
    _isLineNumbersVisible = visible;
    // 行号栏改变换行宽度时需要重新排版
    relayoutFrom(int(_layouts.size()));
}

bool ShadCodeBlockView::isLineNumbersVisible() const
{
    return _isLineNumbersVisible;
}

void ShadCodeBlockView::setStartLine(int line)
{
    if (_startLine == line)
    {
        return;
    }
    _startLine = line;
    // 行号位数变化会改变行号栏宽度
    relayoutFrom(int(_layouts.size()));
}

int ShadCodeBlockView::getStartLine() const
{
    return _startLine;
}

void ShadCodeBlockView::setWordWrap(bool wrap)
{
    if (_isWordWrap == wrap)
    {
        return;
    }
    _isWordWrap = wrap;
    relayoutAll();
}

bool ShadCodeBlockView::isWordWrap() const
{
    return _isWordWrap;
}

void ShadCodeBlockView::setFontPixelSize(int pixelSize)
{
    if (_fontPixelSize == pixelSize)
    {
        return;
    }
    _fontPixelSize = pixelSize;
    updateFontMetrics();
    relayoutAll();
}

void ShadCodeBlockView::setDestructive(bool destructive)
{
    if (_isDestructive == destructive)
    {
        return;
    }
    _isDestructive = destructive;
    // dim 样式的颜色由文字颜色推导，需要重新生成格式
    relayoutAll();
}

void ShadCodeBlockView::setPadding(int horizontal, int top, int bottom)
{
    if (_paddingX == horizontal && _paddingTop == top && _paddingBottom == bottom)
    {
        return;
    }
    _paddingX = horizontal;
    _paddingTop = top;
    _paddingBottom = bottom;
    relayoutAll();
}

void ShadCodeBlockView::setViewportWidth(int width)
{
    if (_viewportWidth == width)
    {
        return;
    }
    _viewportWidth = width;
    // 换行模式下宽度变化会触发全部重新排版，不换行时只更新内容宽度
    relayoutFrom(int(_layouts.size()));
}

int ShadCodeBlockView::contentHeight() const
{
    return qCeil(_contentBottom + _paddingBottom);
}

int ShadCodeBlockView::naturalWidth() const
{
    return qCeil(textLeft() + _maximumLineWidth + _paddingX);
}

QString ShadCodeBlockView::selectedText() const
{
    const Position start = selectionStart();
    const Position end = selectionEnd();
    if (!isBefore(start, end))
    {
        return QString();
    }
    QStringList parts;
    for (int index = start.line; index <= end.line && index < _lines.size(); ++index)
    {
        const QString& line = _lines.at(index);
        const int from = index == start.line ? start.column : 0;
        const int to = index == end.line ? end.column : int(line.size());
        parts.append(line.mid(from, qMax(0, to - from)));
    }
    return parts.join(u'\n');
}

bool ShadCodeBlockView::hasSelectedText() const
{
    return isBefore(selectionStart(), selectionEnd());
}

void ShadCodeBlockView::selectAll()
{
    if (_lines.isEmpty())
    {
        return;
    }
    _anchor = Position();
    _cursor.line = int(_lines.size()) - 1;
    _cursor.column = int(_lines.last().size());
    update();
    updateSelectionClipboard();
}

void ShadCodeBlockView::copySelection()
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard != nullptr && hasSelectedText())
    {
        clipboard->setText(selectedText());
    }
}

QSize ShadCodeBlockView::sizeHint() const
{
    return QSize(naturalWidth(), contentHeight());
}

bool ShadCodeBlockView::event(QEvent* event)
{
    // 获得焦点时由本组件处理复制与全选，不触发窗口中的同名快捷键
    if (event->type() == QEvent::ShortcutOverride)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->matches(QKeySequence::Copy) || keyEvent->matches(QKeySequence::SelectAll))
        {
            event->accept();
            return true;
        }
    }
    // 右键菜单只在第一次打开时创建，语言切换后丢弃，下次打开时按新语言重建
    if (event->type() == QEvent::LanguageChange && _contextMenu != nullptr)
    {
        _contextMenu->deleteLater();
        _contextMenu = nullptr;
        _copyAction = nullptr;
    }
    return QWidget::event(event);
}

void ShadCodeBlockView::paintEvent(QPaintEvent* event)
{
    if (_layouts.empty())
    {
        return;
    }
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    painter.setFont(_codeFont);
    const QColor baseColor = textColor();
    const QColor mutedColor = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor destructiveColor = shadTheme->getColor(ShadThemeType::Destructive);
    const QColor mutedBackground = shadTheme->getColor(ShadThemeType::Muted);
    const QColor selectionBackground = shadTheme->getColor(ShadThemeType::Primary);
    const QColor selectionText = shadTheme->getColor(ShadThemeType::PrimaryForeground);
    const QFontMetricsF metrics(_codeFont);
    const qreal left = textLeft();
    const qreal numberRight = _paddingX + gutterWidth() - kLineNumberGap;
    const Position start = selectionStart();
    const Position end = selectionEnd();
    const bool hasSelection = isBefore(start, end);
    const QRect exposed = event->rect();
    const int firstLine = lineIndexAt(exposed.top());
    const int lastLine = lineIndexAt(exposed.bottom());
    for (int index = firstLine; index <= lastLine; ++index)
    {
        QTextLayout* layout = _layouts.at(index).get();
        const qreal top = _lineTops.at(index);
        const qreal rowHeight = lineHeightOf(index);
        const qreal baseline = top + _halfLeading + _ascent;
        // 行背景横跨整行，包括左右内边距与行号栏
        const ShadCodeBlockType::LineVariant variant = getLineVariant(index);
        if (variant == ShadCodeBlockType::RemovedLine)
        {
            painter.fillRect(QRectF(0.0, top, width(), rowHeight), withAlpha(destructiveColor, kRemovedLineAlpha));
        }
        else if (variant != ShadCodeBlockType::NormalLine)
        {
            painter.fillRect(QRectF(0.0, top, width(), rowHeight), mutedBackground);
        }
        if (variant == ShadCodeBlockType::AddedLine || variant == ShadCodeBlockType::RemovedLine)
        {
            const bool isRemoved = variant == ShadCodeBlockType::RemovedLine;
            painter.setPen(isRemoved ? destructiveColor : mutedColor);
            painter.drawText(QPointF(kMarkerInset, baseline), isRemoved ? QStringLiteral("−") : QStringLiteral("+"));
        }
        if (_isLineNumbersVisible)
        {
            // 行号右对齐（text-end），只显示在折行后的第一行
            const QString number = QString::number(_startLine + index);
            painter.setPen(mutedColor);
            painter.drawText(QPointF(numberRight - metrics.horizontalAdvance(number), baseline), number);
        }

        // 选择背景按整行高度绘制，选中文字改为 primary-foreground
        QList<QTextLayout::FormatRange> selections;
        if (hasSelection && index >= start.line && index <= end.line)
        {
            const int from = index == start.line ? start.column : 0;
            const int to = index == end.line ? end.column : int(_lines.at(index).size());
            for (int visual = 0; visual < layout->lineCount(); ++visual)
            {
                const QTextLine line = layout->lineAt(visual);
                const int lineStart = line.textStart();
                const int lineEnd = lineStart + line.textLength();
                const int selectFrom = qMax(from, lineStart);
                const int selectTo = qMin(to, lineEnd);
                const qreal rowTop = top + visual * _lineHeight;
                if (selectTo > selectFrom)
                {
                    const qreal x1 = line.cursorToX(selectFrom);
                    const qreal x2 = line.cursorToX(selectTo);
                    painter.fillRect(QRectF(left + qMin(x1, x2), rowTop, qAbs(x2 - x1), _lineHeight), selectionBackground);
                }
                // 选择延续到下一行时，在行尾画出换行符的位置
                if (index < end.line && visual == layout->lineCount() - 1)
                {
                    painter.fillRect(QRectF(left + line.naturalTextWidth(), rowTop, _spaceWidth, _lineHeight), selectionBackground);
                }
            }
            if (to > from)
            {
                QTextLayout::FormatRange range;
                range.start = from;
                range.length = to - from;
                range.format.setForeground(selectionText);
                selections.append(range);
            }
        }
        painter.setPen(baseColor);
        layout->draw(&painter, QPointF(left, top), selections);
    }
}

void ShadCodeBlockView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    const Position position = positionAt(event->position());
    // Shift + 点击从原锚点扩展选择
    if (!(event->modifiers() & Qt::ShiftModifier))
    {
        _anchor = position;
    }
    _cursor = position;
    _isSelecting = true;
    update();
    event->accept();
}

void ShadCodeBlockView::mouseMoveEvent(QMouseEvent* event)
{
    if (!_isSelecting || !(event->buttons() & Qt::LeftButton))
    {
        QWidget::mouseMoveEvent(event);
        return;
    }
    _cursor = positionAt(event->position());
    ensurePointVisible(event->position());
    update();
    event->accept();
}

void ShadCodeBlockView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !_isSelecting)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    _isSelecting = false;
    updateSelectionClipboard();
    event->accept();
}

void ShadCodeBlockView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || _lines.isEmpty())
    {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }
    // 双击选中光标处的单词；不是单词字符时选中该字符
    const Position position = positionAt(event->position());
    const QString& text = _lines.at(position.line);
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
    _anchor.line = position.line;
    _anchor.column = from;
    _cursor.line = position.line;
    _cursor.column = to;
    _isSelecting = false;
    update();
    updateSelectionClipboard();
    event->accept();
}

void ShadCodeBlockView::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Copy))
    {
        copySelection();
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::SelectAll))
    {
        selectAll();
        event->accept();
        return;
    }
    // 方向键与翻页键交给外层滚动区域滚动
    QWidget::keyPressEvent(event);
}

void ShadCodeBlockView::contextMenuEvent(QContextMenuEvent* event)
{
    if (_contextMenu == nullptr)
    {
        _contextMenu = new ShadDropdownMenu(this);
        _copyAction = _contextMenu->addItem(tr("复制"), ShadIconType::Copy, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::Copy)));
        QAction* selectAllAction = _contextMenu->addItem(tr("全选"), ShadIconType::TextSelect, ShadKbdGroup::shortcutText(QKeySequence(QKeySequence::SelectAll)));
        connect(_copyAction, &QAction::triggered, this, &ShadCodeBlockView::copySelection);
        connect(selectAllAction, &QAction::triggered, this, &ShadCodeBlockView::selectAll);
    }
    _copyAction->setEnabled(hasSelectedText());
    _contextMenu->popupAt(event->globalPos());
    event->accept();
}

bool ShadCodeBlockView::isBefore(const Position& first, const Position& second)
{
    return first.line < second.line || (first.line == second.line && first.column < second.column);
}

void ShadCodeBlockView::rebuildFrom(int first)
{
    const int start = qBound(0, first, int(_lines.size()));
    _lines.resize(start);
    _spans.resize(start);
    _endStates.resize(start);
    int state = start > 0 ? _endStates.at(start - 1) : 0;
    for (int index = start; index < _rawLines.size(); ++index)
    {
        const QString& raw = _rawLines.at(index);
        if (_languageType == ShadCodeHighlighter::Language::Ansi)
        {
            // ANSI 颜色不延续到下一行，显示文字去掉转义码
            QString text;
            _spans.append(ShadCodeHighlighter::parseAnsiLine(raw, text));
            _lines.append(text);
            _endStates.append(0);
            continue;
        }
        _spans.append(ShadCodeHighlighter::highlightLine(_languageType, raw, state));
        _lines.append(raw);
        _endStates.append(state);
    }
    relayoutFrom(start);
}

void ShadCodeBlockView::relayoutFrom(int first)
{
    const qreal width = wrapWidth();
    int start = qBound(0, first, int(_layouts.size()));
    if (!qFuzzyCompare(width + 1.0, _layoutWrapWidth + 1.0))
    {
        start = 0;
    }
    _layoutWrapWidth = width;
    _layouts.resize(start);
    _lineTops.resize(start);
    qreal top = start > 0 ? _lineTops.at(start - 1) + lineHeightOf(start - 1) : qreal(_paddingTop);
    for (int index = start; index < _lines.size(); ++index)
    {
        _layouts.push_back(createLayout(index));
        _lineTops.append(top);
        top += lineHeightOf(index);
    }
    _contentBottom = top;
    _maximumLineWidth = 0.0;
    if (width > 0.0)
    {
        // 换行时排版宽度受视口限制，自然宽度按不换行的文字宽度估算，避免 sizeHint 随视口宽度收缩
        const QFontMetricsF metrics(_codeFont);
        for (const QString& line : _lines)
        {
            _maximumLineWidth = qMax(_maximumLineWidth, metrics.horizontalAdvance(line));
        }
    }
    else
    {
        for (const std::unique_ptr<QTextLayout>& layout : _layouts)
        {
            _maximumLineWidth = qMax(_maximumLineWidth, layout->maximumWidth());
        }
    }
    updateContentSize();
    update();
}

void ShadCodeBlockView::relayoutAll()
{
    _layoutWrapWidth = -1.0;
    relayoutFrom(0);
}

std::unique_ptr<QTextLayout> ShadCodeBlockView::createLayout(int lineIndex) const
{
    std::unique_ptr<QTextLayout> layout = std::make_unique<QTextLayout>(_lines.at(lineIndex), _codeFont);
    const qreal width = wrapWidth();
    QTextOption option;
    // wrap-break-word：优先在单词边界换行，单词过长时在任意位置断开
    option.setWrapMode(width > 0.0 ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    // 段落方向固定为从左到右，行内的阿拉伯文等由双向文字算法排列
    option.setTextDirection(Qt::LeftToRight);
    option.setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    option.setTabStopDistance(_spaceWidth * kTabSize);
    layout->setTextOption(option);
    layout->setFormats(lineFormats(lineIndex));
    layout->setCacheEnabled(true);
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
        // 每行占 leading-relaxed 的行高，文字在行内垂直居中（与 CSS 的半行距一致）
        line.setPosition(QPointF(0.0, count * _lineHeight + _halfLeading));
        ++count;
    }
    layout->endLayout();
    return layout;
}

QList<QTextLayout::FormatRange> ShadCodeBlockView::lineFormats(int lineIndex) const
{
    QList<QTextLayout::FormatRange> formats;
    const bool isDark = shadTheme->isDark();
    for (const ShadCodeHighlighter::Span& span : _spans.at(lineIndex))
    {
        QColor color = span.ansiColor >= 0 ? ShadCodeHighlighter::ansiColor(span.ansiColor, isDark) : ShadCodeHighlighter::tokenColor(span.kind, isDark);
        if (span.isDim)
        {
            color = ShadStyleHelper::withAlpha(color.isValid() ? color : textColor(), kDimAlpha);
        }
        QTextLayout::FormatRange range;
        range.start = span.start;
        range.length = span.length;
        if (color.isValid())
        {
            range.format.setForeground(color);
        }
        if (span.isBold)
        {
            range.format.setFontWeight(QFont::Bold);
        }
        formats.append(range);
    }
    return formats;
}

QColor ShadCodeBlockView::textColor() const
{
    return shadTheme->getColor(_isDestructive ? ShadThemeType::Destructive : ShadThemeType::Foreground);
}

void ShadCodeBlockView::updateFontMetrics()
{
    QFont font = this->font();
    // font-mono：ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace
    font.setFamilies(ShadStyleHelper::monospaceFontFamilies());
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    font.setPixelSize(_fontPixelSize);
    font.setWeight(QFont::Normal);
    font.setItalic(false);
    _codeFont = font;
    const QFontMetricsF metrics(_codeFont);
    _lineHeight = _fontPixelSize * kLineHeightRatio;
    _ascent = metrics.ascent();
    _halfLeading = (_lineHeight - metrics.height()) / 2.0;
    _spaceWidth = metrics.horizontalAdvance(QChar(u' '));
}

int ShadCodeBlockView::gutterWidth() const
{
    if (!_isLineNumbersVisible)
    {
        return 0;
    }
    // 行号列宽取最宽的行号，至少 min-w-6
    const int lastNumber = _startLine + qMax(1, int(_lines.size())) - 1;
    const int numberWidth = qMax(ShadStyleHelper::textWidth(_codeFont, QString::number(lastNumber)), ShadStyleHelper::textWidth(_codeFont, QString::number(_startLine)));
    return qMax(kLineNumberMinWidth, numberWidth) + kLineNumberGap;
}

qreal ShadCodeBlockView::textLeft() const
{
    return _paddingX + gutterWidth();
}

qreal ShadCodeBlockView::wrapWidth() const
{
    // 视口宽度未知时先按不换行排版，得到宽度后再重新排版
    if (!_isWordWrap || _viewportWidth <= 0)
    {
        return 0.0;
    }
    return qMax<qreal>(1.0, _viewportWidth - textLeft() - _paddingX);
}

void ShadCodeBlockView::updateContentSize()
{
    const int contentHeightValue = contentHeight();
    const int natural = naturalWidth();
    // 换行时内容宽度等于视口宽度；不换行时不小于视口宽度，行背景铺满整行（min-w-max）
    const int contentWidthValue = wrapWidth() > 0.0 ? _viewportWidth : qMax(_viewportWidth, natural);
    if (size() != QSize(contentWidthValue, contentHeightValue))
    {
        resize(contentWidthValue, contentHeightValue);
    }
    if (contentHeightValue != _lastContentHeight || natural != _lastNaturalWidth)
    {
        _lastContentHeight = contentHeightValue;
        _lastNaturalWidth = natural;
        updateGeometry();
        Q_EMIT contentSizeChanged();
    }
}

int ShadCodeBlockView::lineIndexAt(qreal y) const
{
    if (_lineTops.isEmpty())
    {
        return 0;
    }
    const auto it = std::upper_bound(_lineTops.cbegin(), _lineTops.cend(), y);
    const int index = int(std::distance(_lineTops.cbegin(), it)) - 1;
    return qBound(0, index, int(_lineTops.size()) - 1);
}

qreal ShadCodeBlockView::lineHeightOf(int lineIndex) const
{
    return qMax(1, _layouts.at(lineIndex)->lineCount()) * _lineHeight;
}

ShadCodeBlockView::Position ShadCodeBlockView::positionAt(const QPointF& point) const
{
    Position position;
    if (_layouts.empty() || point.y() < _lineTops.first())
    {
        return position;
    }
    if (point.y() >= _contentBottom)
    {
        position.line = int(_lines.size()) - 1;
        position.column = int(_lines.last().size());
        return position;
    }
    const int index = lineIndexAt(point.y());
    const QTextLayout* layout = _layouts.at(index).get();
    position.line = index;
    if (layout->lineCount() <= 0)
    {
        return position;
    }
    const int visual = qBound(0, int((point.y() - _lineTops.at(index)) / _lineHeight), layout->lineCount() - 1);
    position.column = layout->lineAt(visual).xToCursor(point.x() - textLeft(), QTextLine::CursorBetweenCharacters);
    return position;
}

ShadCodeBlockView::Position ShadCodeBlockView::selectionStart() const
{
    return isBefore(_cursor, _anchor) ? _cursor : _anchor;
}

ShadCodeBlockView::Position ShadCodeBlockView::selectionEnd() const
{
    return isBefore(_cursor, _anchor) ? _anchor : _cursor;
}

void ShadCodeBlockView::ensurePointVisible(const QPointF& point)
{
    // 本组件是 QScrollArea 视口的子组件
    QWidget* viewport = parentWidget();
    QScrollArea* scrollArea = qobject_cast<QScrollArea*>(viewport != nullptr ? viewport->parentWidget() : nullptr);
    if (scrollArea != nullptr)
    {
        scrollArea->ensureVisible(qRound(point.x()), qRound(point.y()), kAutoScrollMargin, kAutoScrollMargin);
    }
}

void ShadCodeBlockView::updateSelectionClipboard()
{
    // X11 等平台的选择剪贴板：选中即可用鼠标中键粘贴
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard != nullptr && clipboard->supportsSelection() && hasSelectedText())
    {
        clipboard->setText(selectedText(), QClipboard::Selection);
    }
}
