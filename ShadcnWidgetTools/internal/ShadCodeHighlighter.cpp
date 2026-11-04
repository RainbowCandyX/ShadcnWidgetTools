#include "ShadCodeHighlighter.h"

#include <QChar>
#include <QColor>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <initializer_list>

namespace
{
    using ShadCodeHighlighter::Language;
    using ShadCodeHighlighter::Span;
    using ShadCodeHighlighter::TokenKind;

    // 行状态：低 8 位为跨行结构，CSS 在高位记录花括号深度
    constexpr int kStateNormal = 0;
    constexpr int kStateBlockComment = 1;
    constexpr int kStateTemplateString = 2;
    constexpr int kStateTripleSingle = 3;
    constexpr int kStateTripleDouble = 4;
    constexpr int kStateModeMask = 0xFF;
    constexpr int kStateDepthShift = 8;

    constexpr QChar kEscape(u'\x1b');

    // 通用词法规则：关键字、常量与各语言的语法特性
    struct Rules
    {
        QSet<QString> keywords;
        QSet<QString> constants;
        // 紧跟的下一个标识符着色为函数名，例如 def / function / class
        QSet<QString> definitionWords;
        bool hasSlashComment{false};
        bool hasHashComment{false};
        bool hasTemplateString{false};
        bool hasTripleQuote{false};
        bool hasPreprocessor{false};
        bool hasJsxTags{false};
        bool hasDollarIdentifier{false};
        bool isShell{false};
    };

    QSet<QString> toSet(std::initializer_list<const char*> words)
    {
        QSet<QString> result;
        for (const char* word : words)
        {
            result.insert(QString::fromLatin1(word));
        }
        return result;
    }

    const QSet<QString>& javaScriptKeywords()
    {
        static const QSet<QString> words = toSet({"as", "async", "await", "break", "case", "catch", "class", "const", "continue", "debugger", "default", "delete", "do", "else", "export", "extends", "finally", "for", "from", "function", "get", "if", "import", "in", "instanceof", "let", "new", "of", "return", "set", "static", "switch", "throw", "try", "typeof", "var", "void", "while", "with", "yield"});
        return words;
    }

    const QSet<QString>& javaScriptConstants()
    {
        static const QSet<QString> words = toSet({"true", "false", "null", "undefined", "NaN", "Infinity", "this", "super", "console", "window", "document"});
        return words;
    }

    const Rules& javaScriptRules()
    {
        static const Rules rules = []()
        {
            Rules result;
            result.keywords = javaScriptKeywords();
            result.constants = javaScriptConstants();
            result.definitionWords = toSet({"function", "class"});
            result.hasSlashComment = true;
            result.hasTemplateString = true;
            result.hasJsxTags = true;
            result.hasDollarIdentifier = true;
            return result;
        }();
        return rules;
    }

    const Rules& typeScriptRules()
    {
        static const Rules rules = []()
        {
            Rules result = javaScriptRules();
            result.keywords.unite(toSet({"abstract", "declare", "enum", "implements", "infer", "interface", "is", "keyof", "namespace", "private", "protected", "public", "readonly", "satisfies", "type"}));
            result.constants.unite(toSet({"any", "bigint", "boolean", "never", "number", "object", "string", "symbol", "unknown"}));
            result.definitionWords.unite(toSet({"interface", "type", "enum"}));
            return result;
        }();
        return rules;
    }

    const Rules& pythonRules()
    {
        static const Rules rules = []()
        {
            Rules result;
            result.keywords = toSet({"and", "as", "assert", "async", "await", "break", "case", "class", "continue", "def", "del", "elif", "else", "except", "finally", "for", "from", "global", "if", "import", "in", "is", "lambda", "match", "nonlocal", "not", "or", "pass", "raise", "return", "try", "while", "with", "yield"});
            result.constants = toSet({"True", "False", "None", "self", "cls", "print", "len", "range", "str", "int", "float", "list", "dict", "set", "tuple", "bool", "type", "isinstance", "open", "super", "enumerate", "zip", "map", "filter", "sorted", "min", "max", "sum", "abs"});
            result.definitionWords = toSet({"def", "class"});
            result.hasHashComment = true;
            result.hasTripleQuote = true;
            return result;
        }();
        return rules;
    }

