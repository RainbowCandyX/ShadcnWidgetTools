#include "ShadTablePrivate.h"

#include <QAbstractItemModel>
#include <QColor>
#include <QFont>
#include <QHeaderView>
#include <QObject>
#include <QPainter>
#include <QRect>
#include <QSet>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QtGlobal>

#include <utility>

#include "ShadStyleHelper.h"
#include "ShadTable.h"
#include "ShadTableDelegate.h"
#include "ShadTableHeader.h"
#include "ShadTheme.h"

namespace
{
    // 默认行高 p-2 + text-sm 行高；有行操作列时 h-8 按钮 + p-2
    constexpr int kDefaultRowHeight = 36;
    constexpr int kActionRowHeight = 48;
} // namespace

ShadTablePrivate::ShadTablePrivate()
{
}

ShadTablePrivate::~ShadTablePrivate()
{
}

int ShadTablePrivate::rowHeight() const
{
    if (_defaultRowHeight > 0)
    {
        return _defaultRowHeight;
    }
    const int baseHeight = _actionColumn >= 0 ? kActionRowHeight : kDefaultRowHeight;
    return qMax(baseHeight, _contentRowHeight);
}

void ShadTablePrivate::measureContentRowHeight()
{
    Q_Q(const ShadTable);
    _contentRowHeight = 0;
    const QAbstractItemModel* model = q->model();
    if (model == nullptr)
    {
        return;
    }
    // 所有行等高：取各单元格所需高度的最大值（图片列、两行文字）
    QStyleOptionViewItem option;
    option.font = q->font();
    const int rows = qMin(rowCount(), kMaxMeasureRows);
    const int columns = model->columnCount(q->rootIndex());
    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            if (!q->isColumnHidden(column))
            {
                _contentRowHeight = qMax(_contentRowHeight, _delegate->sizeHint(option, model->index(row, column, q->rootIndex())).height());
            }
        }
    }
}

int ShadTablePrivate::footerHeight() const
{
    return _footerCells.isEmpty() ? 0 : kFooterHeight;
}

int ShadTablePrivate::captionHeight() const
{
    return _caption.isEmpty() ? 0 : kCaptionTop + kCaptionHeight;
}

int ShadTablePrivate::rowCount() const
{
    Q_Q(const ShadTable);
    const QAbstractItemModel* model = q->model();
    return model != nullptr ? model->rowCount(q->rootIndex()) : 0;
}

int ShadTablePrivate::contentHeight() const
{
    const int rows = rowCount();
    const int emptyHeight = rows == 0 && !_emptyText.isEmpty() ? kEmptyHeight : 0;
    return rows * rowHeight() + emptyHeight + footerHeight() + captionHeight();
}

int ShadTablePrivate::naturalColumnWidth(int column) const
{
    Q_Q(const ShadTable);
    int width = _header->contentWidth(column);
    const QAbstractItemModel* model = q->model();
    if (model == nullptr)
    {
        return width;
    }
    QStyleOptionViewItem option;
    option.font = q->font();
    option.rect = QRect(0, 0, width, rowHeight());
    const int rows = qMin(rowCount(), kMaxMeasureRows);
    for (int row = 0; row < rows; ++row)
    {
        width = qMax(width, _delegate->sizeHint(option, model->index(row, column, q->rootIndex())).width());
    }
    return width;
}

int ShadTablePrivate::totalNaturalWidth() const
{
    Q_Q(const ShadTable);
    const QAbstractItemModel* model = q->model();
    if (model == nullptr)
    {
        return 0;
    }
    int total = 0;
    const int columnCount = model->columnCount(q->rootIndex());
    for (int column = 0; column < columnCount; ++column)
    {
        if (q->isColumnHidden(column))
        {
            continue;
        }
        if (_fixedWidths.value(column, -1) >= 0)
        {
            total += _fixedWidths.value(column);
        }
        else if (_userWidths.contains(column))
        {
            total += _userWidths.value(column);
        }
        else
        {
            total += naturalColumnWidth(column);
        }
    }
    return total;
}

void ShadTablePrivate::scheduleLayout()
{
    Q_Q(ShadTable);
    if (_isLayoutPending)
    {
        return;
    }
    _isLayoutPending = true;
    QMetaObject::invokeMethod(q, [this]() { updateLayout(); }, Qt::QueuedConnection);
}

