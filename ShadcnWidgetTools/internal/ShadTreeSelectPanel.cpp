#include "ShadTreeSelectPanel.h"

#include <QAbstractItemModel>
#include <QColor>
#include <QFont>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QVariant>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadComboBoxSearch.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTreeView.h"

namespace
{
    constexpr int kSearchBarHeight = 36;
    constexpr int kSearchPaddingX = 12;
    constexpr int kSearchIconSize = 16;
    constexpr int kSearchIconGap = 8;
    constexpr int kPadding = 4;
    constexpr int kEmptyHeight = 68;
} // namespace

ShadTreeSelectPanel::ShadTreeSelectPanel(QWidget* parent)
    : QWidget(parent)
{
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    _searchEdit = new QLineEdit(this);
    _searchEdit->setFrame(false);
#ifdef Q_OS_MACOS
    // 避免 macOS 额外绘制系统焦点框
    _searchEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    ShadStyleHelper::applyFont(_searchEdit, 14, QFont::Normal);
    _searchEdit->setVisible(false);

    _treeView = new ShadTreeView(this);
    _treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 可见行数变化时更新弹出层高度
    connect(_treeView, &ShadTreeView::expanded, this, [this]() { updateGeometry(); });
    connect(_treeView, &ShadTreeView::collapsed, this, [this]() { updateGeometry(); });
    connect(_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) { applyFilter(text); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]()
    {
        applyThemeColors();
        update();
    });
    applyThemeColors();
}

ShadTreeSelectPanel::~ShadTreeSelectPanel()
{
}

ShadTreeView* ShadTreeSelectPanel::treeView() const
{
    return _treeView;
}

QLineEdit* ShadTreeSelectPanel::searchEdit() const
{
    return _searchEdit;
}

void ShadTreeSelectPanel::setSearchVisible(bool visible)
{
    if (_isSearchVisible == visible)
    {
        return;
    }
    _isSearchVisible = visible;
    _searchEdit->setVisible(visible);
    layoutContent();
    updateGeometry();
    update();
}

bool ShadTreeSelectPanel::isSearchVisible() const
{
    return _isSearchVisible;
}

void ShadTreeSelectPanel::setSearchPlaceholderText(const QString& text)
{
    _searchEdit->setPlaceholderText(text);
}

void ShadTreeSelectPanel::setEmptyText(const QString& text)
{
    _emptyText = text;
    update();
}

void ShadTreeSelectPanel::resetFilter()
{
    {
        // 清空搜索文字时不触发过滤，下面统一恢复
        const QSignalBlocker blocker(_searchEdit);
        _searchEdit->clear();
    }
    if (_isFiltering)
    {
        restoreRows();
    }
    _treeView->setVisible(true);
    layoutContent();
    updateGeometry();
    update();
}

QModelIndex ShadTreeSelectPanel::firstVisibleIndex() const
{
    const QAbstractItemModel* model = _treeView->model();
    if (model == nullptr)
    {
        return QModelIndex();
    }
    for (int row = 0; row < model->rowCount(); ++row)
    {
        if (!_treeView->isRowHidden(row, QModelIndex()))
        {
            return model->index(row, 0);
        }
    }
    return QModelIndex();
}

bool ShadTreeSelectPanel::isIndexUnhidden(const QModelIndex& index) const
{
    for (QModelIndex current = index; current.isValid(); current = current.parent())
    {
        if (_treeView->isRowHidden(current.row(), current.parent()))
        {
            return false;
        }
    }
    return index.isValid();
}

QSize ShadTreeSelectPanel::sizeHint() const
{
    int listHeight = kEmptyHeight;
    if (hasVisibleRows())
    {
        listHeight = qMin(visibleRowCount(QModelIndex()) * _treeView->getItemHeight() + 2 * kPadding, kMaxTreeHeight);
    }
    return QSize(kMinWidth, searchBarHeight() + listHeight);
}

QSize ShadTreeSelectPanel::minimumSizeHint() const
{
    // 浮层按 sizeHint 与 minimumSizeHint 中较大的高度布局，两者一致时高度只由可见行数决定
    return sizeHint();
}

void ShadTreeSelectPanel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    const QColor foreground = shadTheme->getColor(ShadThemeType::PopoverForeground);
    if (_isSearchVisible)
    {
        const QRectF iconRect(kSearchPaddingX, (kSearchBarHeight - kSearchIconSize) / 2.0, kSearchIconSize, kSearchIconSize);
        ShadIcon::paint(&painter, visualRect(layoutDirection(), QRectF(rect()), iconRect), ShadIconType::Search, withAlpha(foreground, 0.5), kSearchIconSize);
        painter.fillRect(QRectF(0, kSearchBarHeight, width(), 1), shadTheme->getColor(ShadThemeType::Border));
    }
    if (!hasVisibleRows())
    {
        const QRect emptyRect(0, searchBarHeight(), width(), height() - searchBarHeight());
        painter.setFont(font());
        painter.setPen(foreground);
        painter.drawText(emptyRect, Qt::AlignCenter, _emptyText);
    }
}

void ShadTreeSelectPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutContent();
}