    const Rules& cppRules()
    {
        static const Rules rules = []()
        {
            Rules result;
            result.keywords = toSet({"alignas", "alignof", "auto", "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class", "co_await", "co_return", "co_yield", "concept", "const", "const_cast", "consteval", "constexpr", "constinit", "continue", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "final", "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "operator", "override", "private", "protected", "public", "register", "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "template", "thread_local", "throw", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "Q_OBJECT", "Q_PROPERTY", "Q_SIGNALS", "Q_SLOTS", "Q_EMIT", "Q_INVOKABLE", "signals", "slots", "emit"});
            result.constants = toSet({"true", "false", "nullptr", "NULL", "this"});
            result.definitionWords = toSet({"class", "struct", "namespace", "enum"});
            result.hasSlashComment = true;
            result.hasPreprocessor = true;
            return result;
        }();
        return rules;
    }

    const Rules& shellRules()
    {
        static const Rules rules = []()
        {
            Rules result;
            result.keywords = toSet({"if", "then", "else", "elif", "fi", "for", "in", "do", "done", "while", "until", "case", "esac", "function", "return", "export", "local", "select"});
            result.definitionWords = toSet({"function"});
            result.hasHashComment = true;
            result.isShell = true;
            return result;
        }();
        return rules;
    }

    // 这些关键字之后的词仍处于命令位置，例如 then 之后的命令名
    const QSet<QString>& shellCommandKeywords()
    {
        static const QSet<QString> words = toSet({"if", "then", "else", "elif", "do", "while", "until", "sudo", "exec", "time"});
        return words;
    }

    void addSpan(QList<Span>& spans, int start, int end, TokenKind kind)
    {
        if (end <= start || kind == TokenKind::Plain)
        {
            return;
        }
        Span span;
        span.start = start;
        span.length = end - start;
        span.kind = kind;
        spans.append(span);
    }

    // 从 from 开始查找未转义的 quote，返回其位置，找不到返回 -1
    int findQuote(const QString& line, int from, QChar quote)
    {
        const int length = int(line.size());
        for (int index = from; index < length; ++index)
        {
            const QChar ch = line.at(index);
            if (ch == u'\\')
            {
                ++index;
                continue;
            }
            if (ch == quote)
            {
                return index;
            }
        }
        return -1;
    }

    int skipSpaces(const QString& line, int from)
    {
        const int length = int(line.size());
        int index = from;
        while (index < length && line.at(index).isSpace())
        {
            ++index;
        }
        return index;
    }

    bool isShellSpecial(QChar ch)
    {
        return ch == u'|' || ch == u';' || ch == u'&' || ch == u'<' || ch == u'>' || ch == u'(' || ch == u')' || ch == u'$' || ch == u'"' || ch == u'\'' || ch == u'`' || ch == u'\\';
    }

    bool isWordStart(const Rules& rules, QChar ch)
    {
        if (rules.isShell)
        {
            return !ch.isSpace() && !isShellSpecial(ch) && ch != u'#';
        }
        return ch.isLetter() || ch == u'_' || (rules.hasDollarIdentifier && ch == u'$');
    }

    bool isWordChar(const Rules& rules, QChar ch)
    {
        if (rules.isShell)
        {
            return !ch.isSpace() && !isShellSpecial(ch);
        }
        return ch.isLetterOrNumber() || ch == u'_' || (rules.hasDollarIdentifier && ch == u'$');
    }

    bool isAllDigits(const QString& word)
    {
        for (const QChar ch : word)
        {
            if (!ch.isDigit())
            {
                return false;
            }
        }
        return !word.isEmpty();
    }

    // Python 字符串前缀：r、b、f、u 及其组合
    bool isStringPrefix(const QString& word)
    {
        for (const QChar ch : word)
        {
            const QChar lower = ch.toLower();
            if (lower != u'r' && lower != u'b' && lower != u'f' && lower != u'u')
            {
                return false;
            }
        }
        return !word.isEmpty() && word.size() <= 2;
    }

