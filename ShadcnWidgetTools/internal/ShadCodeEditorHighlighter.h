#ifndef SHADCODEEDITORHIGHLIGHTER_H
#define SHADCODEEDITORHIGHLIGHTER_H

#include <QString>
#include <QSyntaxHighlighter>

#include "ShadCodeHighlighter.h"

class QTextDocument;

// ShadCodeEditor 的语法高亮：把 ShadCodeHighlighter 的逐行解析接到 QSyntaxHighlighter，配色与 ShadCodeBlock 相同
class ShadCodeEditorHighlighter : public QSyntaxHighlighter
{
    public:
        explicit ShadCodeEditorHighlighter(QTextDocument* document);
        ~ShadCodeEditorHighlighter() override;

        void setLanguage(ShadCodeHighlighter::Language language);
        ShadCodeHighlighter::Language getLanguage() const;

    protected:
        void highlightBlock(const QString& text) override;

    private:
        ShadCodeHighlighter::Language _language{ShadCodeHighlighter::Language::PlainText};
};

#endif // SHADCODEEDITORHIGHLIGHTER_H
