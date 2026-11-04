#include "ShadInputGroupPrivate.h"

#include <QRect>
#include <QWidget>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadInputGroup.h"
#include "ShadStyleHelper.h"

ShadInputGroupPrivate::ShadInputGroupPrivate()
{
}

ShadInputGroupPrivate::~ShadInputGroupPrivate()
{
}

QWidget* ShadInputGroupPrivate::control() const
{
    return _control.data();
}

bool ShadInputGroupPrivate::isLineEdit() const
{
    return _controlType == ShadInputGroupType::LineEdit;
}

QList<ShadInputGroupPrivate::Addon> ShadInputGroupPrivate::addons(ShadInputGroupType::AddonPosition position) const
{
    QList<Addon> result;
    for (const Addon& addon : _addons)
    {
        if (addon.position != position)
        {
            continue;
        }
        // 弹性空白只在上下附加区生效；已删除或隐藏的组件不参与排列
        if (addon.isStretch || (!addon.widget.isNull() && !addon.widget->isHidden()))
        {
            result.append(addon);
        }
    }
    return result;
}

bool ShadInputGroupPrivate::hasAddons(ShadInputGroupType::AddonPosition position) const
{
    for (const Addon& addon : addons(position))
    {
        if (!addon.isStretch)
        {
            return true;
        }
    }
    return false;
}

bool ShadInputGroupPrivate::hasFocusMargin(const QWidget* widget)
{
    // 本库的按钮四周预留焦点环边距，按可视尺寸排列
    return qobject_cast<const ShadButton*>(widget) != nullptr;
}

QSize ShadInputGroupPrivate::itemSize(const QWidget* widget)
{
    if (widget == nullptr)
    {
        return QSize();
    }
    const QSize hint = widget->sizeHint().expandedTo(widget->minimumSize()).boundedTo(widget->maximumSize());
    if (!hasFocusMargin(widget))
    {
        return hint;
    }
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    return QSize(qMax(0, hint.width() - margin), qMax(0, hint.height() - margin));
}

int ShadInputGroupPrivate::rowHeight(ShadInputGroupType::AddonPosition position) const
{
    int height = 0;
    for (const Addon& addon : addons(position))
    {
        height = qMax(height, itemSize(addon.widget.data()).height());
    }
    return height;
}

int ShadInputGroupPrivate::rowWidth(ShadInputGroupType::AddonPosition position) const
{
    int width = 0;
    int count = 0;
    for (const Addon& addon : addons(position))
    {
        if (addon.isStretch)
        {
            continue;
        }
        width += itemSize(addon.widget.data()).width();
        ++count;
    }
    return width + qMax(0, count - 1) * kAddonGap;
}

int ShadInputGroupPrivate::controlRowHeight() const
{
    const QWidget* widget = control();
    if (widget == nullptr)
    {
        return kInlineHeight;
    }
    if (!isLineEdit())
    {
        return widget->sizeHint().height();
    }
    const bool hasBlock = hasAddons(ShadInputGroupType::BlockStart) || hasAddons(ShadInputGroupType::BlockEnd);
    return hasBlock ? kBlockInputHeight : kInlineHeight;
}

int ShadInputGroupPrivate::blockPadding() const
{
    return isLineEdit() ? kBlockPaddingLine : kBlockPaddingText;
}

int ShadInputGroupPrivate::innerHeight() const
{
    int height = controlRowHeight();
    if (hasAddons(ShadInputGroupType::BlockStart))
    {
        height += blockPadding() + rowHeight(ShadInputGroupType::BlockStart);
    }
    if (hasAddons(ShadInputGroupType::BlockEnd))
    {
        height += rowHeight(ShadInputGroupType::BlockEnd) + blockPadding();
    }
    return height;
}