void ShadTablePrivate::updateLayout()
{
    Q_Q(ShadTable);
    _isLayoutPending = false;
    measureContentRowHeight();
    q->verticalHeader()->setDefaultSectionSize(rowHeight());
    QAbstractItemModel* model = q->model();
    if (model == nullptr)
    {
        q->updateGeometry();
        return;
    }
    // 只测量需要自然宽度的列；固定宽度与拖动过的列不测量
    _naturalWidths.clear();
    const int columnCount = model->columnCount(q->rootIndex());
    for (int column = 0; column < columnCount; ++column)
    {
        if (!q->isColumnHidden(column) && _fixedWidths.value(column, -1) < 0 && !_userWidths.contains(column))
        {
            _naturalWidths.insert(column, naturalColumnWidth(column));
        }
    }
    applyColumnWidths();
}

int ShadTablePrivate::baseColumnWidth(int column) const
{
    const int fixedWidth = _fixedWidths.value(column, -1);
    if (fixedWidth >= 0)
    {
        return fixedWidth;
    }
    if (_userWidths.contains(column))
    {
        return _userWidths.value(column);
    }
    return _naturalWidths.value(column, 0);
}

void ShadTablePrivate::applyColumnWidths()
{
    Q_Q(ShadTable);
    QAbstractItemModel* model = q->model();
    if (model == nullptr)
    {
        return;
    }
    const int columnCount = model->columnCount(q->rootIndex());
    QList<int> widths(columnCount, 0);
    int total = 0;
    int flexibleTotal = 0;
    int lastFlexibleColumn = -1;
    for (int column = 0; column < columnCount; ++column)
    {
        if (q->isColumnHidden(column))
        {
            continue;
        }
        const int width = baseColumnWidth(column);
        widths[column] = width;
        total += width;
        // 只有自然宽度的列参与分配剩余宽度
        if (_fixedWidths.value(column, -1) < 0 && !_userWidths.contains(column))
        {
            flexibleTotal += width;
            lastFlexibleColumn = column;
        }
    }
    // 宽度有剩余时按自然宽度的比例分给只有自然宽度的列，舍入误差归最后一列；没有这样的列时剩余宽度留在右侧
    const int extraWidth = q->viewport()->width() - total;
    if (extraWidth > 0 && flexibleTotal > 0)
    {
        int distributedWidth = 0;
        for (int column = 0; column < columnCount; ++column)
        {
            if (q->isColumnHidden(column) || _fixedWidths.value(column, -1) >= 0 || _userWidths.contains(column))
            {
                continue;
            }
            if (column == lastFlexibleColumn)
            {
                widths[column] += extraWidth - distributedWidth;
                break;
            }
            const int addition = int(qint64(extraWidth) * widths[column] / flexibleTotal);
            widths[column] += addition;
            distributedWidth += addition;
        }
    }
    for (int column = 0; column < columnCount; ++column)
    {
        if (!q->isColumnHidden(column))
        {
            _header->resizeSection(column, widths[column]);
        }
    }
    // 不能拖动的列：行选择列、行操作列与固定宽度的列
    QSet<int> nonResizableColumns(_fixedWidths.keyBegin(), _fixedWidths.keyEnd());
    nonResizableColumns.insert(_selectionColumn);
    nonResizableColumns.insert(_actionColumn);
    _header->setNonResizableColumns(nonResizableColumns);
    q->updateGeometry();
    q->viewport()->update();
    _header->viewport()->update();
}

