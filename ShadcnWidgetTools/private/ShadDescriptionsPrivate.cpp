#include "ShadDescriptionsPrivate.h"

#include <QColor>
#include <QFontMetrics>
#include <QLabel>
#include <QList>
#include <QPalette>
#include <QRect>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadDescriptions.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 标题行与内容的间距，标题与操作区的间距
    constexpr int kHeaderGap = 16;
    // 横向与纵向排列时标签与内容的间距
    constexpr int kLabelGap = 8;
    constexpr int kVerticalLabelGap = 4;
    // 标签最多占的宽度比例，超出时换行
    constexpr int kLabelWidthPercent = 40;
} // namespace

ShadDescriptionsPrivate::ShadDescriptionsPrivate()
{
}

ShadDescriptionsPrivate::~ShadDescriptionsPrivate()
{
}

ShadDescriptionsPrivate::Metrics ShadDescriptionsPrivate::metrics() const
{
    Metrics result;
    if (_size == ShadDescriptionsType::Small)
    {
        result.columnGap = 16;
        result.rowGap = 12;
        result.cellPaddingX = 8;
        result.cellPaddingY = 6;
    }
    return result;
}

QString ShadDescriptionsPrivate::labelText(const QString& label) const
{
    // 使用半角冒号，与 Ant Design 一致
    if (!_isColonVisible || label.isEmpty())
    {
        return label;
    }
    return label + QStringLiteral(":");
}

QList<ShadDescriptionsPrivate::Placement> ShadDescriptionsPrivate::placeItems() const
{
    QList<Placement> result;
    const int columns = _columnCount;
    int row = 0;
    int column = 0;
    for (int index = 0; index < _items.size(); ++index)
    {
        const int span = qBound(1, _items.at(index).span, columns);
        if (column > 0 && column + span > columns)
        {
            // 放不下时本行最后一项延伸占满，换到下一行
            Placement& last = result.last();
            last.span = columns - last.column;
            ++row;
            column = 0;
        }
        result.append(Placement{index, row, column, span});
        column += span;
        if (column >= columns)
        {
            ++row;
            column = 0;
        }
    }
    if (!result.isEmpty() && column > 0)
    {
        // 最后一行未填满时最后一项延伸占满
        Placement& last = result.last();
        last.span = columns - last.column;
    }
    return result;
}

int ShadDescriptionsPrivate::rowCount(const QList<Placement>& placements) const
{
    return placements.isEmpty() ? 0 : placements.last().row + 1;
}

int ShadDescriptionsPrivate::labelNaturalWidth(int index) const
{
    const Item& item = _items.at(index);
    if (item.label.isEmpty())
    {
        return 0;
    }
    // 多留 1px，防止刚好放下的文字被换行
    return ShadStyleHelper::textWidth(item.labelLabel->font(), labelText(item.label)) + 1;
}

int ShadDescriptionsPrivate::labelHeight(const QLabel* label, int width)
{
    if (label == nullptr || width <= 0)
    {
        return 0;
    }
    // 空文字的内容项保留一行高度
    if (label->text().isEmpty())
    {
        return label->fontMetrics().height();
    }
    const int height = label->heightForWidth(width);
    return height >= 0 ? height : label->sizeHint().height();
}

QSize ShadDescriptionsPrivate::contentSize(int index, int width) const
{
    const Item& item = _items.at(index);
    const int availableWidth = qMax(1, width);
    if (item.textLabel != nullptr)
    {
        return QSize(availableWidth, labelHeight(item.textLabel, availableWidth));
    }
    QWidget* widget = item.widget.data();
    if (widget == nullptr || widget->isHidden())
    {
        return QSize(0, 0);
    }
    const QSize hint = widget->sizeHint().expandedTo(widget->minimumSize()).boundedTo(widget->maximumSize());
    // 可水平伸展的组件占满单元格宽度
    const bool canGrow = widget->sizePolicy().horizontalPolicy() & QSizePolicy::GrowFlag;
    const int contentWidth = qMin(canGrow ? qMin(availableWidth, widget->maximumWidth()) : hint.width(), availableWidth);
    int contentHeight = widget->hasHeightForWidth() ? widget->heightForWidth(contentWidth) : hint.height();
    if (contentHeight < 0)
    {
        contentHeight = hint.height();
    }
    return QSize(contentWidth, contentHeight);
}

QSize ShadDescriptionsPrivate::extraSize() const
{
    if (_extra.isNull() || _extra->isHidden())
    {
        return QSize();
    }
    return _extra->sizeHint().expandedTo(_extra->minimumSize()).boundedTo(_extra->maximumSize());
}

