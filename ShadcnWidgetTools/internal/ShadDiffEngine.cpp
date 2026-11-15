#include "ShadDiffEngine.h"

#include <QChar>
#include <QHash>
#include <QtGlobal>

#include <algorithm>
#include <vector>

namespace
{
    using ShadDiffEngine::Op;
    using ShadDiffEngine::OpKind;
    using ShadDiffEngine::Range;

    // 编辑距离上限，限制回溯记录的内存
    constexpr int kMaxEditDistance = 2000;
    constexpr int kMaxInlineLength = 1000;
    constexpr qreal kMinInlineSimilarity = 0.5;

    // 超过编辑距离上限时返回 false，ops 不变
    bool diffMiddle(const QList<int>& oldItems, int oldStart, int oldCount, const QList<int>& newItems, int newStart, int newCount, QList<Op>& ops)
    {
        const int maxDistance = qMin(oldCount + newCount, kMaxEditDistance);
        const int offset = maxDistance + 1;
        std::vector<int> furthest(2 * maxDistance + 3, 0);
        std::vector<std::vector<int>> trace;
        int found = -1;
        for (int distance = 0; distance <= maxDistance && found < 0; ++distance)
        {
            trace.emplace_back(furthest.begin() + (offset - distance - 1), furthest.begin() + (offset + distance + 2));
            for (int k = -distance; k <= distance; k += 2)
            {
                const bool isDown = k == -distance || (k != distance && furthest[offset + k - 1] < furthest[offset + k + 1]);
                int x = isDown ? furthest[offset + k + 1] : furthest[offset + k - 1] + 1;
                int y = x - k;
                while (x < oldCount && y < newCount && oldItems.at(oldStart + x) == newItems.at(newStart + y))
                {
                    ++x;
                    ++y;
                }
                furthest[offset + k] = x;
                if (x >= oldCount && y >= newCount)
                {
                    found = distance;
                    break;
                }
            }
        }
        if (found < 0)
        {
            return false;
        }

        QList<Op> reversed;
        int x = oldCount;
        int y = newCount;
        for (int distance = found; distance >= 0; --distance)
        {
            const std::vector<int>& saved = trace[distance];
            const int base = distance + 1;
            const int k = x - y;
            const bool isDown = k == -distance || (k != distance && saved[base + k - 1] < saved[base + k + 1]);
            const int previousK = isDown ? k + 1 : k - 1;
            const int previousX = saved[base + previousK];
            const int previousY = previousX - previousK;
            while (x > previousX && y > previousY)
            {
                --x;
                --y;
                reversed.append(Op{OpKind::Equal, oldStart + x, newStart + y});
            }
            if (distance == 0)
            {
                break;
            }
            if (x == previousX)
            {
                --y;
                reversed.append(Op{OpKind::Added, -1, newStart + y});
            }
            else
            {
                --x;
                reversed.append(Op{OpKind::Removed, oldStart + x, -1});
            }
        }
        for (auto it = reversed.crbegin(); it != reversed.crend(); ++it)
        {
            ops.append(*it);
        }
        return true;
    }

    QList<Op> diffSequence(const QList<int>& oldItems, const QList<int>& newItems)
    {
        const int oldSize = int(oldItems.size());
        const int newSize = int(newItems.size());
        const int common = qMin(oldSize, newSize);
        int prefix = 0;
        while (prefix < common && oldItems.at(prefix) == newItems.at(prefix))
        {
            ++prefix;
        }
        int suffix = 0;
        while (suffix < common - prefix && oldItems.at(oldSize - 1 - suffix) == newItems.at(newSize - 1 - suffix))
        {
            ++suffix;
        }
        const int oldCount = oldSize - prefix - suffix;
        const int newCount = newSize - prefix - suffix;
        QList<Op> ops;
        ops.reserve(oldSize + newSize);
        for (int index = 0; index < prefix; ++index)
        {
            ops.append(Op{OpKind::Equal, index, index});
        }
        if (oldCount == 0 || newCount == 0 || !diffMiddle(oldItems, prefix, oldCount, newItems, prefix, newCount, ops))
        {
            for (int index = 0; index < oldCount; ++index)
            {
                ops.append(Op{OpKind::Removed, prefix + index, -1});
            }
            for (int index = 0; index < newCount; ++index)
            {
                ops.append(Op{OpKind::Added, -1, prefix + index});
            }
        }
        for (int index = 0; index < suffix; ++index)
        {
            ops.append(Op{OpKind::Equal, oldSize - suffix + index, newSize - suffix + index});
        }
        return ops;
    }

