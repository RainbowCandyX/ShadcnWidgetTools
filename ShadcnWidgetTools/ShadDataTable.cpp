#include "ShadDataTable.h"

#include <QAbstractItemModel>
#include <QEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSortFilterProxyModel>
#include <QString>
#include <QVBoxLayout>

#include "ShadButton.h"
#include "ShadDataTableModel.h"
#include "ShadDataTablePrivate.h"
#include "ShadDropdownMenu.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadPagination.h"
#include "ShadStyleHelper.h"
#include "ShadTable.h"
#include "ShadTheme.h"

namespace
{
    // 工具栏与分页栏 py-4，扣除组件四周的焦点环边距
    constexpr int kBarPaddingY = 16 - ShadStyleHelper::kFocusRingWidth;
    // 筛选输入框 max-w-sm（含左右焦点环边距）
    constexpr int kFilterMaxWidth = 384 + 2 * ShadStyleHelper::kFocusRingWidth;
    // 已选择行数与分页之间的间距，扣除分页的焦点环边距
    constexpr int kPaginationSpacing = 16 - ShadStyleHelper::kFocusRingWidth;
    constexpr int kBorderWidth = 1;

    // 包裹表格的圆角边框，对应 overflow-hidden rounded-md border
    class TableFrame : public QWidget
    {
        public:
            explicit TableFrame(QWidget* parent = nullptr)
                : QWidget(parent)
            {
                connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
            }

        protected:
            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                QPainter painter(this);
                ShadStyleHelper::strokeRoundedRect(&painter, QRectF(rect()), shadTheme->getRadius(ShadThemeType::Medium), shadTheme->getColor(ShadThemeType::Border));
            }
    };
} // namespace

ShadDataTable::ShadDataTable(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadDataTablePrivate())
{
    Q_D(ShadDataTable);
    d->q_ptr = this;

    // 数据流：源模型 -> 排序筛选代理 -> 分页与附加列 -> ShadTable
    d->_proxyModel = new QSortFilterProxyModel(this);
    d->_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    d->_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    d->_tableModel = new ShadDataTableModel(this);
    d->_tableModel->setProxyModel(d->_proxyModel);
    d->_tableModel->setPage(0, d->_pageSize);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 工具栏：flex items-center py-4
    d->_toolbar = new QWidget(this);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(d->_toolbar);
    toolbarLayout->setContentsMargins(0, kBarPaddingY, 0, kBarPaddingY);
    toolbarLayout->setSpacing(0);
    d->_filterInput = new ShadInput(d->_toolbar);
    d->_filterInput->setMaximumWidth(kFilterMaxWidth);
    d->_columnsButton = new ShadButton(QString(), ShadButtonType::Outline, d->_toolbar);
    d->_columnsButton->setShadIcon(ShadIconType::Columns3);
    d->_columnsMenu = new ShadDropdownMenu(this);
    toolbarLayout->addWidget(d->_filterInput, 1);
    toolbarLayout->addStretch(0);
    toolbarLayout->addWidget(d->_columnsButton);
    mainLayout->addWidget(d->_toolbar);

    TableFrame* tableFrame = new TableFrame(this);
    QVBoxLayout* frameLayout = new QVBoxLayout(tableFrame);
    frameLayout->setContentsMargins(kBorderWidth, kBorderWidth, kBorderWidth, kBorderWidth);
    frameLayout->setSpacing(0);
    d->_table = new ShadTable(tableFrame);
    d->_table->setModel(d->_tableModel);
    frameLayout->addWidget(d->_table);
    mainLayout->addWidget(tableFrame);

    // 分页栏：flex items-center justify-end space-x-2 py-4，左侧 flex-1 text-sm text-muted-foreground
    QWidget* paginationBar = new QWidget(this);
    QHBoxLayout* paginationLayout = new QHBoxLayout(paginationBar);
    paginationLayout->setContentsMargins(0, kBarPaddingY, 0, kBarPaddingY);
    paginationLayout->setSpacing(kPaginationSpacing);
    d->_summaryLabel = new ShadLabel(paginationBar);
    d->_summaryLabel->setColorRole(ShadThemeType::MutedForeground);
    ShadStyleHelper::applyFont(d->_summaryLabel, 14, QFont::Normal);
    // 分页靠结束侧；没有已选择行数时分页占满整行，内容仍靠结束侧
    d->_pagination = new ShadPagination(paginationBar);
    d->_pagination->setAlignment(Qt::AlignRight);
    paginationLayout->addWidget(d->_summaryLabel, 1);
    paginationLayout->addWidget(d->_pagination);
    mainLayout->addWidget(paginationBar);

    connect(d->_filterInput, &QLineEdit::textChanged, this, [d](const QString& text)
    {
        d->_proxyModel->setFilterFixedString(text);
        // 筛选后回到第一页
        d->_tableModel->setPage(0, d->_pageSize);
    });
    connect(d->_columnsButton, &ShadButton::clicked, this, [d]() { d->showColumnsMenu(); });
    // 分页的页码从 1 开始，表格模型的页序号从 0 开始
    connect(d->_pagination, &ShadPagination::currentPageChanged, this, [d](int page) { d->_tableModel->setPage(page - 1, d->_pageSize); });
    connect(d->_table, &ShadTable::sortRequested, this, [d](int tableColumn) { d->handleSortRequested(tableColumn); });
    connect(d->_table, &ShadTable::actionRequested, this, [d](int tableRow, const QRect& buttonRect) { d->handleActionRequested(tableRow, buttonRect); });
    // 拖动调整的列宽按源模型列号保存（代理模型不改变列号）
    connect(d->_table, &ShadTable::columnResized, this, [this, d](int tableColumn, int width)
    {
        const int column = d->_tableModel->proxyColumn(tableColumn);
        if (column < 0)
        {
            return;
        }
        if (width < 0)
        {
            d->_userWidths.remove(column);
        }
        else
        {
            d->_userWidths.insert(column, width);
        }
        Q_EMIT columnResized(column, width);
    });
    connect(d->_tableModel, &ShadDataTableModel::pageStateChanged, this, [d]() { d->updatePagination(); });
    connect(d->_tableModel, &ShadDataTableModel::selectionChanged, this, [this, d]()
    {
        d->updatePagination();
        Q_EMIT selectionChanged();
    });

    d->updateToolbar();
    // setSelectionSummaryFormat 会调用 updatePagination
    retranslate();
}

