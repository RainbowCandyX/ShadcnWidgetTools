#ifndef SHADCODEHIGHLIGHTER_H
#define SHADCODEHIGHLIGHTER_H

#include <QColor>
#include <QList>
#include <QString>

// ShadCodeBlock 使用的轻量语法高亮，不对外导出
// 按行解析，多行注释与多行字符串通过行状态延续到下一行；配色取 Shiki 的 GitHub Light / GitHub Dark 主题
namespace ShadCodeHighlighter
{
    enum class Language
    {
        PlainText = 0,
        Ansi,
        Bash,
        Css,
        Cpp,
        Json,
        JavaScript,
        TypeScript,
        Python,
    };

    // 着色类别，与 GitHub 主题的颜色一一对应
    enum class TokenKind
    {
        Plain = 0,
        Keyword,
        String,
        Constant,
        Comment,
        Function,
        Variable,
        Tag,
    };

    // 一行中的着色片段，start / length 为行内字符位置
    struct Span
    {
        int start{0};
        int length{0};
        TokenKind kind{TokenKind::Plain};
        // ANSI 前景色序号 0-15，-1 表示默认颜色
        int ansiColor{-1};
        bool isBold{false};
        bool isDim{false};
    };

    // 语言名称（不区分大小写，支持 ts、js、sh、py 等别名）转换为语言；未支持的语言返回 PlainText
    Language languageFromName(const QString& name);

    // 解析一行代码：state 传入上一行结束时的状态（第一行为 0），返回时写入本行结束时的状态
    QList<Span> highlightLine(Language language, const QString& line, int& state);

    // 解析一行中的 ANSI SGR 转义码：text 写入去掉转义码后的文字；颜色与样式不延续到下一行
    QList<Span> parseAnsiLine(const QString& rawLine, QString& text);
    // 去掉全部 ANSI 转义码，用于复制
    QString stripAnsi(const QString& text);

    // 类别的颜色；Plain 返回无效颜色，表示使用组件的文字颜色
    QColor tokenColor(TokenKind kind, bool isDark);
    // ANSI 0-15 号前景色
    QColor ansiColor(int index, bool isDark);
} // namespace ShadCodeHighlighter

#endif // SHADCODEHIGHLIGHTER_H