void ShadTablePrivate::updateHover(const QPoint& pos)
{
    Q_Q(ShadTable);
    const QModelIndex index = q->indexAt(pos);
    const int row = index.isValid() ? index.row() : -1;
    int actionRow = -1;
    if (index.isValid() && index.column() == _actionColumn && _delegate->actionButtonRect(q->visualRect(index), index, q->layoutDirection()).contains(pos))
    {
        actionRow = row;
    }
    if (row != _delegate->getHoveredRow() || actionRow != _delegate->getHoveredActionRow())
    {
        _delegate->setHoveredRow(row);
        _delegate->setHoveredActionRow(actionRow);
        q->viewport()->update();
    }
    const bool isOverCheckbox = index.isValid() && index.column() == _selectionColumn;
    q->viewport()->setCursor(actionRow >= 0 || isOverCheckbox ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void ShadTablePrivate::clearHover()
{
    Q_Q(ShadTable);
    if (_delegate->getHoveredRow() < 0 && _delegate->getHoveredActionRow() < 0)
    {
        return;
    }
    _delegate->setHoveredRow(-1);
    _delegate->setHoveredActionRow(-1);
    q->viewport()->setCursor(Qt::ArrowCursor);
    q->viewport()->update();
}

void ShadTablePrivate::toggleRowChecked(const QModelIndex& index)
{
    Q_Q(ShadTable);
    QAbstractItemModel* model = q->model();
    if (model == nullptr || _selectionColumn < 0)
    {
        return;
    }
    const QModelIndex checkIndex = index.siblingAtColumn(_selectionColumn);
    const bool isChecked = checkIndex.data(Qt::CheckStateRole).toInt() == Qt::Checked;
    model->setData(checkIndex, static_cast<int>(isChecked ? Qt::Unchecked : Qt::Checked), Qt::CheckStateRole);
}

void ShadTablePrivate::toggleAllChecked()
{
    Q_Q(ShadTable);
    QAbstractItemModel* model = q->model();
    if (model == nullptr || _selectionColumn < 0)
    {
        return;
    }
    const Qt::CheckState target = _header->selectionState() == Qt::Checked ? Qt::Unchecked : Qt::Checked;
    const int rows = rowCount();
    for (int row = 0; row < rows; ++row)
    {
        model->setData(model->index(row, _selectionColumn, q->rootIndex()), static_cast<int>(target), Qt::CheckStateRole);
    }
}

void ShadTablePrivate::connectModel()
{
    Q_Q(ShadTable);
    QAbstractItemModel* model = q->model();
    if (model == nullptr)
    {
        return;
    }
    // 数据或结构变化都可能改变列宽、高度与全选状态
    const auto relayout = [this]()
    {
        scheduleLayout();
        _header->viewport()->update();
    };
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::rowsInserted, q, relayout));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::rowsRemoved, q, relayout));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::columnsInserted, q, relayout));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::columnsRemoved, q, relayout));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::modelReset, q, relayout));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::layoutChanged, q, relayout));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::dataChanged, q, relayout));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::headerDataChanged, q, relayout));
}

void ShadTablePrivate::disconnectModel()
{
    for (const QMetaObject::Connection& connection : std::as_const(_modelConnections))
    {
        QObject::disconnect(connection);
    }
    _modelConnections.clear();
}

void ShadTablePrivate::paintFooter(QPainter* painter, int top) const
{
    Q_Q(const ShadTable);
    if (_footerCells.isEmpty())
    {
        return;
    }
    // bg-muted/50 border-t font-medium
    const int width = q->viewport()->width();
    painter->fillRect(QRect(0, top, width, kFooterHeight), ShadStyleHelper::withAlpha(shadTheme->getColor(ShadThemeType::Muted), 0.5));
    painter->fillRect(QRect(0, top, width, 1), shadTheme->getColor(ShadThemeType::Border));
    QFont font = q->font();
    font.setWeight(QFont::Medium);
    painter->setFont(font);
    painter->setPen(shadTheme->getColor(ShadThemeType::Foreground));
    for (const FooterCell& cell : _footerCells)
    {
        if (cell.column < 0 || cell.column >= _header->count())
        {
            continue;
        }
        // 取跨越各列表头区域的并集，RTL 时列从右向左排列，同样适用
        QRect cellRect;
        for (int column = cell.column; column < cell.column + qMax(1, cell.span) && column < _header->count(); ++column)
        {
            if (!_header->isSectionHidden(column))
            {
                cellRect |= QRect(_header->sectionViewportPosition(column), top, _header->sectionSize(column), kFooterHeight);
            }
        }
        if (cellRect.isEmpty())
        {
            continue;
        }
        const QRect textRect = cellRect.adjusted(ShadTableDelegate::kCellPaddingX, 0, -ShadTableDelegate::kCellPaddingX, 0);
        Qt::Alignment alignment = cell.alignment;
        if ((alignment & Qt::AlignVertical_Mask) == 0)
        {
            alignment |= Qt::AlignVCenter;
        }
        painter->drawText(textRect, ShadStyleHelper::visualAlignment(q->layoutDirection(), alignment), ShadStyleHelper::elideText(font, cell.text, textRect.width()));
    }
}

void ShadTablePrivate::paintCaption(QPainter* painter, int top) const
{
    Q_Q(const ShadTable);
    if (_caption.isEmpty())
    {
        return;
    }
    // caption-bottom 默认水平居中
    painter->setFont(q->font());
    painter->setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
    painter->drawText(QRect(0, top + kCaptionTop, q->viewport()->width(), kCaptionHeight), Qt::AlignHCenter | Qt::AlignVCenter, _caption);
}