ShadDataTable::~ShadDataTable()
{
}

void ShadDataTable::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}

void ShadDataTable::retranslate()
{
    Q_D(ShadDataTable);
    ShadInput* filterInput = d->_filterInput;
    ShadButton* columnsButton = d->_columnsButton;
    ShadTable* table = d->_table;
    ShadStyleHelper::applyDefaultText(filterInput, "placeholderText", filterInput->placeholderText(), tr("筛选..."), [filterInput](const QString& text) { filterInput->setPlaceholderText(text); });
    ShadStyleHelper::applyDefaultText(columnsButton, "text", columnsButton->text(), tr("列"), [columnsButton](const QString& text) { columnsButton->setText(text); });
    ShadStyleHelper::applyDefaultText(table, "emptyText", table->getEmptyText(), tr("没有结果"), [table](const QString& text) { table->setEmptyText(text); });
    ShadStyleHelper::applyDefaultText(this, "selectionSummaryFormat", d->_summaryFormat, tr("已选择 %1 / %2 行"), [this](const QString& text) { setSelectionSummaryFormat(text); });
}

void ShadDataTable::setSourceModel(QAbstractItemModel* model)
{
    Q_D(ShadDataTable);
    d->_sourceModel = model;
    d->_hiddenColumns.clear();
    d->_sortColumn = -1;
    d->_sortOrder = Qt::AscendingOrder;
    // 先恢复原始顺序再更换模型，-1 表示不排序
    d->_proxyModel->sort(-1);
    d->_proxyModel->setSourceModel(model);
    d->_tableModel->clearSelection();
    d->_tableModel->setPage(0, d->_pageSize);
    d->updateVisibleColumns();
    d->updatePagination();
}

QAbstractItemModel* ShadDataTable::getSourceModel() const
{
    Q_D(const ShadDataTable);
    return d->_sourceModel.data();
}

void ShadDataTable::setFilterColumn(int column)
{
    Q_D(ShadDataTable);
    d->_filterColumn = column;
    d->_proxyModel->setFilterKeyColumn(column);
    d->updateToolbar();
}

int ShadDataTable::getFilterColumn() const
{
    Q_D(const ShadDataTable);
    return d->_filterColumn;
}

void ShadDataTable::setFilterPlaceholderText(const QString& text)
{
    Q_D(ShadDataTable);
    d->_filterInput->setPlaceholderText(text);
}

void ShadDataTable::setColumnSortable(int column, bool sortable)
{
    Q_D(ShadDataTable);
    if (sortable)
    {
        d->_sortableColumns.insert(column);
    }
    else
    {
        d->_sortableColumns.remove(column);
    }
    d->updateTableColumns();
}

void ShadDataTable::setSortRole(int role)
{
    Q_D(ShadDataTable);
    d->_proxyModel->setSortRole(role);
}

void ShadDataTable::setColumnHideable(int column, bool hideable)
{
    Q_D(ShadDataTable);
    if (hideable)
    {
        d->_hideableColumns.insert(column);
    }
    else
    {
        d->_hideableColumns.remove(column);
    }
    d->updateToolbar();
}

void ShadDataTable::setColumnVisible(int column, bool visible)
{
    Q_D(ShadDataTable);
    if (visible == !d->_hiddenColumns.contains(column))
    {
        return;
    }
    if (visible)
    {
        d->_hiddenColumns.remove(column);
    }
    else
    {
        d->_hiddenColumns.insert(column);
    }
    d->updateVisibleColumns();
}

