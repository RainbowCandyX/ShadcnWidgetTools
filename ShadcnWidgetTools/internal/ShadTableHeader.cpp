#include "ShadTableHeader.h"

#include <QAbstractItemModel>
#include <QColor>
#include <QEvent>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QSet>
#include <QVariant>
#include <QtGlobal>

#include <utility>

#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 可排序列的 ghost 按钮：h-9 has-[>svg]:px-3，排序图标 ml-2 h-4 w-4
    constexpr int kButtonHeight = 36;
    constexpr int kButtonPaddingX = 12;
    constexpr int kIconGap = 8;
    constexpr int kIconSize = 16;
    constexpr int kCheckboxSize = 16;
    // 拖动把手：列结束边缘左右各 4px；悬停与拖动时显示的竖条宽 2px
    constexpr int kResizeHandleHalfWidth = 4;
    constexpr int kResizeBarWidth = 2;
} // namespace

ShadTableHeader::ShadTableHeader(QWidget* parent)
    : QHeaderView(Qt::Horizontal, parent)
{
    // 列宽由 ShadTable 计算，点击由本类自行处理（只响应排序按钮与全选复选框）
    setSectionsClickable(false);
    setSectionsMovable(false);
    setHighlightSections(false);
    setSectionResizeMode(QHeaderView::Fixed);
    setMinimumSectionSize(0);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    viewport()->setAutoFillBackground(false);
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { viewport()->update(); });
}

ShadTableHeader::~ShadTableHeader()
{
}

void ShadTableHeader::setColumnSortable(int column, bool sortable)
{
    if (sortable)
    {
        _sortableColumns.insert(column);
    }
    else
    {
        _sortableColumns.remove(column);
    }
    viewport()->update();
}

bool ShadTableHeader::isColumnSortable(int column) const
{
    return _sortableColumns.contains(column);
}

void ShadTableHeader::setSortState(int column, Qt::SortOrder order)
{
    _sortColumn = column;
    _sortOrder = order;
    viewport()->update();
}

void ShadTableHeader::setSelectionColumn(int column)
{
    _selectionColumn = column;
    viewport()->update();
}

int ShadTableHeader::contentWidth(int column) const
{
    if (column == _selectionColumn)
    {
        // 复选框单元格 pr-0
        return kCellPaddingX + kCheckboxSize;
    }
    const int textWidth = ShadStyleHelper::textWidth(font(), headerText(column));
    if (isColumnSortable(column))
    {
        return 2 * kCellPaddingX + 2 * kButtonPaddingX + textWidth + kIconGap + kIconSize;
    }
    return 2 * kCellPaddingX + textWidth;
}

Qt::CheckState ShadTableHeader::selectionState() const
{
    const QAbstractItemModel* headerModel = model();
    if (headerModel == nullptr || _selectionColumn < 0)
    {
        return Qt::Unchecked;
    }
    const int rowCount = headerModel->rowCount();
    int checkedCount = 0;
    for (int row = 0; row < rowCount; ++row)
    {
        if (headerModel->index(row, _selectionColumn).data(Qt::CheckStateRole).toInt() == Qt::Checked)
        {
            ++checkedCount;
        }
    }
    if (checkedCount == 0)
    {
        return Qt::Unchecked;
    }
    return checkedCount == rowCount ? Qt::Checked : Qt::PartiallyChecked;
}

void ShadTableHeader::setResizeEnabled(bool enabled)
{
    if (_isResizeEnabled == enabled)
    {
        return;
    }
    _isResizeEnabled = enabled;
    _resizingColumn = -1;
    setHoveredResizeColumn(-1);
}

bool ShadTableHeader::isResizeEnabled() const
{
    return _isResizeEnabled;
}

void ShadTableHeader::setNonResizableColumns(const QSet<int>& columns)
{
    _nonResizableColumns = columns;
}