    // "<" 是否为 JSX 标签开头：位于行首、运算符或 return 之后；跟在标识符、数字或右括号后面时为比较或泛型
    bool isJsxContext(const QString& line, int index, const QString& previousWord)
    {
        int previous = index - 1;
        while (previous >= 0 && line.at(previous).isSpace())
        {
            --previous;
        }
        if (previous < 0)
        {
            return true;
        }
        const QChar ch = line.at(previous);
        if (ch.isLetterOrNumber() || ch == u'_' || ch == u')' || ch == u']')
        {
            return previousWord == QStringLiteral("return");
        }
        return true;
    }

    // 延续上一行未结束的注释或字符串，返回本行继续解析的位置；整行都属于该结构时返回 -1
    int continueMultiline(const QString& line, int& state, QList<Span>& spans)
    {
        const int length = int(line.size());
        QString terminator;
        int extra = 0;
        TokenKind kind = TokenKind::String;
        if (state == kStateBlockComment)
        {
            terminator = QStringLiteral("*/");
            kind = TokenKind::Comment;
        }
        else if (state == kStateTripleSingle)
        {
            terminator = QStringLiteral("'''");
        }
        else if (state == kStateTripleDouble)
        {
            terminator = QStringLiteral("\"\"\"");
        }
        else if (state == kStateTemplateString)
        {
            const int end = findQuote(line, 0, u'`');
            if (end < 0)
            {
                addSpan(spans, 0, length, TokenKind::String);
                return -1;
            }
            addSpan(spans, 0, end + 1, TokenKind::String);
            state = kStateNormal;
            return end + 1;
        }
        else
        {
            state = kStateNormal;
            return 0;
        }
        extra = int(terminator.size());
        const int end = int(line.indexOf(terminator));
        if (end < 0)
        {
            addSpan(spans, 0, length, kind);
            return -1;
        }
        addSpan(spans, 0, end + extra, kind);
        state = kStateNormal;
        return end + extra;
    }