    // 按码点拆分，代理对算作一个字符
    void toCodePoints(const QString& text, QList<int>& points, QList<int>& offsets)
    {
        for (int index = 0; index < text.size(); ++index)
        {
            offsets.append(index);
            const QChar ch = text.at(index);
            if (ch.isHighSurrogate() && index + 1 < text.size() && text.at(index + 1).isLowSurrogate())
            {
                points.append(int(QChar::surrogateToUcs4(ch, text.at(index + 1))));
                ++index;
            }
            else
            {
                points.append(int(ch.unicode()));
            }
        }
        offsets.append(int(text.size()));
    }

    void appendRange(QList<Range>& ranges, int start, int end)
    {
        if (!ranges.isEmpty() && ranges.last().start + ranges.last().length == start)
        {
            ranges.last().length += end - start;
            return;
        }
        ranges.append(Range{start, end - start});
    }
} // namespace

namespace ShadDiffEngine
{
    QList<Op> diffLines(const QStringList& oldLines, const QStringList& newLines)
    {
        // 相同的行映射为相同编号，加快比较
        QHash<QString, int> lineIds;
        const auto toIds = [&lineIds](const QStringList& lines)
        {
            QList<int> ids;
            ids.reserve(lines.size());
            for (const QString& line : lines)
            {
                const auto it = lineIds.constFind(line);
                if (it != lineIds.constEnd())
                {
                    ids.append(it.value());
                    continue;
                }
                const int id = int(lineIds.size());
                lineIds.insert(line, id);
                ids.append(id);
            }
            return ids;
        };
        const QList<int> oldIds = toIds(oldLines);
        const QList<int> newIds = toIds(newLines);
        QList<Op> ops = diffSequence(oldIds, newIds);
        auto runStart = ops.begin();
        while (runStart != ops.end())
        {
            if (runStart->kind == OpKind::Equal)
            {
                ++runStart;
                continue;
            }
            const auto runEnd = std::find_if(runStart, ops.end(), [](const Op& op) { return op.kind == OpKind::Equal; });
            std::stable_partition(runStart, runEnd, [](const Op& op) { return op.kind == OpKind::Removed; });
            runStart = runEnd;
        }
        return ops;
    }

    bool diffCharacters(const QString& oldLine, const QString& newLine, QList<Range>& oldRanges, QList<Range>& newRanges)
    {
        oldRanges.clear();
        newRanges.clear();
        if (oldLine.isEmpty() || newLine.isEmpty() || oldLine.size() > kMaxInlineLength || newLine.size() > kMaxInlineLength)
        {
            return false;
        }
        QList<int> oldPoints;
        QList<int> oldOffsets;
        QList<int> newPoints;
        QList<int> newOffsets;
        toCodePoints(oldLine, oldPoints, oldOffsets);
        toCodePoints(newLine, newPoints, newOffsets);
        const QList<Op> ops = diffSequence(oldPoints, newPoints);
        int equalCount = 0;
        for (const Op& op : ops)
        {
            if (op.kind == OpKind::Equal)
            {
                ++equalCount;
            }
        }
        // 相同字符太少时按整行改动显示
        if (2.0 * equalCount / (oldPoints.size() + newPoints.size()) < kMinInlineSimilarity)
        {
            return false;
        }
        for (const Op& op : ops)
        {
            if (op.kind == OpKind::Removed)
            {
                appendRange(oldRanges, oldOffsets.at(op.oldIndex), oldOffsets.at(op.oldIndex + 1));
            }
            else if (op.kind == OpKind::Added)
            {
                appendRange(newRanges, newOffsets.at(op.newIndex), newOffsets.at(op.newIndex + 1));
            }
        }
        return true;
    }
} // namespace ShadDiffEngine
