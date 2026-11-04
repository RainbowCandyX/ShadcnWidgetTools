#include "ShadCodeEditorHighlighter.h"

#include <QColor>
#include <QList>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QtGlobal>

#include "ShadTheme.h"

ShadCodeEditorHighlighter::ShadCodeEditorHighlighter(QTextDocument* document)
    : QSyntaxHighlighter(document)
{
}

ShadCodeEditorHighlighter::~ShadCodeEditorHighlighter()
{
}

void ShadCodeEditorHighlighter::setLanguage(ShadCodeHighlighter::Language language)
{
    if (_language == language)
    {
        return;
    }
    _language = language;
    rehighlight();
}

ShadCodeHighlighter::Language ShadCodeEditorHighlighter::getLanguage() const
{
    return _language;
}

void ShadCodeEditorHighlighter::highlightBlock(const QString& text)
{
    // previousBlockState 为 -1 时按初始状态 0 解析
    int state = qMax(0, previousBlockState());
    const QList<ShadCodeHighlighter::Span> spans = ShadCodeHighlighter::highlightLine(_language, text, state);
    const bool isDark = shadTheme->isDark();
    for (const ShadCodeHighlighter::Span& span : spans)
    {
        // Plain 片段颜色无效，沿用编辑器文字颜色
        const QColor color = ShadCodeHighlighter::tokenColor(span.kind, isDark);
        if (!color.isValid())
        {
            continue;
        }
        QTextCharFormat format;
        format.setForeground(color);
        setFormat(span.start, span.length, format);
    }
    setCurrentBlockState(state);
}