    QList<Span> lexGeneric(const Rules& rules, const QString& line, int& state)
    {
        QList<Span> spans;
        const int length = int(line.size());
        int index = continueMultiline(line, state, spans);
        if (index < 0)
        {
            return spans;
        }
        // Shell 中每段命令的第一个词为命令名
        bool isCommandPosition = rules.isShell;
        // 位于 JSX 开始标签内，标识符后跟 "=" 时为属性名
        bool isInTag = false;
        QString previousWord;
        while (index < length)
        {
            const QChar ch = line.at(index);
            const QChar next = index + 1 < length ? line.at(index + 1) : QChar();
            if (ch.isSpace())
            {
                ++index;
                continue;
            }
            if (rules.hasSlashComment && ch == u'/' && next == u'/')
            {
                addSpan(spans, index, length, TokenKind::Comment);
                return spans;
            }
            if (rules.hasSlashComment && ch == u'/' && next == u'*')
            {
                const int end = int(line.indexOf(QStringLiteral("*/"), index + 2));
                if (end < 0)
                {
                    addSpan(spans, index, length, TokenKind::Comment);
                    state = kStateBlockComment;
                    return spans;
                }
                addSpan(spans, index, end + 2, TokenKind::Comment);
                index = end + 2;
                continue;
            }
            if (rules.hasPreprocessor && ch == u'#' && line.left(index).trimmed().isEmpty())
            {
                // 预处理指令：#include <...> 的头文件名按字符串着色
                int end = skipSpaces(line, index + 1);
                while (end < length && line.at(end).isLetter())
                {
                    ++end;
                }
                addSpan(spans, index, end, TokenKind::Keyword);
                const int argument = skipSpaces(line, end);
                if (argument < length && line.at(argument) == u'<')
                {
                    const int close = int(line.indexOf(u'>', argument + 1));
                    if (close > argument)
                    {
                        addSpan(spans, argument, close + 1, TokenKind::String);
                        index = close + 1;
                        continue;
                    }
                }
                index = end;
                continue;
            }
            // Shell 中 # 只在词首表示注释，例如 a#b 不是注释
            if (rules.hasHashComment && ch == u'#' && (!rules.isShell || index == 0 || line.at(index - 1).isSpace()))
            {
                addSpan(spans, index, length, TokenKind::Comment);
                return spans;
            }
            if (rules.hasTripleQuote && (ch == u'\'' || ch == u'"') && line.mid(index, 3) == QString(3, ch))
            {
                const int end = int(line.indexOf(QString(3, ch), index + 3));
                if (end < 0)
                {
                    addSpan(spans, index, length, TokenKind::String);
                    state = ch == u'\'' ? kStateTripleSingle : kStateTripleDouble;
                    return spans;
                }
                addSpan(spans, index, end + 3, TokenKind::String);
                index = end + 3;
                isCommandPosition = false;
                continue;
            }
            if (!isInTag && (ch == u'"' || ch == u'\'' || (rules.hasTemplateString && ch == u'`')))
            {
                const int end = findQuote(line, index + 1, ch);
                if (end < 0)
                {
                    // 模板字符串可以跨行；普通字符串没有闭合时着色到行尾
                    addSpan(spans, index, length, TokenKind::String);
                    if (ch == u'`')
                    {
                        state = kStateTemplateString;
                    }
                    return spans;
                }
                addSpan(spans, index, end + 1, TokenKind::String);
                index = end + 1;
                isCommandPosition = false;
                continue;
            }
            if (isInTag && (ch == u'"' || ch == u'\''))
            {
                // JSX 属性值
                const int end = findQuote(line, index + 1, ch);
                const int stop = end < 0 ? length : end + 1;
                addSpan(spans, index, stop, TokenKind::String);
                index = stop;
                continue;
            }
            if (rules.isShell && ch == u'$')
            {
                // $NAME、${NAME}、$1、$? 为变量；后面是空格时为命令行提示符
                if (next == u'{')
                {
                    const int close = int(line.indexOf(u'}', index + 2));
                    const int stop = close < 0 ? length : close + 1;
                    addSpan(spans, index, stop, TokenKind::Variable);
                    index = stop;
                    isCommandPosition = false;
                    continue;
                }
                if (next.isLetterOrNumber() || next == u'_' || next == u'?' || next == u'@' || next == u'#' || next == u'*')
                {
                    int end = index + 2;
                    if (next.isLetter() || next == u'_')
                    {
                        while (end < length && (line.at(end).isLetterOrNumber() || line.at(end) == u'_'))
                        {
                            ++end;
                        }
                    }
                    addSpan(spans, index, end, TokenKind::Variable);
                    index = end;
                    isCommandPosition = false;
                    continue;
                }
                ++index;
                continue;
            }
            if (rules.hasTripleQuote && ch == u'@' && (next.isLetter() || next == u'_'))
            {
                // Python 装饰器
                int end = index + 1;
                while (end < length && (line.at(end).isLetterOrNumber() || line.at(end) == u'_' || line.at(end) == u'.'))
                {
                    ++end;
                }
                addSpan(spans, index, end, TokenKind::Function);
                index = end;
                continue;
            }
            if (rules.hasJsxTags && ch == u'<' && (next.isLetter() || next == u'/' || next == u'>') && isJsxContext(line, index, previousWord))
            {
                int start = index + 1;
                if (start < length && line.at(start) == u'/')
                {
                    ++start;
                }
                int end = start;
                while (end < length && (line.at(end).isLetterOrNumber() || line.at(end) == u'.' || line.at(end) == u'-' || line.at(end) == u'_'))
                {
                    ++end;
                }
                addSpan(spans, start, end, TokenKind::Tag);
                isInTag = true;
                previousWord.clear();
                index = end;
                continue;
            }
            if (!rules.isShell && (ch.isDigit() || (ch == u'.' && next.isDigit())))
            {
                int end = index + 1;
                while (end < length && (line.at(end).isLetterOrNumber() || line.at(end) == u'.' || line.at(end) == u'_'))
                {
                    ++end;
                }
                addSpan(spans, index, end, TokenKind::Constant);
                previousWord.clear();
                index = end;
                continue;
            }
            if (isWordStart(rules, ch))
            {
                int end = index + 1;
                while (end < length && isWordChar(rules, line.at(end)))
                {
                    ++end;
                }
                const QString word = line.mid(index, end - index);
                if (rules.hasTripleQuote && end < length && (line.at(end) == u'"' || line.at(end) == u'\'') && isStringPrefix(word))
                {
                    // 前缀与紧跟的字符串一起着色，字符串本身在下一轮处理
                    addSpan(spans, index, end, TokenKind::String);
                    index = end;
                    continue;
                }
                TokenKind kind = TokenKind::Plain;
                if (isInTag)
                {
                    kind = end < length && line.at(end) == u'=' ? TokenKind::Function : TokenKind::Plain;
                }
                else if (rules.keywords.contains(word))
                {
                    kind = TokenKind::Keyword;
                }
                else if (rules.constants.contains(word))
                {
                    kind = TokenKind::Constant;
                }
                else if (rules.definitionWords.contains(previousWord))
                {
                    kind = TokenKind::Function;
                }
                else if (rules.isShell && isCommandPosition)
                {
                    kind = TokenKind::Function;
                }
                else if (rules.isShell && isAllDigits(word))
                {
                    kind = TokenKind::Constant;
                }
                else if (!rules.isShell)
                {
                    const int after = skipSpaces(line, end);
                    if (after < length && line.at(after) == u'(')
                    {
                        kind = TokenKind::Function;
                    }
                }
                addSpan(spans, index, end, kind);
                isCommandPosition = rules.isShell && shellCommandKeywords().contains(word);
                previousWord = word;
                index = end;
                continue;
            }
            if (isInTag && ch == u'>')
            {
                isInTag = false;
            }
            if (rules.isShell)
            {
                // 管道、命令分隔符与子 shell 之后是新命令
                isCommandPosition = ch == u'|' || ch == u';' || ch == u'&' || ch == u'(' || ch == u'`';
            }
            previousWord.clear();
            ++index;
        }
        return spans;
    }