bool ShadDescriptionsPrivate::hasHeader() const
{
    return !_title.isEmpty() || extraSize().isValid();
}

ShadDescriptionsPrivate::LayoutResult ShadDescriptionsPrivate::computeLayout(int width) const
{
    LayoutResult result;
    result.items.resize(_items.size());
    const int contentWidth = qMax(1, width);
    int top = 0;
    if (hasHeader())
    {
        const QSize extra = extraSize();
        const bool hasExtra = extra.isValid();
        const int extraWidth = hasExtra ? qMin(extra.width(), contentWidth) : 0;
        const int titleWidth = _title.isEmpty() ? 0 : qMax(1, contentWidth - (hasExtra ? extraWidth + kHeaderGap : 0));
        const int titleHeight = labelHeight(_titleLabel, titleWidth);
        const int headerHeight = qMax(titleHeight, hasExtra ? extra.height() : 0);
        result.titleRect = QRect(0, top + (headerHeight - titleHeight) / 2, titleWidth, titleHeight);
        if (hasExtra)
        {
            result.extraRect = QRect(contentWidth - extraWidth, top + (headerHeight - extra.height()) / 2, extraWidth, extra.height());
        }
        top += headerHeight;
        if (!_items.isEmpty())
        {
            top += kHeaderGap;
        }
    }
    if (_items.isEmpty())
    {
        result.totalHeight = top;
        return result;
    }
    const QList<Placement> placements = placeItems();
    result.totalHeight = _isBordered ? layoutBordered(placements, contentWidth, top, &result) : layoutPlain(placements, contentWidth, top, &result);
    return result;
}

int ShadDescriptionsPrivate::layoutPlain(const QList<Placement>& placements, int width, int top, LayoutResult* result) const
{
    const Metrics m = metrics();
    const int columns = _columnCount;
    const int columnWidth = qMax(1, (width - (columns - 1) * m.columnGap) / columns);
    const bool isVertical = _itemLayout == ShadDescriptionsType::Vertical;
    int rowTop = top;
    int rowHeight = 0;
    int currentRow = -1;
    for (const Placement& placement : placements)
    {
        if (placement.row != currentRow)
        {
            if (currentRow >= 0)
            {
                rowTop += rowHeight + m.rowGap;
            }
            currentRow = placement.row;
            rowHeight = 0;
        }
        const Item& item = _items.at(placement.item);
        const int x = placement.column * (columnWidth + m.columnGap);
        // 行末的项吸收等分余数，保证右边缘对齐
        const int itemWidth = placement.column + placement.span >= columns ? width - x : placement.span * columnWidth + (placement.span - 1) * m.columnGap;
        const int naturalWidth = labelNaturalWidth(placement.item);
        ItemGeometry& geometry = (*result).items[placement.item];
        if (isVertical)
        {
            const int labelHeightValue = labelHeight(item.labelLabel, naturalWidth > 0 ? itemWidth : 0);
            const int gap = labelHeightValue > 0 ? kVerticalLabelGap : 0;
            const QSize content = contentSize(placement.item, itemWidth);
            geometry.label = QRect(x, rowTop, itemWidth, labelHeightValue);
            geometry.content = QRect(x, rowTop + labelHeightValue + gap, content.width(), content.height());
            rowHeight = qMax(rowHeight, labelHeightValue + gap + content.height());
            continue;
        }
        const int labelWidth = qMin(naturalWidth, itemWidth * kLabelWidthPercent / 100);
        const int gap = labelWidth > 0 ? kLabelGap : 0;
        const int contentWidth = qMax(1, itemWidth - labelWidth - gap);
        const int labelHeightValue = labelHeight(item.labelLabel, labelWidth);
        const QSize content = contentSize(placement.item, contentWidth);
        geometry.label = QRect(x, rowTop, labelWidth, labelHeightValue);
        geometry.content = QRect(x + labelWidth + gap, rowTop, content.width(), content.height());
        rowHeight = qMax(rowHeight, qMax(labelHeightValue, content.height()));
    }
    return rowTop + rowHeight;
}

