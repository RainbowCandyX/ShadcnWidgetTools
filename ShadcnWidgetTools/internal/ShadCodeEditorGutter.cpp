#include "ShadCodeEditorGutter.h"

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextLayout>
#include <QtGlobal>

#include "ShadCodeEditor.h"
#include "ShadCodeEditorPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadCodeEditorGutter::ShadCodeEditorGutter(ShadCodeEditor* editor)
    : QWidget(editor), _editor(editor)
{
    setLayoutDirection(Qt::LeftToRight);
}

ShadCodeEditorGutter::~ShadCodeEditorGutter()
{
}

int ShadCodeEditorGutter::gutterWidth() const
{
    const int lastNumber = qMax(1, _editor->blockCount());
    const int numberWidth = ShadStyleHelper::textWidth(_editor->font(), QString::number(lastNumber));
    const int margin = qRound(_editor->document()->documentMargin());
    return ShadCodeEditorPrivate::kPaddingX + qMax(kLineNumberMinWidth, numberWidth) + kLineNumberGap - margin;
}

void ShadCodeEditorGutter::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setOpacity(_editor->isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
    const QFont font = _editor->font();
    painter.setFont(font);
    const QFontMetricsF metrics(font);
    const QColor mutedColor = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor currentColor = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor lineColor = ShadCodeEditorPrivate::currentLineColor();
    const bool isLineHighlighted = !_editor->isReadOnly();
    const QTextCursor cursor = _editor->textCursor();
    const int currentBlock = cursor.blockNumber();
    // gap-3 中有文档边距位于视口内
    const int margin = qRound(_editor->document()->documentMargin());
    const qreal numberRight = width() - (kLineNumberGap - margin);
    const QRect exposed = event->rect();
    const qreal rowHeight = _editor->d_func()->lineHeight();
    QTextBlock block = _editor->firstVisibleBlock();
    qreal top = _editor->blockBoundingGeometry(block).translated(_editor->contentOffset()).top();
    while (block.isValid() && top <= exposed.bottom())
    {
        const qreal height = _editor->blockBoundingRect(block).height();
        if (block.isVisible() && top + height >= exposed.top())
        {
            const bool isCurrent = block.blockNumber() == currentBlock;
            const QTextLayout* layout = block.layout();
            const bool hasLines = layout != nullptr && layout->lineCount() > 0;
            if (isCurrent && isLineHighlighted && hasLines)
            {
                const QTextLine cursorLine = layout->lineForTextPosition(cursor.positionInBlock());
                if (cursorLine.isValid())
                {
                    painter.fillRect(QRectF(0.0, top + cursorLine.lineNumber() * rowHeight, width(), rowHeight), lineColor);
                }
            }
            // 行号与文本块第一行文字的基线对齐
            const qreal baseline = hasLines ? top + layout->lineAt(0).y() + layout->lineAt(0).ascent() : top + metrics.ascent();
            const QString number = QString::number(block.blockNumber() + 1);
            painter.setPen(isCurrent ? currentColor : mutedColor);
            painter.drawText(QPointF(numberRight - metrics.horizontalAdvance(number), baseline), number);
        }
        top += height;
        block = block.next();
    }
}