    bool isCssWordChar(QChar ch)
    {
        return ch.isLetterOrNumber() || ch == u'-' || ch == u'_';
    }

    int consumeCssWord(const QString& line, int from)
    {
        const int length = int(line.size());
        int end = from;
        while (end < length && isCssWordChar(line.at(end)))
        {
            ++end;
        }
        return end;
    }

    // 当前位置之后先出现 "{" 时为选择器（嵌套规则或多选择器），先出现 ";" 或 "}" 时为声明
    bool isCssSelectorAhead(const QString& line, int from)
    {
        const int length = int(line.size());
        for (int index = from; index < length; ++index)
        {
            const QChar ch = line.at(index);
            if (ch == u'{')
            {
                return true;
            }
            if (ch == u';' || ch == u'}')
            {
                return false;
            }
        }
        return false;
    }

    QList<Span> lexCss(const QString& line, int& state)
    {
        QList<Span> spans;
        const int length = int(line.size());
        int depth = state >> kStateDepthShift;
        int mode = state & kStateModeMask;
        int index = 0;
        if (mode == kStateBlockComment)
        {
            const int end = int(line.indexOf(QStringLiteral("*/")));
            if (end < 0)
            {
                addSpan(spans, 0, length, TokenKind::Comment);
                return spans;
            }
            addSpan(spans, 0, end + 2, TokenKind::Comment);
            index = end + 2;
            mode = kStateNormal;
        }
        // 声明中冒号之后为属性值
        bool isValue = false;
        while (index < length)
        {
            const QChar ch = line.at(index);
            const QChar next = index + 1 < length ? line.at(index + 1) : QChar();
            if (ch.isSpace())
            {
                ++index;
                continue;
            }
            if (ch == u'/' && next == u'*')
            {
                const int end = int(line.indexOf(QStringLiteral("*/"), index + 2));
                if (end < 0)
                {
                    addSpan(spans, index, length, TokenKind::Comment);
                    state = kStateBlockComment | (depth << kStateDepthShift);
                    return spans;
                }
                addSpan(spans, index, end + 2, TokenKind::Comment);
                index = end + 2;
                continue;
            }
            if (ch == u'"' || ch == u'\'')
            {
                const int end = findQuote(line, index + 1, ch);
                const int stop = end < 0 ? length : end + 1;
                addSpan(spans, index, stop, TokenKind::String);
                index = stop;
                continue;
            }
            if (ch == u'{' || ch == u'}' || ch == u';')
            {
                depth = ch == u'{' ? depth + 1 : (ch == u'}' ? qMax(0, depth - 1) : depth);
                isValue = false;
                ++index;
                continue;
            }
            if (ch == u'@')
            {
                const int end = consumeCssWord(line, index + 1);
                addSpan(spans, index, end, TokenKind::Keyword);
                index = qMax(end, index + 1);
                continue;
            }
            const bool isSelector = !isValue && (depth == 0 || isCssSelectorAhead(line, index));
            if (isSelector)
            {
                // .class、#id 与 :hover 为 entity 色，元素名为 tag 色
                if ((ch == u'.' || ch == u'#' || ch == u':') && isCssWordChar(next))
                {
                    const int end = consumeCssWord(line, index + 1);
                    addSpan(spans, index, end, TokenKind::Function);
                    index = end;
                    continue;
                }
                if (ch.isLetter())
                {
                    const int end = consumeCssWord(line, index);
                    addSpan(spans, index, end, TokenKind::Tag);
                    index = end;
                    continue;
                }
                ++index;
                continue;
            }
            if (ch == u':' && !isValue)
            {
                isValue = true;
                ++index;
                continue;
            }
            if (!isValue && (ch.isLetter() || ch == u'-'))
            {
                const int end = consumeCssWord(line, index);
                addSpan(spans, index, end, TokenKind::Constant);
                index = qMax(end, index + 1);
                continue;
            }
            if (isValue && ch == u'!')
            {
                const int end = consumeCssWord(line, index + 1);
                addSpan(spans, index, end, TokenKind::Keyword);
                index = end;
                continue;
            }
            if (isValue && (ch == u'#' || isCssWordChar(ch) || (ch == u'.' && next.isDigit())))
            {
                // 数值、单位、颜色与关键字值
                int end = index + 1;
                while (end < length && (isCssWordChar(line.at(end)) || line.at(end) == u'.' || line.at(end) == u'%'))
                {
                    ++end;
                }
                addSpan(spans, index, end, TokenKind::Constant);
                index = end;
                continue;
            }
            ++index;
        }
        state = mode | (depth << kStateDepthShift);
        return spans;
    }