int ShadDescriptionsPrivate::layoutBordered(const QList<Placement>& placements, int width, int top, LayoutResult* result) const
{
    const Metrics m = metrics();
    const int columns = _columnCount;
    const int rows = rowCount(placements);
    // 预留外边框与分隔线，右端与下端坐标不含端点
    const int innerLeft = 1;
    const int innerRight = qMax(innerLeft + 1, width - 1);
    const int innerWidth = innerRight - innerLeft;
    const bool isVertical = _itemLayout == ShadDescriptionsType::Vertical;
    if (isVertical)
    {
        // 每个网格行拆成标签行与内容行
        const int cellWidth = qMax(1, (innerWidth - (columns - 1)) / columns);
        QList<int> labelRowHeights(rows, 0);
        QList<int> contentRowHeights(rows, 0);
        QList<int> cellLefts;
        QList<int> cellRights;
        for (const Placement& placement : placements)
        {
            const int left = innerLeft + placement.column * (cellWidth + 1);
            const int lastColumn = placement.column + placement.span - 1;
            const int right = lastColumn >= columns - 1 ? innerRight : innerLeft + lastColumn * (cellWidth + 1) + cellWidth;
            cellLefts.append(left);
            cellRights.append(right);
            const int innerCellWidth = qMax(1, right - left - 2 * m.cellPaddingX);
            const int labelHeightValue = labelHeight(_items.at(placement.item).labelLabel, labelNaturalWidth(placement.item) > 0 ? innerCellWidth : 0);
            labelRowHeights[placement.row] = qMax(labelRowHeights[placement.row], labelHeightValue + 2 * m.cellPaddingY);
            contentRowHeights[placement.row] = qMax(contentRowHeights[placement.row], contentSize(placement.item, innerCellWidth).height() + 2 * m.cellPaddingY);
        }
        QList<int> labelTops(rows, 0);
        QList<int> contentTops(rows, 0);
        int y = top + 1;
        for (int row = 0; row < rows; ++row)
        {
            labelTops[row] = y;
            y += labelRowHeights.at(row) + 1;
            contentTops[row] = y;
            y += contentRowHeights.at(row) + 1;
        }
        for (int index = 0; index < placements.size(); ++index)
        {
            const Placement& placement = placements.at(index);
            const int left = cellLefts.at(index);
            const int cellWidthValue = cellRights.at(index) - left;
            const int innerCellWidth = qMax(1, cellWidthValue - 2 * m.cellPaddingX);
            ItemGeometry& geometry = (*result).items[placement.item];
            geometry.labelCell = QRect(left, labelTops.at(placement.row), cellWidthValue, labelRowHeights.at(placement.row));
            geometry.contentCell = QRect(left, contentTops.at(placement.row), cellWidthValue, contentRowHeights.at(placement.row));
            const int labelHeightValue = labelHeight(_items.at(placement.item).labelLabel, labelNaturalWidth(placement.item) > 0 ? innerCellWidth : 0);
            const QSize content = contentSize(placement.item, innerCellWidth);
            geometry.label = QRect(left + m.cellPaddingX, labelTops.at(placement.row) + m.cellPaddingY, innerCellWidth, labelHeightValue);
            geometry.content = QRect(left + m.cellPaddingX, contentTops.at(placement.row) + m.cellPaddingY, content.width(), content.height());
        }
        result->tableRect = QRect(0, top, width, y - top);
        return y;
    }

    // 横向排列：同一列的标签格等宽，按该列最宽的标签计算
    const int share = qMax(1, innerWidth / columns);
    QList<int> labelWidths(columns, 0);
    for (const Placement& placement : placements)
    {
        const int naturalWidth = labelNaturalWidth(placement.item) + 2 * m.cellPaddingX;
        labelWidths[placement.column] = qMax(labelWidths.at(placement.column), qMin(naturalWidth, share * kLabelWidthPercent / 100));
    }
    int labelSum = 0;
    for (const int labelWidth : labelWidths)
    {
        labelSum += labelWidth;
    }
    const int contentCellWidth = qMax(1, (innerWidth - (2 * columns - 1) - labelSum) / columns);
    QList<int> labelLefts(columns, 0);
    QList<int> contentLefts(columns, 0);
    QList<int> contentRights(columns, 0);
    int x = innerLeft;
    for (int column = 0; column < columns; ++column)
    {
        labelLefts[column] = x;
        contentLefts[column] = x + labelWidths.at(column) + 1;
        contentRights[column] = contentLefts.at(column) + contentCellWidth;
        x = contentRights.at(column) + 1;
    }
    // 最后一列吸收等分宽度的余数
    contentRights[columns - 1] = qMax(contentLefts.at(columns - 1) + 1, innerRight);

    QList<int> rowHeights(rows, 0);
    for (const Placement& placement : placements)
    {
        const int labelInnerWidth = qMax(0, labelWidths.at(placement.column) - 2 * m.cellPaddingX);
        const int contentInnerWidth = qMax(1, contentRights.at(placement.column + placement.span - 1) - contentLefts.at(placement.column) - 2 * m.cellPaddingX);
        const int labelHeightValue = labelHeight(_items.at(placement.item).labelLabel, labelNaturalWidth(placement.item) > 0 ? labelInnerWidth : 0);
        const int contentHeight = contentSize(placement.item, contentInnerWidth).height();
        rowHeights[placement.row] = qMax(rowHeights.at(placement.row), qMax(labelHeightValue, contentHeight) + 2 * m.cellPaddingY);
    }
    QList<int> rowTops(rows, 0);
    int y = top + 1;
    for (int row = 0; row < rows; ++row)
    {
        rowTops[row] = y;
        y += rowHeights.at(row) + 1;
    }
    for (const Placement& placement : placements)
    {
        const int rowTop = rowTops.at(placement.row);
        const int rowHeight = rowHeights.at(placement.row);
        const int labelLeft = labelLefts.at(placement.column);
        const int labelWidth = labelWidths.at(placement.column);
        const int contentLeft = contentLefts.at(placement.column);
        const int contentRight = contentRights.at(placement.column + placement.span - 1);
        const int labelInnerWidth = qMax(0, labelWidth - 2 * m.cellPaddingX);
        const int contentInnerWidth = qMax(1, contentRight - contentLeft - 2 * m.cellPaddingX);
        ItemGeometry& geometry = (*result).items[placement.item];
        geometry.labelCell = QRect(labelLeft, rowTop, labelWidth, rowHeight);
        geometry.contentCell = QRect(contentLeft, rowTop, contentRight - contentLeft, rowHeight);
        const int labelHeightValue = labelHeight(_items.at(placement.item).labelLabel, labelNaturalWidth(placement.item) > 0 ? labelInnerWidth : 0);
        const QSize content = contentSize(placement.item, contentInnerWidth);
        geometry.label = QRect(labelLeft + m.cellPaddingX, rowTop + m.cellPaddingY, labelInnerWidth, labelHeightValue);
        geometry.content = QRect(contentLeft + m.cellPaddingX, rowTop + m.cellPaddingY, content.width(), content.height());
    }
    result->tableRect = QRect(0, top, width, y - top);
    return y;
}

