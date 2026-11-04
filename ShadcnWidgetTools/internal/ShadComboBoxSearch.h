#ifndef SHADCOMBOBOXSEARCH_H
#define SHADCOMBOBOXSEARCH_H

#include <QString>
#include <QStringList>

// ShadComboBox 的搜索工具，不对外导出
namespace ShadComboBoxSearch
{
    // 按键查找的缓冲超时，超过该时间未输入则重新开始，对应 Radix Select 的 typeahead
    constexpr int kTypeaheadTimeoutMs = 1000;

    // 模糊匹配：忽略大小写与查询中的空白，查询字符按顺序出现在文本中即匹配；不匹配返回 -1，匹配返回正数，越大越相关
    // 相关度从高到低：整段作为前缀 > 整段出现在词首 > 整段出现在其他位置 > 按顺序分散出现（连续、词首出现的字符加分，间隔越大扣分越多）
    int fuzzyScore(const QString& text, const QString& query);

    // 按键查找：texts 中不可选的项传空字符串，从 currentIndex 开始循环查找以 search 开头的项（忽略大小写），找不到返回 -1
    // search 由同一个字符重复组成（例如 "aaa"）时只按首字符查找；只有一个字符时从下一项开始找，以便连续按同一个键在同首字母的项之间轮换
    int typeaheadIndex(const QStringList& texts, const QString& search, int currentIndex);
} // namespace ShadComboBoxSearch

#endif // SHADCOMBOBOXSEARCH_H
