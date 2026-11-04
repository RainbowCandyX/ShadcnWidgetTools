#include "ShadButtonGroupPrivate.h"

#include <QLineEdit>
#include <QWidget>
#include <QtGlobal>

#include "ShadButtonGroup.h"
#include "ShadStyleHelper.h"

namespace
{
    // 输入框对应 [&>input]:flex-1，分得多余的宽度
    bool isInput(const QWidget* widget)
    {
        return qobject_cast<const QLineEdit*>(widget) != nullptr;
    }
} // namespace

ShadButtonGroupTextPrivate::ShadButtonGroupTextPrivate()
{
}

ShadButtonGroupTextPrivate::~ShadButtonGroupTextPrivate()
{
}

ShadButtonGroupPrivate::ShadButtonGroupPrivate()
{
}

ShadButtonGroupPrivate::~ShadButtonGroupPrivate()
{
}

QList<int> ShadButtonGroupPrivate::visibleItems() const
{
    QList<int> indexes;
    for (int index = 0; index < int(_items.size()); ++index)
    {
        const Item& item = _items.at(index);
        if (item.isSeparator || (!item.widget.isNull() && !item.widget->isHidden()))
        {
            indexes.append(index);
        }
    }
    return indexes;
}

bool ShadButtonGroupPrivate::isNested() const
{
    for (const int index : visibleItems())
    {
        if (qobject_cast<const ShadButtonGroup*>(_items.at(index).widget.data()) != nullptr)
        {
            return true;
        }
    }
    return false;
}

int ShadButtonGroupPrivate::itemLength(const Item& item, bool useMinimum) const
{
    if (item.isSeparator)
    {
        return 1;
    }
    const QWidget* widget = item.widget.data();
    if (widget == nullptr)
    {
        return 0;
    }
    // 按 sizeHint 排列，并遵守组件自身的最小 / 最大尺寸（例如 setFixedWidth）
    const QSize preferred = useMinimum && isInput(widget) ? widget->minimumSizeHint() : widget->sizeHint();
    const QSize size = preferred.expandedTo(widget->minimumSize()).boundedTo(widget->maximumSize());
    const int length = _orientation == Qt::Horizontal ? size.width() : size.height();
    return qMax(0, length - 2 * ShadStyleHelper::kFocusRingWidth);
}

int ShadButtonGroupPrivate::crossLength() const
{
    int length = 0;
    for (const int index : visibleItems())
    {
        const QWidget* widget = _items.at(index).widget.data();
        if (widget == nullptr)
        {
            continue;
        }
        const QSize size = widget->sizeHint().expandedTo(widget->minimumSize()).boundedTo(widget->maximumSize());
        length = qMax(length, (_orientation == Qt::Horizontal ? size.height() : size.width()) - 2 * ShadStyleHelper::kFocusRingWidth);
    }
    return length;
}

QSize ShadButtonGroupPrivate::contentSize(bool useMinimum) const
{
    const QList<int> indexes = visibleItems();
    const int gap = isNested() ? kNestedGap : 0;
    int mainLength = 0;
    for (const int index : indexes)
    {
        mainLength += itemLength(_items.at(index), useMinimum);
    }
    mainLength += qMax(0, int(indexes.size()) - 1) * gap;
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const int cross = crossLength();
    if (_orientation == Qt::Horizontal)
    {
        return QSize(mainLength + margin, cross + margin);
    }
    return QSize(cross + margin, mainLength + margin);
}

void ShadButtonGroupPrivate::relayout()
{
    Q_Q(ShadButtonGroup);
    pruneItems();
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const bool isHorizontal = _orientation == Qt::Horizontal;
    const bool nested = isNested();
    const int gap = nested ? kNestedGap : 0;
    const int cross = crossLength();
    const QList<int> indexes = visibleItems();
    const int count = int(indexes.size());

    // 主方向长度；横向时组件比内容宽，多余部分平分给输入框，组件较窄时输入框收缩到最小宽度
    QList<int> lengths;
    QList<int> minimumLengths;
    QList<int> inputPositions;
    int total = 0;
    for (int position = 0; position < count; ++position)
    {
        const Item& item = _items.at(indexes.at(position));
        lengths.append(itemLength(item, false));
        minimumLengths.append(itemLength(item, true));
        total += lengths.last();
        if (isHorizontal && isInput(item.widget.data()))
        {
            inputPositions.append(position);
        }
    }
    total += qMax(0, count - 1) * gap;
    if (isHorizontal && !inputPositions.isEmpty())
    {
        int extra = q->width() - 2 * margin - total;
        for (int order = 0; order < int(inputPositions.size()); ++order)
        {
            // 余数分给最后一个输入框
            const int remaining = int(inputPositions.size()) - order;
            const int share = extra / remaining;
            const int position = inputPositions.at(order);
            const int newLength = qMax(minimumLengths.at(position), lengths.at(position) + share);
            extra -= newLength - lengths.at(position);
            lengths[position] = newLength;
        }
    }

    // 拼接时第一项之后的项起始侧为直角且不描边，最后一项之前的项结束侧为直角；RTL 下横向的起始侧为右侧
    const Qt::LayoutDirection direction = q->layoutDirection();
    const Qt::Edge startEdge = isHorizontal ? (direction == Qt::RightToLeft ? Qt::RightEdge : Qt::LeftEdge) : Qt::TopEdge;
    const Qt::Edge endEdge = isHorizontal ? (direction == Qt::RightToLeft ? Qt::LeftEdge : Qt::RightEdge) : Qt::BottomEdge;
    const QRect bounds = q->rect();
    _separatorRects.clear();
    int offset = margin;
    for (int position = 0; position < count; ++position)
    {
        const Item& item = _items.at(indexes.at(position));
        const int length = lengths.at(position);
        const QRect body = isHorizontal ? QRect(offset, margin, length, cross) : QRect(margin, offset, cross, length);
        if (item.isSeparator)
        {
            _separatorRects.append(ShadStyleHelper::visualRect(direction, bounds, body));
        }
        else
        {
            QWidget* widget = item.widget.data();
            widget->setGeometry(ShadStyleHelper::visualRect(direction, bounds, body.adjusted(-margin, -margin, margin, margin)));
            ShadStyleHelper::GroupEdges edges;
            if (!nested && position > 0)
            {
                edges.squareEdges |= startEdge;
                edges.hiddenBorderEdges |= startEdge;
            }
            if (!nested && position + 1 < count)
            {
                edges.squareEdges |= endEdge;
            }
            ShadStyleHelper::setGroupEdges(widget, edges);
        }
        offset += length + gap;
    }
    q->update();
}

void ShadButtonGroupPrivate::insertItem(int index, QWidget* widget, bool isSeparator)
{
    Q_Q(ShadButtonGroup);
    pruneItems();
    const int count = int(_items.size());
    const int position = index < 0 || index > count ? count : index;
    Item item;
    item.isSeparator = isSeparator;
    if (widget != nullptr)
    {
        // 保留调用方显式隐藏的状态，其余情况加入后显示
        const bool isExplicitlyHidden = widget->isHidden() && widget->testAttribute(Qt::WA_WState_ExplicitShowHide);
        if (widget->parentWidget() != q)
        {
            widget->setParent(q);
        }
        widget->installEventFilter(q);
        if (!isExplicitlyHidden)
        {
            widget->show();
        }
        item.widget = widget;
    }
    _items.insert(position, item);
    q->updateGeometry();
    relayout();
}

void ShadButtonGroupPrivate::pruneItems()
{
    _items.removeIf([](const Item& item) { return !item.isSeparator && item.widget.isNull(); });
}
