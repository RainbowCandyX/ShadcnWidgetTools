#include "ShadTreeDelegate.h"

#include <QAbstractItemModel>
#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTreeView>
#include <QVariant>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTreeView.h"

namespace
{
    // 第一列内容紧跟展开箭头，其余列 px-2
    constexpr int kFirstColumnPadding = 4;
    constexpr int kCellPaddingX = 8;
    constexpr int kIconSize = 16;
    constexpr int kCheckboxSize = 16;
    constexpr int kGap = 8;

    QIcon decorationIcon(const QModelIndex& index)
    {
        const QVariant decoration = index.data(Qt::DecorationRole);
        if (decoration.userType() == QMetaType::QPixmap)
        {
            return QIcon(decoration.value<QPixmap>());
        }
        return decoration.value<QIcon>();
    }

    // IconRole 中的 Lucide 图标，没有时为 None；文件夹展开时换成打开的文件夹
    ShadIconType::IconName lucideIcon(const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        const QVariant iconData = index.data(ShadTreeView::IconRole);
        if (!iconData.isValid())
        {
            return ShadIconType::None;
        }
        const ShadIconType::IconName icon = static_cast<ShadIconType::IconName>(iconData.toInt());
        const QTreeView* view = qobject_cast<const QTreeView*>(option.widget);
        if (icon == ShadIconType::Folder && view != nullptr && view->isExpanded(index.siblingAtColumn(0)))
        {
            return ShadIconType::FolderOpen;
        }
        return icon;
    }

    Qt::Alignment cellAlignment(const QModelIndex& index)
    {
        const QVariant value = index.data(Qt::TextAlignmentRole);
        Qt::Alignment alignment = value.isValid() ? Qt::Alignment(value.toInt()) : Qt::Alignment(Qt::AlignLeft);
        if ((alignment & Qt::AlignVertical_Mask) == 0)
        {
            alignment |= Qt::AlignVCenter;
        }
        return alignment;
    }
} // namespace

ShadTreeDelegate::ShadTreeDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

ShadTreeDelegate::~ShadTreeDelegate()
{
}

void ShadTreeDelegate::setItemHeight(int height)
{
    _itemHeight = qMax(1, height);
}

int ShadTreeDelegate::getItemHeight() const
{
    return _itemHeight;
}

void ShadTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    using namespace ShadStyleHelper;
    const QRect rect = option.rect;
    // option.rect 已是 RTL 下的实际位置，单元格内部元素按 LTR 计算后镜像
    const Qt::LayoutDirection direction = option.direction;
    const bool isSelected = option.state.testFlag(QStyle::State_Selected);
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    if (!index.flags().testFlag(Qt::ItemIsEnabled))
    {
        painter->setOpacity(kDisabledOpacity);
    }

    int left = rect.left() + startPadding(index);
    const int right = rect.right() + 1 - kCellPaddingX;
    if (hasCheckbox(index))
    {
        const Qt::CheckState state = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
        drawCheckbox(painter, QRectF(checkboxRect(option, index)), state);
        left += kCheckboxSize + kGap;
    }

    // 选中行为 accent-foreground，其余为模型 ForegroundRole 或前景色
    const QVariant foreground = index.data(Qt::ForegroundRole);
    QColor textColor = shadTheme->getColor(ShadThemeType::Foreground);
    if (isSelected)
    {
        textColor = shadTheme->getColor(ShadThemeType::AccentForeground);
    }
    else if (foreground.canConvert<QBrush>())
    {
        textColor = foreground.value<QBrush>().color();
    }

    const QRect iconRect = visualRect(direction, rect, QRect(left, rect.top() + (rect.height() - kIconSize) / 2, kIconSize, kIconSize));
    const ShadIconType::IconName icon = lucideIcon(option, index);
    if (icon != ShadIconType::None)
    {
        ShadIcon::paint(painter, QRectF(iconRect), icon, textColor, kIconSize);
        left += kIconSize + kGap;
    }
    else
    {
        const QIcon decoration = decorationIcon(index);
        if (!decoration.isNull())
        {
            decoration.paint(painter, iconRect);
            left += kIconSize + kGap;
        }
    }

    // 选中行 font-medium；模型 FontRole 中显式设置的属性优先
    QFont font = option.font;
    if (isSelected)
    {
        font.setWeight(QFont::Medium);
    }
    const QVariant fontData = index.data(Qt::FontRole);
    if (fontData.canConvert<QFont>())
    {
        font = fontData.value<QFont>().resolve(font);
    }
    const QRect textRect = visualRect(direction, rect, QRect(left, rect.top(), qMax(0, right - left), rect.height()));
    painter->setFont(font);
    painter->setPen(textColor);
    painter->drawText(textRect, visualAlignment(direction, cellAlignment(index)), elideText(font, index.data(Qt::DisplayRole).toString(), textRect.width()));
    painter->restore();
}

