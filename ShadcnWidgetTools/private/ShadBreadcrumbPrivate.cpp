#include "ShadBreadcrumbPrivate.h"

#include <QAction>
#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QRect>
#include <QSize>
#include <QtGlobal>

#include <limits>
#include <utility>

#include "ShadBreadcrumb.h"
#include "ShadBreadcrumbItem.h"
#include "ShadDropdownMenu.h"
#include "ShadStyleHelper.h"

ShadBreadcrumbPrivate::ShadBreadcrumbPrivate()
{
}

ShadBreadcrumbPrivate::~ShadBreadcrumbPrivate()
{
}

int ShadBreadcrumbPrivate::appendEntry(EntryKind kind, const QString& text, ShadDropdownMenu* menu)
{
    Entry entry;
    entry.kind = kind;
    entry.text = text;
    entry.menu = menu;
    _entries.append(entry);
    rebuildItems();
    return int(_entries.size()) - 1;
}

void ShadBreadcrumbPrivate::rebuildItems()
{
    Q_Q(ShadBreadcrumb);
    for (ShadBreadcrumbItem* item : std::as_const(_items))
    {
        ShadStyleHelper::clearFocusWithin(item);
        item->hide();
        item->deleteLater();
    }
    _items.clear();

    // 需要显示的条目序号，-1 表示自动折叠生成的省略号
    const int count = int(_entries.size());
    QList<int> displayed;
    QList<int> collapsed;
    const int limit = _maximumVisibleItems > 0 ? qMax(2, _maximumVisibleItems) : 0;
    if (limit > 0 && count > limit)
    {
        const int tailStart = count - (limit - 1);
        displayed.append(0);
        displayed.append(-1);
        for (int index = 1; index < tailStart; ++index)
        {
            collapsed.append(index);
        }
        for (int index = tailStart; index < count; ++index)
        {
            displayed.append(index);
        }
    }
    else
    {
        for (int index = 0; index < count; ++index)
        {
            displayed.append(index);
        }
    }

    if (!collapsed.isEmpty())
    {
        if (_collapsedMenu == nullptr)
        {
            _collapsedMenu = new ShadDropdownMenu(q);
        }
        _collapsedMenu->clear();
        for (const int index : std::as_const(collapsed))
        {
            const Entry& entry = _entries.at(index);
            if (entry.kind == EntryKind::Ellipsis || entry.kind == EntryKind::Page)
            {
                continue;
            }
            QAction* action = _collapsedMenu->addItem(entry.text);
            // 排队发出：信号处理中修改面包屑会清空菜单并删除正在触发的 QAction
            QObject::connect(action, &QAction::triggered, q, [q, index]() { Q_EMIT q->linkClicked(index); }, Qt::QueuedConnection);
        }
    }

    using Kind = ShadBreadcrumbItem::Kind;
    for (int position = 0; position < displayed.size(); ++position)
    {
        if (position > 0)
        {
            ShadBreadcrumbItem* separator = new ShadBreadcrumbItem(Kind::Separator, q);
            separator->setShadIcon(_separatorIcon);
            _items.append(separator);
        }
        const int index = displayed.at(position);
        ShadBreadcrumbItem* item = nullptr;
        if (index < 0)
        {
            item = new ShadBreadcrumbItem(Kind::EllipsisButton, q);
            item->setMenu(_collapsedMenu);
            const QString moreText = QCoreApplication::translate("ShadBreadcrumb", "显示更多");
            item->setToolTip(moreText);
            item->setAccessibleName(moreText);
        }
        else
        {
            const Entry& entry = _entries.at(index);
            switch (entry.kind)
            {
                case EntryKind::Link:
                    {
                        item = new ShadBreadcrumbItem(Kind::Link, q);
                        QObject::connect(item, &ShadBreadcrumbItem::clicked, q, [q, index]() { Q_EMIT q->linkClicked(index); });
                        break;
                    }
                case EntryKind::Page:
                    {
                        item = new ShadBreadcrumbItem(Kind::Page, q);
                        break;
                    }
                case EntryKind::Ellipsis:
                    {
                        item = new ShadBreadcrumbItem(entry.menu.isNull() ? Kind::Ellipsis : Kind::EllipsisButton, q);
                        item->setMenu(entry.menu.data());
                        break;
                    }
                case EntryKind::Dropdown:
                    {
                        item = new ShadBreadcrumbItem(Kind::Dropdown, q);
                        item->setMenu(entry.menu.data());
                        QObject::connect(item, &ShadBreadcrumbItem::clicked, q, [q, index]() { Q_EMIT q->linkClicked(index); });
                        break;
                    }
            }
            item->setText(entry.text);
        }
        _items.append(item);
    }

    for (ShadBreadcrumbItem* item : std::as_const(_items))
    {
        item->show();
    }
    q->updateGeometry();
    layoutItems(q->width(), true);
}

int ShadBreadcrumbPrivate::itemWidth(const ShadBreadcrumbItem* item, int availableWidth) const
{
    const int hintWidth = item->sizeHint().width();
    if (!item->hasText())
    {
        return hintWidth;
    }
    int width = hintWidth;
    if (_maximumItemWidth > 0)
    {
        width = qMin(width, _maximumItemWidth + 2 * ShadStyleHelper::kFocusRingWidth);
    }
    width = qMin(width, availableWidth);
    return qMax(width, item->minimumSizeHint().width());
}

int ShadBreadcrumbPrivate::layoutItems(int width, bool apply) const
{
    Q_Q(const ShadBreadcrumb);
    const int availableWidth = width > 0 ? width : std::numeric_limits<int>::max() / 2;

    // 先分行：放不下时换行，每行至少一项
    struct Placement
    {
        ShadBreadcrumbItem* item;
        int x;
        int width;
        int height;
        int row;
    };
    QList<Placement> placements;
    QList<int> rowHeights;
    int x = 0;
    for (ShadBreadcrumbItem* item : _items)
    {
        const int itemSizeWidth = itemWidth(item, availableWidth);
        const int itemHeight = item->sizeHint().height();
        if (rowHeights.isEmpty() || (x > 0 && x + itemSizeWidth > availableWidth))
        {
            rowHeights.append(0);
            x = 0;
        }
        placements.append(Placement{item, x, itemSizeWidth, itemHeight, int(rowHeights.size()) - 1});
        rowHeights.last() = qMax(rowHeights.last(), itemHeight);
        x += itemSizeWidth + kItemSpacing;
    }
    QList<int> rowTops;
    int totalHeight = 0;
    for (int row = 0; row < rowHeights.size(); ++row)
    {
        if (row > 0)
        {
            totalHeight += kItemSpacing;
        }
        rowTops.append(totalHeight);
        totalHeight += rowHeights.at(row);
    }

    if (apply)
    {
        // 按 LTR 计算后在组件内镜像，RTL 时从右向左排列；同一行内垂直居中，组件比内容高时整体垂直居中
        const QRect bounds(0, 0, qMax(0, width), totalHeight);
        const int offsetY = qMax(0, (q->height() - totalHeight) / 2);
        for (const Placement& placement : placements)
        {
            const QRect logicalRect(placement.x, offsetY + rowTops.at(placement.row) + (rowHeights.at(placement.row) - placement.height) / 2, placement.width, placement.height);
            placement.item->setGeometry(ShadStyleHelper::visualRect(q->layoutDirection(), bounds, logicalRect));
        }
    }
    return totalHeight;
}