    QList<Span> lexJson(const QString& line, int& state)
    {
        QList<Span> spans;
        const int length = int(line.size());
        int index = 0;
        if (state == kStateBlockComment)
        {
            const int end = int(line.indexOf(QStringLiteral("*/")));
            if (end < 0)
            {
                addSpan(spans, 0, length, TokenKind::Comment);
                return spans;
            }
            addSpan(spans, 0, end + 2, TokenKind::Comment);
            index = end + 2;
        }
        state = kStateNormal;
        while (index < length)
        {
            const QChar ch = line.at(index);
            const QChar next = index + 1 < length ? line.at(index + 1) : QChar();
            if (ch == u'/' && next == u'/')
            {
                addSpan(spans, index, length, TokenKind::Comment);
                return spans;
            }
            if (ch == u'/' && next == u'*')
            {
                const int end = int(line.indexOf(QStringLiteral("*/"), index + 2));
                if (end < 0)
                {
                    addSpan(spans, index, length, TokenKind::Comment);
                    state = kStateBlockComment;
                    return spans;
                }
                addSpan(spans, index, end + 2, TokenKind::Comment);
                index = end + 2;
                continue;
            }
            if (ch == u'"')
            {
                // 后面紧跟冒号的字符串为键名
                const int end = findQuote(line, index + 1, ch);
                const int stop = end < 0 ? length : end + 1;
                const int after = skipSpaces(line, stop);
                const bool isKey = after < length && line.at(after) == u':';
                addSpan(spans, index, stop, isKey ? TokenKind::Constant : TokenKind::String);
                index = stop;
                continue;
            }
            if (ch.isDigit() || ch == u'-' || ch.isLetter())
            {
                int end = index + 1;
                while (end < length && (line.at(end).isLetterOrNumber() || line.at(end) == u'.' || line.at(end) == u'-' || line.at(end) == u'+'))
                {
                    ++end;
                }
                addSpan(spans, index, end, TokenKind::Constant);
                index = end;
                continue;
            }
            ++index;
        }
        return spans;
    }