QSize ShadTableHeader::sizeHint() const
{
    return QSize(QHeaderView::sizeHint().width(), kHeaderHeight);
}

void ShadTableHeader::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    // 不调用 QHeaderView::paintEvent，避免绘制原生样式的表头背景
    QPainter painter(viewport());
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    for (int visualIndex = 0; visualIndex < count(); ++visualIndex)
    {
        const int column = logicalIndex(visualIndex);
        if (!isSectionHidden(column))
        {
            paintSection(&painter, sectionRect(column), column);
        }
    }
    // 对应 TableHeader 的 [&_tr]:border-b
    painter.fillRect(QRect(0, viewport()->height() - 1, viewport()->width(), 1), shadTheme->getColor(ShadThemeType::Border));
    // 拖动把手的竖条：悬停为 border 色，拖动中为 primary 色
    const int barColumn = _resizingColumn >= 0 ? _resizingColumn : _hoveredResizeColumn;
    if (barColumn >= 0 && barColumn < count() && !isSectionHidden(barColumn))
    {
        const QColor barColor = shadTheme->getColor(_resizingColumn >= 0 ? ShadThemeType::Primary : ShadThemeType::Border);
        painter.fillRect(QRect(resizeEdge(barColumn) - kResizeBarWidth / 2, 0, kResizeBarWidth, viewport()->height()), barColor);
    }
}

void ShadTableHeader::paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const
{
    using namespace ShadStyleHelper;
    if (logicalIndex == _selectionColumn)
    {
        drawCheckbox(painter, QRectF(checkboxRect()), selectionState());
        return;
    }
    const QString text = headerText(logicalIndex);
    painter->setFont(font());
    if (!isColumnSortable(logicalIndex))
    {
        const QRect textRect = rect.adjusted(kCellPaddingX, 0, -kCellPaddingX, 0);
        painter->setPen(shadTheme->getColor(ShadThemeType::Foreground));
        painter->drawText(textRect, visualAlignment(layoutDirection(), headerAlignment(logicalIndex)), elideText(font(), text, textRect.width()));
        return;
    }
    // ghost 按钮：hover:bg-accent hover:text-accent-foreground（暗色 hover:bg-accent/50）
    const QRect buttonRect = sortButtonRect(logicalIndex);
    const bool isHovered = logicalIndex == _hoveredSortColumn;
    if (isHovered)
    {
        const QColor accent = shadTheme->getColor(ShadThemeType::Accent);
        fillRoundedRect(painter, QRectF(buttonRect), shadTheme->getRadius(ShadThemeType::Medium), shadTheme->isDark() ? withAlpha(accent, 0.5) : accent);
    }
    const QColor color = shadTheme->getColor(isHovered ? ShadThemeType::AccentForeground : ShadThemeType::Foreground);
    // 文字在起始侧、排序图标在结束侧，按 LTR 计算后在按钮内镜像
    // QHeaderView 自身有 visualRect(QModelIndex) 成员会遮蔽 using namespace，这里必须写全限定名
    const Qt::LayoutDirection direction = layoutDirection();
    const int textWidth = ShadStyleHelper::textWidth(font(), text);
    const QRect textRect(buttonRect.left() + kButtonPaddingX, buttonRect.top(), textWidth, buttonRect.height());
    painter->setPen(color);
    painter->drawText(ShadStyleHelper::visualRect(direction, buttonRect, textRect), Qt::AlignCenter, text);
    // 未排序显示 arrow-up-down，升序 arrow-up，降序 arrow-down
    ShadIconType::IconName icon = ShadIconType::ArrowUpDown;
    if (logicalIndex == _sortColumn)
    {
        icon = _sortOrder == Qt::AscendingOrder ? ShadIconType::ArrowUp : ShadIconType::ArrowDown;
    }
    const QRectF iconRect(textRect.right() + 1 + kIconGap, buttonRect.top() + (buttonRect.height() - kIconSize) / 2.0, kIconSize, kIconSize);
    ShadIcon::paint(painter, ShadStyleHelper::visualRect(direction, QRectF(buttonRect), iconRect), icon, color, kIconSize);
}

