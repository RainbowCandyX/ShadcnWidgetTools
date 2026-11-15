#ifndef SHADDIFFENGINE_H
#define SHADDIFFENGINE_H

#include <QList>
#include <QString>
#include <QStringList>

// ShadDiffView 使用的文本差异算法，不对外导出
namespace ShadDiffEngine
{
    enum class OpKind
    {
        Equal = 0,
        Removed,
        Added,
    };

    struct Op
    {
        OpKind kind{OpKind::Equal};
        int oldIndex{-1};
        int newIndex{-1};
    };

    struct Range
    {
        int start{0};
        int length{0};
    };

    // 每段连续改动中，删除排在新增之前
    QList<Op> diffLines(const QStringList& oldLines, const QStringList& newLines);

    // 差异过大或过长时返回 false，不标记行内变化
    bool diffCharacters(const QString& oldLine, const QString& newLine, QList<Range>& oldRanges, QList<Range>& newRanges);
} // namespace ShadDiffEngine

#endif // SHADDIFFENGINE_H
