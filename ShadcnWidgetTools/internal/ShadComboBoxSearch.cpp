#include "ShadComboBoxSearch.h"

#include <QChar>
#include <QtGlobal>

namespace
{
    // 前缀、词首、任意位置整段匹配的基础分，保证整段匹配始终排在分散匹配之前
    constexpr int kPrefixScore = 3000;
    constexpr int kWordStartScore = 2000;
    constexpr int kSubstringScore = 1000;
    // 分散匹配的分数上限，低于整段匹配的最低分
    constexpr int kMaxSubsequenceScore = 900;
    constexpr int kCharScore = 10;
    constexpr int kConsecutiveBonus = 15;
    constexpr int kWordStartBonus = 10;
    constexpr int kMaxGapPenalty = 10;

    // 位置 index 处于词首：文本开头，或前一个字符是空白、标点或符号
    bool isWordStart(const QString& text, int index)
    {
        if (index <= 0)
        {
            return true;
        }
        const QChar previous = text.at(index - 1);
        return previous.isSpace() || previous.isPunct() || previous.isSymbol();
    }
} // namespace

namespace ShadComboBoxSearch
{
    int fuzzyScore(const QString& text, const QString& query)
    {
        const QString haystack = text.toCaseFolded();
        QString needle;
        needle.reserve(query.size());
        for (const QChar character : query.toCaseFolded())
        {
            if (!character.isSpace())
            {
                needle.append(character);
            }
        }
        if (needle.isEmpty())
        {
            return 0;
        }

        // 整段匹配：越靠前、文本越短越相关；各档位的分数区间互不重叠
        const int substringIndex = haystack.indexOf(needle);
        if (substringIndex == 0)
        {
            return kPrefixScore - qMin(int(haystack.size()), kSubstringScore / 2 - 1);
        }
        if (substringIndex > 0)
        {
            const int baseScore = isWordStart(haystack, substringIndex) ? kWordStartScore : kSubstringScore;
            return baseScore + kSubstringScore / 2 - qMin(substringIndex, kSubstringScore / 2 - 1);
        }

        // 分散匹配：每个查询字符在上一个匹配位置之后查找
        int score = 0;
        int searchFrom = 0;
        int lastMatch = -2;
        for (const QChar character : needle)
        {
            const int index = haystack.indexOf(character, searchFrom);
            if (index < 0)
            {
                return -1;
            }
            score += kCharScore;
            if (index == lastMatch + 1)
            {
                score += kConsecutiveBonus;
            }
            if (isWordStart(haystack, index))
            {
                score += kWordStartBonus;
            }
            score -= qMin(index - searchFrom, kMaxGapPenalty);
            lastMatch = index;
            searchFrom = index + 1;
        }
        return qBound(1, score, kMaxSubsequenceScore);
    }

    int typeaheadIndex(const QStringList& texts, const QString& search, int currentIndex)
    {
        const int count = int(texts.size());
        if (count == 0 || search.isEmpty())
        {
            return -1;
        }
        bool isRepeated = true;
        for (const QChar character : search)
        {
            if (character != search.at(0))
            {
                isRepeated = false;
                break;
            }
        }
        const QString prefix = isRepeated ? search.left(1) : search;
        // 只有一个字符时从下一项开始找；多个字符时当前项本身也可能匹配
        const int offset = prefix.size() == 1 ? 1 : 0;
        const int base = currentIndex < 0 ? -offset : currentIndex;
        for (int step = 0; step < count; ++step)
        {
            const int index = ((base + offset + step) % count + count) % count;
            const QString& text = texts.at(index);
            if (!text.isEmpty() && text.startsWith(prefix, Qt::CaseInsensitive))
            {
                return index;
            }
        }
        return -1;
    }
} // namespace ShadComboBoxSearch