    QColor hexColor(const char* hex)
    {
        return QColor(QString::fromLatin1(hex));
    }
} // namespace

namespace ShadCodeHighlighter
{
    Language languageFromName(const QString& name)
    {
        const QString key = name.trimmed().toLower();
        if (key == QStringLiteral("ansi"))
        {
            return Language::Ansi;
        }
        if (key == QStringLiteral("bash") || key == QStringLiteral("sh") || key == QStringLiteral("shell") || key == QStringLiteral("shellscript") || key == QStringLiteral("zsh") || key == QStringLiteral("console"))
        {
            return Language::Bash;
        }
        if (key == QStringLiteral("css"))
        {
            return Language::Css;
        }
        if (key == QStringLiteral("cpp") || key == QStringLiteral("c++") || key == QStringLiteral("cxx") || key == QStringLiteral("cc") || key == QStringLiteral("c") || key == QStringLiteral("h") || key == QStringLiteral("hpp"))
        {
            return Language::Cpp;
        }
        if (key == QStringLiteral("json") || key == QStringLiteral("jsonc"))
        {
            return Language::Json;
        }
        if (key == QStringLiteral("javascript") || key == QStringLiteral("js") || key == QStringLiteral("jsx") || key == QStringLiteral("mjs") || key == QStringLiteral("cjs"))
        {
            return Language::JavaScript;
        }
        if (key == QStringLiteral("typescript") || key == QStringLiteral("ts") || key == QStringLiteral("tsx") || key == QStringLiteral("mts") || key == QStringLiteral("cts"))
        {
            return Language::TypeScript;
        }
        if (key == QStringLiteral("python") || key == QStringLiteral("py"))
        {
            return Language::Python;
        }
        return Language::PlainText;
    }

    QList<Span> highlightLine(Language language, const QString& line, int& state)
    {
        switch (language)
        {
            case Language::Bash:
                return lexGeneric(shellRules(), line, state);
            case Language::Css:
                return lexCss(line, state);
            case Language::Cpp:
                return lexGeneric(cppRules(), line, state);
            case Language::Json:
                return lexJson(line, state);
            case Language::JavaScript:
                return lexGeneric(javaScriptRules(), line, state);
            case Language::TypeScript:
                return lexGeneric(typeScriptRules(), line, state);
            case Language::Python:
                return lexGeneric(pythonRules(), line, state);
            case Language::PlainText:
            case Language::Ansi:
                break;
        }
        state = kStateNormal;
        return QList<Span>();
    }