QSize ShadTreeDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QFont font = option.font;
    const QVariant fontData = index.data(Qt::FontRole);
    if (fontData.canConvert<QFont>())
    {
        font = fontData.value<QFont>().resolve(font);
    }
    // 选中时字重变为 medium，按 medium 计算宽度，避免选中后文字被省略
    font.setWeight(QFont::Medium);
    int width = startPadding(index) + kCellPaddingX + ShadStyleHelper::textWidth(font, index.data(Qt::DisplayRole).toString());
    if (hasCheckbox(index))
    {
        width += kCheckboxSize + kGap;
    }
    if (index.data(ShadTreeView::IconRole).isValid() || !decorationIcon(index).isNull())
    {
        width += kIconSize + kGap;
    }
    return QSize(width, _itemHeight);
}

bool ShadTreeDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (model == nullptr || !hasCheckbox(index) || !index.flags().testFlag(Qt::ItemIsEnabled))
    {
        return false;
    }
    switch (event->type())
    {
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            {
                const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() != Qt::LeftButton || !checkboxRect(option, index).contains(mouseEvent->position().toPoint()))
                {
                    return false;
                }
                // 双击复选框不展开节点，只在松开时切换一次
                if (event->type() == QEvent::MouseButtonDblClick)
                {
                    return true;
                }
                break;
            }
        case QEvent::KeyPress:
            {
                const int key = static_cast<QKeyEvent*>(event)->key();
                if (key != Qt::Key_Space && key != Qt::Key_Select)
                {
                    return false;
                }
                break;
            }
        default:
            return false;
    }
    const Qt::CheckState state = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
    Qt::CheckState nextState = state == Qt::Checked ? Qt::Unchecked : Qt::Checked;
    // 用户三态：未选中、半选、选中循环
    if (index.flags().testFlag(Qt::ItemIsUserTristate))
    {
        nextState = static_cast<Qt::CheckState>((static_cast<int>(state) + 1) % 3);
    }
    if (!model->setData(index, static_cast<int>(nextState), Qt::CheckStateRole))
    {
        return false;
    }
    // 自动三态的父节点：勾选或取消时同步全部子节点；之后按子节点重新计算各级自动三态的祖先
    if (index.flags().testFlag(Qt::ItemIsAutoTristate) && nextState != Qt::PartiallyChecked)
    {
        setDescendantsCheckState(model, index, nextState);
    }
    updateAncestorsCheckState(model, index);
    return true;
}

void ShadTreeDelegate::setDescendantsCheckState(QAbstractItemModel* model, const QModelIndex& index, Qt::CheckState state)
{
    // 子节点的复选框与 index 位于同一列
    const QModelIndex parent = index.siblingAtColumn(0);
    for (int row = 0; row < model->rowCount(parent); ++row)
    {
        const QModelIndex child = model->index(row, index.column(), parent);
        if (hasCheckbox(child) && child.flags().testFlag(Qt::ItemIsEnabled))
        {
            model->setData(child, static_cast<int>(state), Qt::CheckStateRole);
        }
        setDescendantsCheckState(model, child, state);
    }
}

void ShadTreeDelegate::updateAncestorsCheckState(QAbstractItemModel* model, const QModelIndex& index)
{
    const int column = index.column();
    for (QModelIndex parent = index.parent(); parent.isValid(); parent = parent.parent())
    {
        const QModelIndex checkIndex = parent.siblingAtColumn(column);
        if (!hasCheckbox(checkIndex) || !checkIndex.flags().testFlag(Qt::ItemIsAutoTristate))
        {
            break;
        }
        // 子节点全部勾选为选中，全部未勾选为未选中，其余为半选
        int checkableCount = 0;
        int checkedCount = 0;
        int uncheckedCount = 0;
        for (int row = 0; row < model->rowCount(parent); ++row)
        {
            const QModelIndex child = model->index(row, column, parent);
            if (!hasCheckbox(child))
            {
                continue;
            }
            ++checkableCount;
            const int childState = child.data(Qt::CheckStateRole).toInt();
            if (childState == Qt::Checked)
            {
                ++checkedCount;
            }
            else if (childState == Qt::Unchecked)
            {
                ++uncheckedCount;
            }
        }
        if (checkableCount == 0)
        {
            break;
        }
        Qt::CheckState parentState = Qt::PartiallyChecked;
        if (checkedCount == checkableCount)
        {
            parentState = Qt::Checked;
        }
        else if (uncheckedCount == checkableCount)
        {
            parentState = Qt::Unchecked;
        }
        model->setData(checkIndex, static_cast<int>(parentState), Qt::CheckStateRole);
    }
}

bool ShadTreeDelegate::hasCheckbox(const QModelIndex& index)
{
    return index.flags().testFlag(Qt::ItemIsUserCheckable) && index.data(Qt::CheckStateRole).isValid();
}

QRect ShadTreeDelegate::checkboxRect(const QStyleOptionViewItem& option, const QModelIndex& index)
{
    const QRect rect = option.rect;
    const QRect logicalRect(rect.left() + startPadding(index), rect.top() + (rect.height() - kCheckboxSize) / 2, kCheckboxSize, kCheckboxSize);
    return ShadStyleHelper::visualRect(option.direction, rect, logicalRect);
}

int ShadTreeDelegate::startPadding(const QModelIndex& index)
{
    return index.column() == 0 ? kFirstColumnPadding : kCellPaddingX;
}