void ShadTableHeader::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint pos = event->position().toPoint();
    if (_resizingColumn >= 0)
    {
        // 向结束方向拖动变宽，RTL 时结束方向为左侧
        const int delta = isRightToLeft() ? _resizeStartX - pos.x() : pos.x() - _resizeStartX;
        Q_EMIT columnResizing(_resizingColumn, qMax(kMinimumResizeWidth, _resizeStartWidth + delta));
        event->accept();
        return;
    }
    const int resizeColumn = resizeHandleAt(pos);
    setHoveredResizeColumn(resizeColumn);
    if (resizeColumn >= 0)
    {
        setHoveredSortColumn(-1);
        viewport()->setCursor(Qt::SplitHCursor);
        event->accept();
        return;
    }
    int hoveredColumn = -1;
    for (const int column : std::as_const(_sortableColumns))
    {
        if (column < count() && !isSectionHidden(column) && sortButtonRect(column).contains(pos))
        {
            hoveredColumn = column;
            break;
        }
    }
    setHoveredSortColumn(hoveredColumn);
    const bool isOverCheckbox = _selectionColumn >= 0 && checkboxRect().contains(pos);
    viewport()->setCursor(hoveredColumn >= 0 || isOverCheckbox ? Qt::PointingHandCursor : Qt::ArrowCursor);
    event->accept();
}

void ShadTableHeader::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && _hoveredResizeColumn >= 0)
    {
        // 按下拖动把手：记录起始位置与列宽，拖动中按差值计算新宽度
        _resizingColumn = _hoveredResizeColumn;
        _resizeStartX = event->position().toPoint().x();
        _resizeStartWidth = sectionSize(_resizingColumn);
        _pressedSortColumn = -1;
        _isCheckboxPressed = false;
        viewport()->update();
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        _pressedSortColumn = _hoveredSortColumn;
        _isCheckboxPressed = _selectionColumn >= 0 && checkboxRect().contains(event->position().toPoint());
    }
    event->accept();
}

void ShadTableHeader::mouseReleaseEvent(QMouseEvent* event)
{
    const QPoint pos = event->position().toPoint();
    if (event->button() == Qt::LeftButton && _resizingColumn >= 0)
    {
        const int column = _resizingColumn;
        _resizingColumn = -1;
        setHoveredResizeColumn(resizeHandleAt(pos));
        viewport()->setCursor(_hoveredResizeColumn >= 0 ? Qt::SplitHCursor : Qt::ArrowCursor);
        viewport()->update();
        Q_EMIT columnResizeFinished(column, sectionSize(column));
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        if (_pressedSortColumn >= 0 && sortButtonRect(_pressedSortColumn).contains(pos))
        {
            Q_EMIT sortClicked(_pressedSortColumn);
        }
        else if (_isCheckboxPressed && checkboxRect().contains(pos))
        {
            Q_EMIT selectAllClicked();
        }
    }
    _pressedSortColumn = -1;
    _isCheckboxPressed = false;
    event->accept();
}

void ShadTableHeader::mouseDoubleClickEvent(QMouseEvent* event)
{
    // 双击拖动把手：恢复自动宽度
    const int resizeColumn = event->button() == Qt::LeftButton ? resizeHandleAt(event->position().toPoint()) : -1;
    if (resizeColumn >= 0)
    {
        Q_EMIT columnResetRequested(resizeColumn);
        event->accept();
        return;
    }
    // 不交给 QHeaderView：双击分隔处会自动调整列宽
    // 快速连点时第二次按下以双击事件送达，按一次普通按下处理，排序按钮与全选框松开时照常触发
    mousePressEvent(event);
}

