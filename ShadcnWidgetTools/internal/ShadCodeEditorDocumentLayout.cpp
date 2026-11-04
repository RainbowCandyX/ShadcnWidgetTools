#include "ShadCodeEditorDocumentLayout.h"

#include <QPointF>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QtGlobal>

ShadCodeEditorDocumentLayout::ShadCodeEditorDocumentLayout(QTextDocument* document)
    : QPlainTextDocumentLayout(document)
{
}

ShadCodeEditorDocumentLayout::~ShadCodeEditorDocumentLayout()
{
}

void ShadCodeEditorDocumentLayout::setLineHeight(int height)
{
    const int lineHeight = qMax(1, height);
    if (_lineHeight == lineHeight)
    {
        return;
    }
    _lineHeight = lineHeight;
    // 通知编辑器按新的块高度重算滚动范围
    Q_EMIT documentSizeChanged(documentSize());
    requestUpdate();
}

int ShadCodeEditorDocumentLayout::lineHeight() const
{
    return _lineHeight;
}

QRectF ShadCodeEditorDocumentLayout::blockBoundingRect(const QTextBlock& block) const
{
    // 基类负责未排版文本块的排版
    QRectF rect = QPlainTextDocumentLayout::blockBoundingRect(block);
    if (!block.isValid() || !block.isVisible())
    {
        return rect;
    }
    QTextLayout* layout = block.layout();
    const int lineCount = layout->lineCount();
    for (int index = 0; index < lineCount; ++index)
    {
        QTextLine line = layout->lineAt(index);
        // 垂直居中（同 CSS 半行距），取整避免半像素；字体高于行高时贴齐行顶
        const qreal halfLeading = qMax(0, qRound((_lineHeight - line.height()) / 2.0));
        line.setPosition(QPointF(line.x(), index * _lineHeight + halfLeading));
    }
    const qreal bottomMargin = block.next().isValid() ? 0.0 : document()->documentMargin();
    rect.setHeight(lineCount * _lineHeight + bottomMargin);
    return rect;
}