void ShadInputGroupPrivate::relayout()
{
    Q_Q(ShadInputGroup);
    pruneAddons();
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const QRect bounds = q->rect();
    const QRect inner = bounds.adjusted(margin + 1, margin + 1, -(margin + 1), -(margin + 1));
    const Qt::LayoutDirection direction = q->layoutDirection();
    // 按 LTR 的可视区域计算，可聚焦组件外扩焦点环边距，RTL 时在组件内镜像
    const auto place = [&](QWidget* widget, const QRect& logicalBody)
    {
        const QRect logicalRect = hasFocusMargin(widget) ? logicalBody.adjusted(-margin, -margin, margin, margin) : logicalBody;
        widget->setGeometry(ShadStyleHelper::visualRect(direction, bounds, logicalRect));
    };
    // 上下附加区：px-3 起始侧对齐，弹性空白平分剩余宽度
    const auto layoutBlockRow = [&](ShadInputGroupType::AddonPosition position, int top)
    {
        const QList<Addon> items = addons(position);
        const int rowHeightValue = rowHeight(position);
        int stretchCount = 0;
        for (const Addon& addon : items)
        {
            stretchCount += addon.isStretch ? 1 : 0;
        }
        const int available = inner.width() - 2 * kAddonPadding;
        const int stretchSpace = stretchCount > 0 ? qMax(0, available - rowWidth(position)) / stretchCount : 0;
        int x = inner.left() + kAddonPadding;
        bool isFirstItem = true;
        for (const Addon& addon : items)
        {
            if (addon.isStretch)
            {
                x += stretchSpace;
                continue;
            }
            if (!isFirstItem)
            {
                x += kAddonGap;
            }
            const QSize size = itemSize(addon.widget.data());
            place(addon.widget.data(), QRect(x, top + (rowHeightValue - size.height()) / 2, size.width(), size.height()));
            x += size.width();
            isFirstItem = false;
        }
    };
    const auto rowHasButton = [&](ShadInputGroupType::AddonPosition position)
    {
        for (const Addon& addon : addons(position))
        {
            if (qobject_cast<const ShadButton*>(addon.widget.data()) != nullptr)
            {
                return true;
            }
        }
        return false;
    };

    int top = inner.top();
    if (hasAddons(ShadInputGroupType::BlockStart))
    {
        top += blockPadding();
        layoutBlockRow(ShadInputGroupType::BlockStart, top);
        top += rowHeight(ShadInputGroupType::BlockStart);
    }

    // 行内附加区与输入框：附加区 pl-3 / pr-3（含按钮时向外收），输入框靠近附加区一侧为 pl-2 / pr-2
    const int controlHeight = controlRowHeight();
    const int inputPadding = isLineEdit() ? kAddonPadding : 0;
    int controlLeft = inner.left() + inputPadding;
    int controlRight = inner.right() + 1 - inputPadding;
    const QList<Addon> startItems = addons(ShadInputGroupType::InlineStart);
    if (hasAddons(ShadInputGroupType::InlineStart))
    {
        int x = inner.left() + kAddonPadding - (rowHasButton(ShadInputGroupType::InlineStart) ? kButtonPullIn : 0);
        for (const Addon& addon : startItems)
        {
            if (addon.isStretch)
            {
                continue;
            }
            const QSize size = itemSize(addon.widget.data());
            place(addon.widget.data(), QRect(x, top + (controlHeight - size.height()) / 2, size.width(), size.height()));
            x += size.width() + kAddonGap;
        }
        controlLeft = x - kAddonGap + kInputPaddingNearAddon;
    }
    const QList<Addon> endItems = addons(ShadInputGroupType::InlineEnd);
    if (hasAddons(ShadInputGroupType::InlineEnd))
    {
        int x = inner.right() + 1 - kAddonPadding + (rowHasButton(ShadInputGroupType::InlineEnd) ? kButtonPullIn : 0);
        for (int index = int(endItems.size()) - 1; index >= 0; --index)
        {
            QWidget* widget = endItems.at(index).widget.data();
            if (endItems.at(index).isStretch || widget == nullptr)
            {
                continue;
            }
            const QSize size = itemSize(widget);
            x -= size.width();
            place(widget, QRect(x, top + (controlHeight - size.height()) / 2, size.width(), size.height()));
            x -= kAddonGap;
        }
        controlRight = x + kAddonGap - kInputPaddingNearAddon;
    }
    if (QWidget* widget = control())
    {
        place(widget, QRect(controlLeft, top, qMax(0, controlRight - controlLeft), controlHeight));
    }
    top += controlHeight;

    if (hasAddons(ShadInputGroupType::BlockEnd))
    {
        layoutBlockRow(ShadInputGroupType::BlockEnd, top);
    }
    q->update();
}

void ShadInputGroupPrivate::addAddon(ShadInputGroupType::AddonPosition position, QWidget* widget)
{
    Q_Q(ShadInputGroup);
    Addon addon;
    addon.position = position;
    addon.widget = widget;
    addon.isStretch = widget == nullptr;
    if (widget != nullptr)
    {
        if (widget->parentWidget() != q)
        {
            widget->setParent(q);
        }
        widget->installEventFilter(q);
        widget->show();
    }
    _addons.append(addon);
    q->updateGeometry();
    relayout();
}

void ShadInputGroupPrivate::pruneAddons()
{
    // 去掉已被外部删除的组件，弹性空白保留
    _addons.removeIf([](const Addon& addon) { return !addon.isStretch && addon.widget.isNull(); });
}
