#ifndef SHADCODEEDITORDOCUMENTLAYOUT_H
#define SHADCODEEDITORDOCUMENTLAYOUT_H

#include <QPlainTextDocumentLayout>
#include <QRectF>

class QTextBlock;
class QTextDocument;

// ShadCodeEditor 的文档排版：每行占固定行高，文字在行内垂直居中
// QPlainTextEdit 使用文本块行位置前都会先调用 blockBoundingRect，因此在其中重新放置各行
class ShadCodeEditorDocumentLayout : public QPlainTextDocumentLayout
{
    Q_OBJECT

    public:
        explicit ShadCodeEditorDocumentLayout(QTextDocument* document);
        ~ShadCodeEditorDocumentLayout() override;

        void setLineHeight(int height);
        int lineHeight() const;

        QRectF blockBoundingRect(const QTextBlock& block) const override;

    private:
        int _lineHeight{23};
};

#endif // SHADCODEEDITORDOCUMENTLAYOUT_H
