#include "ShadCodeEditorPrivate.h"

#include <QChar>
#include <QFont>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QMargins>
#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QRectF>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>
#include <QWidget>
#include <QtGlobal>
#include <QtMath>

#include <initializer_list>

#include "ShadCodeEditor.h"
#include "ShadCodeEditorDocumentLayout.h"
#include "ShadCodeEditorGutter.h"
#include "ShadTheme.h"

ShadCodeEditorPrivate::ShadCodeEditorPrivate()
{
}

ShadCodeEditorPrivate::~ShadCodeEditorPrivate()
{
}

QColor ShadCodeEditorPrivate::currentLineColor()
{
    return shadTheme->getColor(ShadThemeType::Muted);
}

void ShadCodeEditorPrivate::applyFont()
{
    Q_Q(ShadCodeEditor);
    QFont font = q->font();
    font.setFamilies(ShadStyleHelper::monospaceFontFamilies());
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    font.setPixelSize(kFontPixelSize);
    font.setWeight(QFont::Normal);
    q->setFont(font);
}

void ShadCodeEditorPrivate::applyThemeColors()
{
    Q_Q(ShadCodeEditor);
    using ShadStyleHelper::kDisabledOpacity;
    using ShadStyleHelper::withAlpha;
    const QColor transparent(0, 0, 0, 0);
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor placeholder = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor selectionText = shadTheme->getColor(ShadThemeType::PrimaryForeground);

    QPalette palette = q->palette();
    for (QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled})
    {
        const qreal alpha = group == QPalette::Disabled ? kDisabledOpacity : 1.0;
        palette.setColor(group, QPalette::Base, transparent);
        palette.setColor(group, QPalette::Window, transparent);
        palette.setColor(group, QPalette::Text, withAlpha(foreground, alpha));
        palette.setColor(group, QPalette::PlaceholderText, withAlpha(placeholder, alpha));
        // 基类选区只按文字高度绘制，背景改由 paintLineBackgrounds 自绘，这里只保留选中文字颜色
        palette.setColor(group, QPalette::Highlight, transparent);
        palette.setColor(group, QPalette::HighlightedText, selectionText);
    }
    q->setPalette(palette);
    q->update();
    q->viewport()->update();
}

void ShadCodeEditorPrivate::applyTabStop()
{
    Q_Q(ShadCodeEditor);
    const QFontMetricsF metrics(q->font());
    q->setTabStopDistance(_tabSize * metrics.horizontalAdvance(QChar(u' ')));
}

void ShadCodeEditorPrivate::updateLineHeight()
{
    Q_Q(ShadCodeEditor);
    const QFont font = q->font();
    const int pixelSize = QFontInfo(font).pixelSize();
    const int naturalHeight = qCeil(QFontMetricsF(font).lineSpacing());
    _documentLayout->setLineHeight(qMax(naturalHeight, qRound(pixelSize * kLineHeightRatio)));
}

int ShadCodeEditorPrivate::lineHeight() const
{
    return _documentLayout->lineHeight();
}

int ShadCodeEditorPrivate::documentMargin() const
{
    Q_Q(const ShadCodeEditor);
    return qRound(q->document()->documentMargin());
}

void ShadCodeEditorPrivate::updateViewportMargins()
{
    Q_Q(ShadCodeEditor);
    const int margin = documentMargin();
    const int start = _isLineNumbersVisible ? _gutter->gutterWidth() : kPaddingX - margin;
    const QMargins margins(start, kPaddingY - margin, kPaddingX - margin, kPaddingY - margin);
    // 边距不变时跳过，避免重新排布视口
    if (q->viewportMargins() != margins)
    {
        q->setViewportMargins(margins);
    }
    _gutter->setVisible(_isLineNumbersVisible);
    updateGutterGeometry();
}

void ShadCodeEditorPrivate::updateGutterGeometry()
{
    Q_Q(ShadCodeEditor);
    const QRect contents = q->contentsRect();
    const QRect viewportRect = q->viewport()->geometry();
    _gutter->setGeometry(contents.left(), viewportRect.top(), qMax(0, viewportRect.left() - contents.left()), viewportRect.height());
    _gutter->update();
}

void ShadCodeEditorPrivate::updateCurrentLine()
{
    Q_Q(ShadCodeEditor);
    q->viewport()->update();
    _gutter->update();
}