void ShadTreeSelectPanel::applyThemeColors()
{
    using ShadStyleHelper::withAlpha;
    const QColor foreground = shadTheme->getColor(ShadThemeType::PopoverForeground);
    const QColor placeholder = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor selection = shadTheme->getColor(ShadThemeType::Primary);
    const QColor selectionText = shadTheme->getColor(ShadThemeType::PrimaryForeground);
    // 背景与分隔线由面板自绘
    const QString styleSheet = QStringLiteral("QLineEdit{background-color:transparent;border:none;padding:0px;color:%1;selection-background-color:%2;selection-color:%3;}").arg(foreground.name(QColor::HexArgb), selection.name(QColor::HexArgb), selectionText.name(QColor::HexArgb));
    _searchEdit->setStyleSheet(styleSheet);
    // 占位符颜色样式表无法设置，通过调色板设置
    QPalette palette = _searchEdit->palette();
    palette.setColor(QPalette::Active, QPalette::PlaceholderText, placeholder);
    palette.setColor(QPalette::Inactive, QPalette::PlaceholderText, placeholder);
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, withAlpha(placeholder, ShadStyleHelper::kDisabledOpacity));
    _searchEdit->setPalette(palette);
}

void ShadTreeSelectPanel::applyFilter(const QString& query)
{
    if (_treeView->model() == nullptr)
    {
        return;
    }
    QModelIndex firstMatch;
    if (query.trimmed().isEmpty())
    {
        if (_isFiltering)
        {
            restoreRows();
        }
    }
    else
    {
        if (!_isFiltering)
        {
            _savedExpandedRows.clear();
            saveExpandedRows(QModelIndex());
            _isFiltering = true;
        }
        filterRows(QModelIndex(), query, false, &firstMatch);
    }
    _treeView->setVisible(hasVisibleRows());
    layoutContent();
    updateGeometry();
    update();
    Q_EMIT filtered(firstMatch);
}

bool ShadTreeSelectPanel::filterRows(const QModelIndex& parent, const QString& query, bool keepAll, QModelIndex* firstMatch)
{
    const QAbstractItemModel* model = _treeView->model();
    bool hasMatch = false;
    for (int row = 0; row < model->rowCount(parent); ++row)
    {
        const QModelIndex index = model->index(row, 0, parent);
        const bool isSelfMatch = ShadComboBoxSearch::fuzzyScore(index.data(Qt::DisplayRole).toString(), query) > 0;
        // 先序记录第一个匹配项，与树中从上到下的顺序一致
        if (isSelfMatch && !firstMatch->isValid())
        {
            *firstMatch = index;
        }
        const bool hasChildMatch = filterRows(index, query, keepAll || isSelfMatch, firstMatch);
        _treeView->setRowHidden(row, parent, !(keepAll || isSelfMatch || hasChildMatch));
        // 只展开含匹配子孙的节点，匹配节点自身保持折叠
        if (hasChildMatch)
        {
            _treeView->expand(index);
        }
        else
        {
            _treeView->collapse(index);
        }
        hasMatch = hasMatch || isSelfMatch || hasChildMatch;
    }
    return hasMatch;
}

void ShadTreeSelectPanel::restoreRows()
{
    showAllRows(QModelIndex());
    _treeView->collapseAll();
    for (const QPersistentModelIndex& index : std::as_const(_savedExpandedRows))
    {
        if (index.isValid())
        {
            _treeView->expand(index);
        }
    }
    _savedExpandedRows.clear();
    _isFiltering = false;
}

void ShadTreeSelectPanel::showAllRows(const QModelIndex& parent)
{
    const QAbstractItemModel* model = _treeView->model();
    if (model == nullptr)
    {
        return;
    }
    for (int row = 0; row < model->rowCount(parent); ++row)
    {
        _treeView->setRowHidden(row, parent, false);
        showAllRows(model->index(row, 0, parent));
    }
}

void ShadTreeSelectPanel::saveExpandedRows(const QModelIndex& parent)
{
    const QAbstractItemModel* model = _treeView->model();
    for (int row = 0; row < model->rowCount(parent); ++row)
    {
        const QModelIndex index = model->index(row, 0, parent);
        if (_treeView->isExpanded(index))
        {
            _savedExpandedRows.append(QPersistentModelIndex(index));
            saveExpandedRows(index);
        }
    }
}

int ShadTreeSelectPanel::visibleRowCount(const QModelIndex& parent) const
{
    const QAbstractItemModel* model = _treeView->model();
    if (model == nullptr)
    {
        return 0;
    }
    int count = 0;
    for (int row = 0; row < model->rowCount(parent); ++row)
    {
        if (_treeView->isRowHidden(row, parent))
        {
            continue;
        }
        ++count;
        const QModelIndex index = model->index(row, 0, parent);
        if (_treeView->isExpanded(index))
        {
            count += visibleRowCount(index);
        }
    }
    return count;
}

bool ShadTreeSelectPanel::hasVisibleRows() const
{
    return firstVisibleIndex().isValid();
}

int ShadTreeSelectPanel::searchBarHeight() const
{
    return _isSearchVisible ? kSearchBarHeight + 1 : 0;
}

void ShadTreeSelectPanel::layoutContent()
{
    const int searchHeight = searchBarHeight();
    if (_isSearchVisible)
    {
        const int editLeft = kSearchPaddingX + kSearchIconSize + kSearchIconGap;
        const QRect editRect(editLeft, 0, width() - editLeft - kSearchPaddingX, kSearchBarHeight);
        _searchEdit->setGeometry(ShadStyleHelper::visualRect(layoutDirection(), rect(), editRect));
    }
    _treeView->setGeometry(kPadding, searchHeight + kPadding, qMax(0, width() - 2 * kPadding), qMax(0, height() - searchHeight - 2 * kPadding));
}