ShadDescriptionsPrivate::Item* ShadDescriptionsPrivate::itemAt(int index)
{
    if (index < 0 || index >= _items.size())
    {
        return nullptr;
    }
    return &_items[index];
}

const ShadDescriptionsPrivate::Item* ShadDescriptionsPrivate::itemAt(int index) const
{
    if (index < 0 || index >= _items.size())
    {
        return nullptr;
    }
    return &_items.at(index);
}

void ShadDescriptionsPrivate::updateColors()
{
    Q_Q(ShadDescriptions);
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor muted = shadTheme->getColor(ShadThemeType::MutedForeground);
    const auto applyColor = [](QLabel* label, const QColor& color)
    {
        if (label == nullptr)
        {
            return;
        }
        QPalette palette = label->palette();
        palette.setColor(QPalette::WindowText, color);
        label->setPalette(palette);
    };
    applyColor(_titleLabel, foreground);
    for (const Item& item : _items)
    {
        applyColor(item.labelLabel, muted);
        applyColor(item.textLabel, foreground);
    }
    q->update();
}

void ShadDescriptionsPrivate::layoutChildren()
{
    Q_Q(ShadDescriptions);
    _layout = computeLayout(q->width());
    const QRect bounds = q->rect();
    const Qt::LayoutDirection direction = q->layoutDirection();
    _titleLabel->setVisible(!_title.isEmpty());
    _titleLabel->setGeometry(ShadStyleHelper::visualRect(direction, bounds, _layout.titleRect));
    if (!_extra.isNull() && !_extra->isHidden())
    {
        _extra->setGeometry(ShadStyleHelper::visualRect(direction, bounds, _layout.extraRect));
    }
    for (int index = 0; index < _items.size(); ++index)
    {
        const Item& item = _items.at(index);
        const ItemGeometry& geometry = _layout.items.at(index);
        item.labelLabel->setGeometry(ShadStyleHelper::visualRect(direction, bounds, geometry.label));
        if (item.textLabel != nullptr)
        {
            item.textLabel->setGeometry(ShadStyleHelper::visualRect(direction, bounds, geometry.content));
        }
        else if (!item.widget.isNull())
        {
            item.widget->setGeometry(ShadStyleHelper::visualRect(direction, bounds, geometry.content));
        }
    }
    q->update();
}

void ShadDescriptionsPrivate::updateContent()
{
    Q_Q(ShadDescriptions);
    layoutChildren();
    q->updateGeometry();
}
