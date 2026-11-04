#include "ShadTreeView.h"

#include <QAbstractItemView>
#include <QColor>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QFrame>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QStyleOptionViewItem>
#include <QtMath>

#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadScrollBar.h"
#include "ShadStyleHelper.h"
#include "ShadTableHeader.h"
#include "ShadTheme.h"
#include "ShadTreeDelegate.h"
#include "ShadTreeViewPrivate.h"
#include "ShadTreeViewStyle.h"

namespace
{
    // 键盘焦点环（ring-2），绘制在行背景内侧，不被相邻行覆盖
    constexpr qreal kFocusRingStroke = 2.0;
} // namespace

ShadTreeView::ShadTreeView(QWidget* parent)
    : QTreeView(parent), d_ptr(new ShadTreeViewPrivate())
{
    Q_D(ShadTreeView);
    d->q_ptr = this;
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->_style = new ShadTreeViewStyle(this);
    setStyle(d->_style);
    d->_delegate = new ShadTreeDelegate(this);
    d->_delegate->setItemHeight(ShadTreeViewPrivate::kItemHeight);
    setItemDelegate(d->_delegate);
    // 表头与 ShadTable 一致，默认隐藏；最后一列铺满剩余宽度
    d->_header = new ShadTableHeader(this);
    setHeader(d->_header);
    d->_header->setStretchLastSection(true);
    setHeaderHidden(true);

    setIndentation(ShadTreeViewPrivate::kIndentation);
    setRootIsDecorated(true);
    setUniformRowHeights(true);
    setAnimated(true);
    setAllColumnsShowFocus(true);
    setExpandsOnDoubleClick(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBar(new ShadScrollBar(Qt::Horizontal, this));
    setVerticalScrollBar(new ShadScrollBar(Qt::Vertical, this));
    viewport()->setMouseTracking(true);
    viewport()->setAutoFillBackground(false);

    connect(this, &QTreeView::expanded, this, [d](const QModelIndex& index) { d->animateChevron(index, true); });
    connect(this, &QTreeView::collapsed, this, [d](const QModelIndex& index) { d->animateChevron(index, false); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { viewport()->update(); });
}

ShadTreeView::~ShadTreeView()
{
    Q_D(ShadTreeView);
    // 私有数据先于子对象释放，先删除捕获私有数据的动画
    d->clearChevronAnimations();
}

void ShadTreeView::setItemHeight(int height)
{
    Q_D(ShadTreeView);
    if (height <= 0 || height == d->_delegate->getItemHeight())
    {
        return;
    }
    d->_delegate->setItemHeight(height);
    doItemsLayout();
}

int ShadTreeView::getItemHeight() const
{
    Q_D(const ShadTreeView);
    return d->_delegate->getItemHeight();
}

void ShadTreeView::setIndentGuidesVisible(bool visible)
{
    Q_D(ShadTreeView);
    if (d->_isIndentGuidesVisible == visible)
    {
        return;
    }
    d->_isIndentGuidesVisible = visible;
    viewport()->update();
}

bool ShadTreeView::isIndentGuidesVisible() const
{
    Q_D(const ShadTreeView);
    return d->_isIndentGuidesVisible;
}

void ShadTreeView::drawRow(QPainter* painter, const QStyleOptionViewItem& options, const QModelIndex& index) const
{
    Q_D(const ShadTreeView);
    using namespace ShadStyleHelper;
    const QModelIndex treeIndex = index.siblingAtColumn(treePosition());
    const QRectF backgroundRect(d->rowBackgroundRect(options.rect, treeIndex));
    const bool isEnabled = this->isEnabled() && treeIndex.flags().testFlag(Qt::ItemIsEnabled);
    const bool isSelected = selectionModel() != nullptr && selectionModel()->isSelected(treeIndex);
    const bool isHovered = isEnabled && d->_hoveredIndex.isValid() && d->_hoveredIndex == index.siblingAtColumn(0);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    if (!backgroundRect.isEmpty())
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        // hover:bg-accent，选中 bg-accent
        if (isSelected || isHovered)
        {
            fillRoundedRect(painter, backgroundRect, radius, shadTheme->getColor(ShadThemeType::Accent));
        }
        // 键盘导航时当前行显示 ring-2
        const bool isCurrent = currentIndex().isValid() && currentIndex().siblingAtColumn(treePosition()) == treeIndex;
        if (d->_isFocusVisible && hasFocus() && isCurrent)
        {
            strokeRoundedRect(painter, backgroundRect, radius, shadTheme->getColor(ShadThemeType::Ring), kFocusRingStroke);
        }
        painter->restore();
    }
    QTreeView::drawRow(painter, options, index);
}

void ShadTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
{
    Q_D(const ShadTreeView);
    const int indent = indentation();
    if (indent <= 0)
    {
        return;
    }
    const bool isRightToLeft = this->isRightToLeft();
    const int depth = d->depthOf(index);
    const int chevronLevel = rootIsDecorated() ? depth : depth - 1;
    // 第 level 个缩进格的位置，RTL 时从右侧开始
    const auto cellRect = [&rect, indent, isRightToLeft](int level)
    {
        const int left = isRightToLeft ? rect.right() + 1 - (level + 1) * indent : rect.left() + level * indent;
        return QRectF(left, rect.top(), indent, rect.height());
    };
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 缩进线：每个祖先层级在其展开箭头下方画一条 1px 竖线
    if (d->_isIndentGuidesVisible)
    {
        const QColor guideColor = shadTheme->getColor(ShadThemeType::Border);
        for (int level = 0; level < chevronLevel; ++level)
        {
            const QRectF cell = cellRect(level);
            painter->fillRect(QRectF(qFloor(cell.center().x()), cell.top(), 1.0, cell.height()), guideColor);
        }
    }

    // 展开箭头：ChevronRight 旋转 90 度为展开，RTL 时为朝左的箭头向下旋转
    if (chevronLevel >= 0 && model() != nullptr && model()->hasChildren(index))
    {
        if (!index.flags().testFlag(Qt::ItemIsEnabled))
        {
            painter->setOpacity(ShadStyleHelper::kDisabledOpacity);
        }
        const QPointF center = cellRect(chevronLevel).center();
        const qreal angle = 90.0 * d->chevronProgress(index) * (isRightToLeft ? -1.0 : 1.0);
        const qreal half = ShadTreeViewPrivate::kChevronSize / 2.0;
        painter->translate(center);
        painter->rotate(angle);
        ShadIcon::paint(painter, QRectF(-half, -half, ShadTreeViewPrivate::kChevronSize, ShadTreeViewPrivate::kChevronSize), isRightToLeft ? ShadIconType::ChevronLeft : ShadIconType::ChevronRight, shadTheme->getColor(ShadThemeType::MutedForeground), ShadTreeViewPrivate::kChevronSize);
    }
    painter->restore();
}

bool ShadTreeView::viewportEvent(QEvent* event)
{
    Q_D(ShadTreeView);
    if (event->type() == QEvent::Leave)
    {
        d->setHoveredIndex(QModelIndex());
    }
    return QTreeView::viewportEvent(event);
}

void ShadTreeView::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadTreeView);
    d->setHoveredIndex(indexAt(event->position().toPoint()));
    QTreeView::mouseMoveEvent(event);
}

void ShadTreeView::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadTreeView);
    // 鼠标操作不显示焦点环
    d->_isFocusVisible = false;
    QTreeView::mousePressEvent(event);
}

void ShadTreeView::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadTreeView);
    // 键盘导航显示焦点环
    d->_isFocusVisible = true;
    QTreeView::keyPressEvent(event);
    viewport()->update();
}

void ShadTreeView::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadTreeView);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QTreeView::focusInEvent(event);
    viewport()->update();
}

void ShadTreeView::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadTreeView);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QTreeView::focusOutEvent(event);
    viewport()->update();
}