bool ShadDataTable::isColumnVisible(int column) const
{
    Q_D(const ShadDataTable);
    return !d->_hiddenColumns.contains(column);
}

void ShadDataTable::setColumnFixedWidth(int column, int width)
{
    Q_D(ShadDataTable);
    if (width < 0)
    {
        d->_fixedWidths.remove(column);
    }
    else
    {
        d->_fixedWidths.insert(column, width);
    }
    d->updateTableColumns();
}

void ShadDataTable::setColumnImage(int column, const QSize& size, ShadTableType::ImageShape shape)
{
    Q_D(ShadDataTable);
    if (column < 0 || size.isEmpty())
    {
        return;
    }
    ShadDataTablePrivate::ImageColumn image;
    image.size = size;
    image.shape = shape;
    d->_imageColumns.insert(column, image);
    d->updateTableColumns();
}

void ShadDataTable::clearColumnImage(int column)
{
    Q_D(ShadDataTable);
    if (d->_imageColumns.remove(column) == 0)
    {
        return;
    }
    d->updateTableColumns();
}

void ShadDataTable::setColumnsResizable(bool resizable)
{
    Q_D(ShadDataTable);
    d->_table->setColumnsResizable(resizable);
}

bool ShadDataTable::isColumnsResizable() const
{
    Q_D(const ShadDataTable);
    return d->_table->isColumnsResizable();
}

void ShadDataTable::setColumnUserWidth(int column, int width)
{
    Q_D(ShadDataTable);
    if (column < 0)
    {
        return;
    }
    if (width < 0)
    {
        d->_userWidths.remove(column);
    }
    else
    {
        d->_userWidths.insert(column, width);
    }
    d->updateTableColumns();
}

int ShadDataTable::getColumnUserWidth(int column) const
{
    Q_D(const ShadDataTable);
    return d->_userWidths.value(column, -1);
}

void ShadDataTable::resetColumnWidths()
{
    Q_D(ShadDataTable);
    d->_userWidths.clear();
    d->_table->resetColumnWidths();
}

void ShadDataTable::setSelectionEnabled(bool enabled)
{
    Q_D(ShadDataTable);
    d->_tableModel->setSelectionEnabled(enabled);
    d->updateTableColumns();
    d->updatePagination();
}

bool ShadDataTable::isSelectionEnabled() const
{
    Q_D(const ShadDataTable);
    return d->_tableModel->selectionColumn() >= 0;
}

QList<int> ShadDataTable::getSelectedRows() const
{
    Q_D(const ShadDataTable);
    return d->_tableModel->getSelectedSourceRows();
}

void ShadDataTable::clearSelection()
{
    Q_D(ShadDataTable);
    d->_tableModel->clearSelection();
}

void ShadDataTable::setRowActionMenu(ShadDropdownMenu* menu)
{
    Q_D(ShadDataTable);
    d->_rowActionMenu = menu;
    d->_tableModel->setActionEnabled(menu != nullptr);
    d->updateTableColumns();
}

int ShadDataTable::getActionRow() const
{
    Q_D(const ShadDataTable);
    return d->_actionRow;
}

void ShadDataTable::setPageSize(int pageSize)
{
    Q_D(ShadDataTable);
    d->_pageSize = qMax(1, pageSize);
    d->_tableModel->setPage(0, d->_pageSize);
}

int ShadDataTable::getPageSize() const
{
    Q_D(const ShadDataTable);
    return d->_pageSize;
}

void ShadDataTable::setPageIndex(int pageIndex)
{
    Q_D(ShadDataTable);
    d->_tableModel->setPage(pageIndex, d->_pageSize);
}

int ShadDataTable::getPageIndex() const
{
    Q_D(const ShadDataTable);
    return d->_tableModel->getPageIndex();
}

int ShadDataTable::getPageCount() const
{
    Q_D(const ShadDataTable);
    return d->_tableModel->getPageCount();
}

void ShadDataTable::setEmptyText(const QString& text)
{
    Q_D(ShadDataTable);
    d->_table->setEmptyText(text);
}

void ShadDataTable::setColumnsButtonText(const QString& text)
{
    Q_D(ShadDataTable);
    d->_columnsButton->setText(text);
}

void ShadDataTable::setPaginationTexts(const QString& previousText, const QString& nextText)
{
    Q_D(ShadDataTable);
    d->_pagination->setPreviousText(previousText);
    d->_pagination->setNextText(nextText);
}

void ShadDataTable::setSelectionSummaryFormat(const QString& format)
{
    Q_D(ShadDataTable);
    d->_summaryFormat = format;
    d->updatePagination();
}

ShadTable* ShadDataTable::getTable() const
{
    Q_D(const ShadDataTable);
    return d->_table;
}

ShadPagination* ShadDataTable::getPagination() const
{
    Q_D(const ShadDataTable);
    return d->_pagination;
}