void ShadCodeEditorPrivate::paintLineBackgrounds(const QRect& exposed)
{
    Q_Q(ShadCodeEditor);
    const QTextCursor cursor = q->textCursor();
    const bool hasSelection = cursor.hasSelection();
    const bool isLineHighlighted = !q->isReadOnly();
    if (!hasSelection && !isLineHighlighted)
    {
        return;
    }
    QPainter painter(q->viewport());
    painter.setOpacity(q->isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
    const QColor lineColor = currentLineColor();
    const QColor selectionColor = shadTheme->getColor(ShadThemeType::Primary);
    const qreal rowHeight = lineHeight();
    const qreal viewportWidth = q->viewport()->width();
    const qreal spaceWidth = QFontMetricsF(q->font()).horizontalAdvance(QChar(u' '));
    const QPointF offset = q->contentOffset();
    const int selectionStart = cursor.selectionStart();
    const int selectionEnd = cursor.selectionEnd();
    const QTextBlock cursorBlock = cursor.block();
    int cursorLine = -1;
    if (cursorBlock.layout() != nullptr)
    {
        const QTextLine line = cursorBlock.layout()->lineForTextPosition(cursor.positionInBlock());
        cursorLine = line.isValid() ? line.lineNumber() : -1;
    }

    QTextBlock block = q->firstVisibleBlock();
    qreal top = q->blockBoundingGeometry(block).translated(offset).top();
    while (block.isValid() && top <= exposed.bottom())
    {
        const qreal height = q->blockBoundingRect(block).height();
        const QTextLayout* layout = block.layout();
        if (block.isVisible() && top + height >= exposed.top() && layout != nullptr)
        {
            const int blockStart = block.position();
            // 不含末尾的段落分隔符
            const int textLength = block.length() - 1;
            const int from = selectionStart - blockStart;
            const int to = selectionEnd - blockStart;
            for (int index = 0; index < layout->lineCount(); ++index)
            {
                const QTextLine line = layout->lineAt(index);
                const qreal rowTop = top + index * rowHeight;
                if (isLineHighlighted && block == cursorBlock && index == cursorLine)
                {
                    painter.fillRect(QRectF(0.0, rowTop, viewportWidth, rowHeight), lineColor);
                }
                if (!hasSelection)
                {
                    continue;
                }
                const int lineStart = line.textStart();
                const int lineEnd = lineStart + line.textLength();
                const int selectFrom = qMax(from, lineStart);
                const int selectTo = qMin(to, lineEnd);
                if (selectTo > selectFrom)
                {
                    const qreal x1 = offset.x() + line.cursorToX(selectFrom);
                    const qreal x2 = offset.x() + line.cursorToX(selectTo);
                    painter.fillRect(QRectF(qMin(x1, x2), rowTop, qAbs(x2 - x1), rowHeight), selectionColor);
                }
                // 选区延续到下一块时，行尾补画换行符位置
                const bool isLastLine = index == layout->lineCount() - 1;
                if (isLastLine && from <= textLength && to > textLength)
                {
                    painter.fillRect(QRectF(offset.x() + line.cursorToX(lineEnd), rowTop, spaceWidth, rowHeight), selectionColor);
                }
            }
        }
        top += height;
        block = block.next();
    }
}

QPointF ShadCodeEditorPrivate::mapToTextLine(const QPointF& position) const
{
    Q_Q(const ShadCodeEditor);
    // 基类只按文字本身的范围找行，这里修正落在行内空白中的纵坐标
    const QTextBlock block = q->cursorForPosition(position.toPoint()).block();
    const QTextLayout* layout = block.isValid() ? block.layout() : nullptr;
    if (layout == nullptr || !block.isVisible() || layout->lineCount() == 0)
    {
        return position;
    }
    const qreal top = q->blockBoundingGeometry(block).translated(q->contentOffset()).top();
    const int row = qFloor((position.y() - top) / lineHeight());
    if (row < 0 || row >= layout->lineCount())
    {
        return position;
    }
    const QTextLine line = layout->lineAt(row);
    const qreal lineTop = top + line.y();
    const qreal lineBottom = lineTop + line.height();
    return QPointF(position.x(), qBound(lineTop, position.y(), qMax(lineTop, lineBottom - 1.0)));
}

void ShadCodeEditorPrivate::paintFrame()
{
    Q_Q(ShadCodeEditor);
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(q->rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const bool isFocused = q->hasFocus();
    QPainter painter(q);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(q->isEnabled() ? 1.0 : kDisabledOpacity);
    drawShadowXs(&painter, bodyRect, radius);
    if (shadTheme->isDark())
    {
        fillRoundedRect(&painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3));
    }
    QColor borderColor = shadTheme->getColor(isFocused ? ShadThemeType::Ring : ShadThemeType::Input);
    if (_isInvalid)
    {
        borderColor = shadTheme->getColor(ShadThemeType::Destructive);
    }
    strokeRoundedRect(&painter, bodyRect, radius, borderColor);
    if (isFocused || _isInvalid)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(_isInvalid));
    }
}