void ShadTableHeader::leaveEvent(QEvent* event)
{
    setHoveredSortColumn(-1);
    // 拖动中光标可能移出表头，保持竖条直到松开
    if (_resizingColumn < 0)
    {
        setHoveredResizeColumn(-1);
    }
    QHeaderView::leaveEvent(event);
}

int ShadTableHeader::resizeEdge(int column) const
{
    const int position = sectionViewportPosition(column);
    return isRightToLeft() ? position : position + sectionSize(column);
}

int ShadTableHeader::resizeHandleAt(const QPoint& pos) const
{
    if (!_isResizeEnabled)
    {
        return -1;
    }
    // 多个边缘都在范围内时取最近的一个
    int result = -1;
    int bestDistance = kResizeHandleHalfWidth + 1;
    for (int column = 0; column < count(); ++column)
    {
        if (isSectionHidden(column) || _nonResizableColumns.contains(column))
        {
            continue;
        }
        const int distance = qAbs(pos.x() - resizeEdge(column));
        if (distance <= kResizeHandleHalfWidth && distance < bestDistance)
        {
            result = column;
            bestDistance = distance;
        }
    }
    return result;
}

void ShadTableHeader::setHoveredResizeColumn(int column)
{
    if (_hoveredResizeColumn == column)
    {
        return;
    }
    _hoveredResizeColumn = column;
    viewport()->update();
}

QRect ShadTableHeader::sortButtonRect(int column) const
{
    const QRect section = sectionRect(column);
    const int textWidth = ShadStyleHelper::textWidth(font(), headerText(column));
    const int buttonWidth = 2 * kButtonPaddingX + textWidth + kIconGap + kIconSize;
    const int top = (height() - kButtonHeight) / 2;
    // 表头对齐到结束侧时按钮靠结束侧，其余靠起始侧；按 LTR 计算后在区域内镜像
    const Qt::LayoutDirection direction = layoutDirection();
    const bool isEnd = ShadStyleHelper::logicalAlignment(direction, headerAlignment(column)).testFlag(Qt::AlignRight);
    const int left = isEnd ? section.right() + 1 - kCellPaddingX - buttonWidth : section.left() + kCellPaddingX;
    return ShadStyleHelper::visualRect(direction, section, QRect(left, top, buttonWidth, kButtonHeight));
}

QRect ShadTableHeader::checkboxRect() const
{
    if (_selectionColumn < 0 || _selectionColumn >= count() || isSectionHidden(_selectionColumn))
    {
        return QRect();
    }
    const QRect section = sectionRect(_selectionColumn);
    return ShadStyleHelper::visualRect(layoutDirection(), section, QRect(section.left() + kCellPaddingX, (height() - kCheckboxSize) / 2, kCheckboxSize, kCheckboxSize));
}

QRect ShadTableHeader::sectionRect(int column) const
{
    return QRect(sectionViewportPosition(column), 0, sectionSize(column), height());
}

QString ShadTableHeader::headerText(int column) const
{
    const QAbstractItemModel* headerModel = model();
    return headerModel != nullptr ? headerModel->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString() : QString();
}

Qt::Alignment ShadTableHeader::headerAlignment(int column) const
{
    // 默认 text-left align-middle；模型可通过 TextAlignmentRole 指定（例如金额列 text-right）
    const QAbstractItemModel* headerModel = model();
    const QVariant value = headerModel != nullptr ? headerModel->headerData(column, Qt::Horizontal, Qt::TextAlignmentRole) : QVariant();
    Qt::Alignment alignment = value.isValid() ? Qt::Alignment(value.toInt()) : Qt::Alignment(Qt::AlignLeft);
    if ((alignment & Qt::AlignVertical_Mask) == 0)
    {
        alignment |= Qt::AlignVCenter;
    }
    return alignment;
}

void ShadTableHeader::setHoveredSortColumn(int column)
{
    if (_hoveredSortColumn == column)
    {
        return;
    }
    _hoveredSortColumn = column;
    viewport()->update();
}
