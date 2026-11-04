#include "ShadTable.h"

#include <QAbstractItemModel>
#include <QEvent>
#include <QFrame>
#include <QHeaderView>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadScrollBar.h"
#include "ShadStyleHelper.h"
#include "ShadTableDelegate.h"
#include "ShadTableHeader.h"
#include "ShadTablePrivate.h"
#include "ShadTheme.h"

namespace
{
    // 关闭自动高度时的默认高度
    constexpr int kDefaultScrollHeight = 320;
    // 最小宽度，内容更宽时横向滚动
    constexpr int kMinimumWidth = 120;
} // namespace

ShadTable::ShadTable(QWidget* parent)
    : QTableView(parent), d_ptr(new ShadTablePrivate())
{
    Q_D(ShadTable);
    d->q_ptr = this;
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->_header = new ShadTableHeader(this);
    setHorizontalHeader(d->_header);
    d->_delegate = new ShadTableDelegate(this);
    setItemDelegate(d->_delegate);

    verticalHeader()->hide();
    verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    verticalHeader()->setMinimumSectionSize(0);
    verticalHeader()->setDefaultSectionSize(d->rowHeight());
    setCornerButtonEnabled(false);
    setShowGrid(false);
    setWordWrap(false);
    setFrameShape(QFrame::NoFrame);
    // 只用于展示：不使用视图自身的选择与编辑，行选择由行选择列的 CheckStateRole 表示
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBar(new ShadScrollBar(Qt::Horizontal, this));
    setVerticalScrollBar(new ShadScrollBar(Qt::Vertical, this));
    viewport()->setMouseTracking(true);
    viewport()->setAutoFillBackground(false);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    d->_header->setResizeEnabled(d->_isColumnsResizable);
    // 拖动中保存宽度并只重新分配列宽，不重新测量内容
    connect(d->_header, &ShadTableHeader::columnResizing, this, [d](int column, int width)
    {
        d->_userWidths.insert(column, width);
        d->applyColumnWidths();
    });
    connect(d->_header, &ShadTableHeader::columnResizeFinished, this, &ShadTable::columnResized);
    connect(d->_header, &ShadTableHeader::columnResetRequested, this, [this, d](int column)
    {
        if (d->_userWidths.remove(column) == 0)
        {
            return;
        }
        d->scheduleLayout();
        Q_EMIT columnResized(column, -1);
    });
    connect(d->_header, &ShadTableHeader::sortClicked, this, &ShadTable::sortRequested);
    connect(d->_header, &ShadTableHeader::selectAllClicked, this, [d]() { d->toggleAllChecked(); });
    // 是否出现横向滚动条会影响自动高度
    connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, [this]() { updateGeometry(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { viewport()->update(); });
}

ShadTable::~ShadTable()
{
}

void ShadTable::setModel(QAbstractItemModel* model)
{
    Q_D(ShadTable);
    d->disconnectModel();
    QTableView::setModel(model);
    d->connectModel();
    d->clearHover();
    d->scheduleLayout();
}

void ShadTable::setColumnFixedWidth(int column, int width)
{
    Q_D(ShadTable);
    if (width < 0)
    {
        d->_fixedWidths.remove(column);
    }
    else
    {
        d->_fixedWidths.insert(column, width);
    }
    d->scheduleLayout();
}

void ShadTable::setFooterText(int column, const QString& text, Qt::Alignment alignment, int span)
{
    Q_D(ShadTable);
    ShadTablePrivate::FooterCell cell;
    cell.column = column;
    cell.text = text;
    cell.alignment = alignment;
    cell.span = qMax(1, span);
    bool isReplaced = false;
    for (ShadTablePrivate::FooterCell& existingCell : d->_footerCells)
    {
        if (existingCell.column == column)
        {
            existingCell = cell;
            isReplaced = true;
            break;
        }
    }
    if (!isReplaced)
    {
        d->_footerCells.append(cell);
    }
    updateGeometry();
    viewport()->update();
}

void ShadTable::clearFooter()
{
    Q_D(ShadTable);
    d->_footerCells.clear();
    updateGeometry();
    viewport()->update();
}

void ShadTable::setCaption(const QString& caption)
{
    Q_D(ShadTable);
    d->_caption = caption;
    updateGeometry();
    viewport()->update();
}

QString ShadTable::getCaption() const
{
    Q_D(const ShadTable);
    return d->_caption;
}

void ShadTable::setEmptyText(const QString& text)
{
    Q_D(ShadTable);
    d->_emptyText = text;
    updateGeometry();
    viewport()->update();
}

QString ShadTable::getEmptyText() const
{
    Q_D(const ShadTable);
    return d->_emptyText;
}

void ShadTable::setAutoHeight(bool autoHeight)
{
    Q_D(ShadTable);
    if (d->_isAutoHeight == autoHeight)
    {
        return;
    }
    d->_isAutoHeight = autoHeight;
    setVerticalScrollBarPolicy(autoHeight ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
    setSizePolicy(QSizePolicy::Expanding, autoHeight ? QSizePolicy::Fixed : QSizePolicy::Expanding);
    updateGeometry();
}

bool ShadTable::isAutoHeight() const
{
    Q_D(const ShadTable);
    return d->_isAutoHeight;
}

void ShadTable::setSelectionColumn(int column)
{
    Q_D(ShadTable);
    d->_selectionColumn = column;
    d->_header->setSelectionColumn(column);
    d->_delegate->setSelectionColumn(column);
    d->scheduleLayout();
}

int ShadTable::getSelectionColumn() const
{
    Q_D(const ShadTable);
    return d->_selectionColumn;
}

void ShadTable::setActionColumn(int column)
{
    Q_D(ShadTable);
    d->_actionColumn = column;
    d->_delegate->setActionColumn(column);
    d->scheduleLayout();
}

int ShadTable::getActionColumn() const
{
    Q_D(const ShadTable);
    return d->_actionColumn;
}

void ShadTable::setColumnSortable(int column, bool sortable)
{
    Q_D(ShadTable);
    d->_header->setColumnSortable(column, sortable);
    d->scheduleLayout();
}

void ShadTable::setSortIndicator(int column, Qt::SortOrder order)
{
    Q_D(ShadTable);
    d->_header->setSortState(column, order);
}

void ShadTable::setColumnImage(int column, const QSize& size, ShadTableType::ImageShape shape)
{
    Q_D(ShadTable);
    if (column < 0 || size.isEmpty())
    {
        return;
    }
    ShadTableDelegate::ImageColumn image;
    image.size = size;
    image.shape = shape;
    d->_imageColumns.insert(column, image);
    d->_delegate->setImageColumns(d->_imageColumns);
    d->scheduleLayout();
}

void ShadTable::clearColumnImage(int column)
{
    Q_D(ShadTable);
    if (d->_imageColumns.remove(column) == 0)
    {
        return;
    }
    d->_delegate->setImageColumns(d->_imageColumns);
    d->scheduleLayout();
}

void ShadTable::setDefaultRowHeight(int height)
{
    Q_D(ShadTable);
    d->_defaultRowHeight = height > 0 ? height : -1;
    d->scheduleLayout();
}

int ShadTable::getDefaultRowHeight() const
{
    Q_D(const ShadTable);
    return d->_defaultRowHeight;
}

void ShadTable::setColumnsResizable(bool resizable)
{
    Q_D(ShadTable);
    d->_isColumnsResizable = resizable;
    d->_header->setResizeEnabled(resizable);
}

bool ShadTable::isColumnsResizable() const
{
    Q_D(const ShadTable);
    return d->_isColumnsResizable;
}

void ShadTable::setColumnUserWidth(int column, int width)
{
    Q_D(ShadTable);
    if (column < 0)
    {
        return;
    }
    if (width < 0)
    {
        if (d->_userWidths.remove(column) == 0)
        {
            return;
        }
    }
    else
    {
        d->_userWidths.insert(column, qMax(ShadTableHeader::kMinimumResizeWidth, width));
    }
    d->scheduleLayout();
}

int ShadTable::getColumnUserWidth(int column) const
{
    Q_D(const ShadTable);
    return d->_userWidths.value(column, -1);
}

void ShadTable::resetColumnWidths()
{
    Q_D(ShadTable);
    if (d->_userWidths.isEmpty())
    {
        return;
    }
    d->_userWidths.clear();
    d->scheduleLayout();
}

QSize ShadTable::sizeHint() const
{
    Q_D(const ShadTable);
    const int frame = 2 * frameWidth();
    const int width = d->totalNaturalWidth() + frame;
    if (!d->_isAutoHeight)
    {
        return QSize(width, kDefaultScrollHeight);
    }
    const int headerHeight = horizontalHeader()->isHidden() ? 0 : horizontalHeader()->sizeHint().height();
    int height = headerHeight + d->contentHeight() + frame;
    if (horizontalScrollBar()->maximum() > horizontalScrollBar()->minimum())
    {
        height += horizontalScrollBar()->sizeHint().height();
    }
    return QSize(width, height);
}

QSize ShadTable::minimumSizeHint() const
{
    Q_D(const ShadTable);
    if (!d->_isAutoHeight)
    {
        const int headerHeight = horizontalHeader()->isHidden() ? 0 : horizontalHeader()->sizeHint().height();
        return QSize(kMinimumWidth, headerHeight + d->rowHeight());
    }
    return QSize(kMinimumWidth, sizeHint().height());
}

bool ShadTable::viewportEvent(QEvent* event)
{
    Q_D(ShadTable);
    if (event->type() == QEvent::Leave)
    {
        d->clearHover();
    }
    return QTableView::viewportEvent(event);
}

void ShadTable::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadTable);
    d->updateHover(event->position().toPoint());
    QTableView::mouseMoveEvent(event);
}