    QList<Span> parseAnsiLine(const QString& rawLine, QString& text)
    {
        QList<Span> spans;
        text.clear();
        text.reserve(rawLine.size());
        int color = -1;
        bool isBold = false;
        bool isDim = false;
        int spanStart = 0;
        // 样式变化前，把上一段有样式的文字记为一个片段
        const auto flush = [&]()
        {
            const int end = int(text.size());
            if (end > spanStart && (color >= 0 || isBold || isDim))
            {
                Span span;
                span.start = spanStart;
                span.length = end - spanStart;
                span.ansiColor = color;
                span.isBold = isBold;
                span.isDim = isDim;
                spans.append(span);
            }
            spanStart = end;
        };
        const int length = int(rawLine.size());
        int index = 0;
        while (index < length)
        {
            const QChar ch = rawLine.at(index);
            if (ch != kEscape)
            {
                text.append(ch);
                ++index;
                continue;
            }
            if (index + 1 >= length || rawLine.at(index + 1) != u'[')
            {
                // 不是 CSI 序列的单独 ESC 直接丢弃
                ++index;
                continue;
            }
            // CSI 序列以 0x40-0x7E 之间的字符结束，只有以 m 结束的 SGR 序列影响样式
            int end = index + 2;
            while (end < length && (rawLine.at(end).unicode() < 0x40 || rawLine.at(end).unicode() > 0x7E))
            {
                ++end;
            }
            if (end >= length)
            {
                break;
            }
            if (rawLine.at(end) == u'm')
            {
                flush();
                const QStringList parameters = rawLine.mid(index + 2, end - index - 2).split(u';');
                for (int position = 0; position < parameters.size(); ++position)
                {
                    const int code = parameters.at(position).isEmpty() ? 0 : parameters.at(position).toInt();
                    if (code == 0)
                    {
                        color = -1;
                        isBold = false;
                        isDim = false;
                    }
                    else if (code == 1)
                    {
                        isBold = true;
                    }
                    else if (code == 2)
                    {
                        isDim = true;
                    }
                    else if (code == 22)
                    {
                        isBold = false;
                        isDim = false;
                    }
                    else if (code == 39)
                    {
                        color = -1;
                    }
                    else if (code >= 30 && code <= 37)
                    {
                        color = code - 30;
                    }
                    else if (code >= 90 && code <= 97)
                    {
                        color = code - 90 + 8;
                    }
                    else if (code == 38 && position + 1 < parameters.size())
                    {
                        // 38;5;n 只支持前 16 色，38;2;r;g;b 跳过
                        const int mode = parameters.at(position + 1).toInt();
                        if (mode == 5 && position + 2 < parameters.size())
                        {
                            const int value = parameters.at(position + 2).toInt();
                            color = value < 16 ? value : -1;
                            position += 2;
                        }
                        else if (mode == 2)
                        {
                            position += 4;
                        }
                    }
                }
            }
            index = end + 1;
        }
        flush();
        return spans;
    }

    QString stripAnsi(const QString& text)
    {
        const QStringList lines = text.split(u'\n');
        QStringList result;
        result.reserve(lines.size());
        for (const QString& line : lines)
        {
            QString plain;
            parseAnsiLine(line, plain);
            result.append(plain);
        }
        return result.join(u'\n');
    }

    QColor tokenColor(TokenKind kind, bool isDark)
    {
        switch (kind)
        {
            case TokenKind::Keyword:
                return hexColor(isDark ? "#F97583" : "#D73A49");
            case TokenKind::String:
                return hexColor(isDark ? "#9ECBFF" : "#032F62");
            case TokenKind::Constant:
                return hexColor(isDark ? "#79B8FF" : "#005CC5");
            case TokenKind::Comment:
                return hexColor("#6A737D");
            case TokenKind::Function:
                return hexColor(isDark ? "#B392F0" : "#6F42C1");
            case TokenKind::Variable:
                return hexColor(isDark ? "#FFAB70" : "#E36209");
            case TokenKind::Tag:
                return hexColor(isDark ? "#85E89D" : "#22863A");
            case TokenKind::Plain:
                break;
        }
        return QColor();
    }

    QColor ansiColor(int index, bool isDark)
    {
        // GitHub 主题的终端配色：黑、红、绿、黄、蓝、品红、青、白，以及对应的亮色
        static const char* const lightColors[16] = {"#24292E", "#D73A49", "#28A745", "#DBAB09", "#0366D6", "#5A32A3", "#1B7C83", "#6A737D", "#959DA5", "#CB2431", "#22863A", "#B08800", "#005CC5", "#5A32A3", "#3192AA", "#D1D5DA"};
        static const char* const darkColors[16] = {"#586069", "#EA4A5A", "#34D058", "#FFEA7F", "#2188FF", "#B392F0", "#39C5CF", "#D1D5DA", "#959DA5", "#F97583", "#85E89D", "#FFEA7F", "#79B8FF", "#B392F0", "#56D4DD", "#FAFBFC"};
        if (index < 0 || index >= 16)
        {
            return QColor();
        }
        return hexColor(isDark ? darkColors[index] : lightColors[index]);
    }
} // namespace ShadCodeHighlighter