void ShadTable::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadTable);
    if (event->button() == Qt::LeftButton)
    {
        const QPoint pos = event->position().toPoint();
        const QModelIndex index = indexAt(pos);
        // 行选择列整个单元格都可以点击切换（对应 [role=checkbox] 单元格）
        if (index.isValid() && index.column() == d->_selectionColumn)
        {
            d->toggleRowChecked(index);
            event->accept();
            return;
        }
        if (index.isValid() && index.column() == d->_actionColumn)
        {
            const QRect buttonRect = d->_delegate->actionButtonRect(visualRect(index), index, layoutDirection());
            if (buttonRect.contains(pos))
            {
                Q_EMIT actionRequested(index.row(), QRect(viewport()->mapToGlobal(buttonRect.topLeft()), buttonRect.size()));
                event->accept();
                return;
            }
        }
    }
    QTableView::mouseReleaseEvent(event);
}

void ShadTable::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_D(ShadTable);
    // 行选择列与行操作列的双击按两次单击处理，不触发视图的 doubleClicked
    const QModelIndex index = indexAt(event->position().toPoint());
    if (index.isValid() && (index.column() == d->_selectionColumn || index.column() == d->_actionColumn))
    {
        event->accept();
        return;
    }
    QTableView::mouseDoubleClickEvent(event);
}

void ShadTable::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadTable);
    QTableView::resizeEvent(event);
    // 宽度变化时立即重新分配列宽，避免拖动窗口时列宽滞后
    d->updateLayout();
}

void ShadTable::paintEvent(QPaintEvent* event)
{
    Q_D(ShadTable);
    QTableView::paintEvent(event);
    // 行之后依次绘制空状态行、表尾与说明文字
    QPainter painter(viewport());
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    const int rows = d->rowCount();
    int top = rows > 0 ? rowViewportPosition(rows - 1) + rowHeight(rows - 1) : 0;
    if (rows == 0 && !d->_emptyText.isEmpty())
    {
        painter.setFont(font());
        painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
        painter.drawText(QRect(0, top, viewport()->width(), ShadTablePrivate::kEmptyHeight), Qt::AlignCenter, d->_emptyText);
        top += ShadTablePrivate::kEmptyHeight;
    }
    d->paintFooter(&painter, top);
    d->paintCaption(&painter, top + d->footerHeight());
}
