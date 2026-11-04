#ifndef SHADBREADCRUMBPRIVATE_H
#define SHADBREADCRUMBPRIVATE_H

#include <QList>
#include <QPointer>
#include <QString>

#include "ShadIconType.h"
#include "ShadProperty.h"
#include "ShadStyleHelper.h"

class ShadBreadcrumb;
class ShadBreadcrumbItem;
class ShadDropdownMenu;

class ShadBreadcrumbPrivate
{
    SHAD_D_CREATE(ShadBreadcrumb)

    public:
        ShadBreadcrumbPrivate();
        ~ShadBreadcrumbPrivate();

        enum class EntryKind
        {
            Link,
            Page,
            Ellipsis,
            Dropdown,
        };

        // gap-2.5（行内与换行后的行间距相同），扣除相邻两项的焦点环边距
        static constexpr int kItemSpacing = 10 - 2 * ShadStyleHelper::kFocusRingWidth;

        // 调用方添加的一项；显示用的子组件由 rebuildItems 按折叠规则生成
        struct Entry
        {
            EntryKind kind{EntryKind::Link};
            QString text;
            QPointer<ShadDropdownMenu> menu;
        };

        int appendEntry(EntryKind kind, const QString& text, ShadDropdownMenu* menu);
        // 按条目与折叠规则重新生成子组件；旧组件延后删除，允许在其点击信号中修改面包屑
        void rebuildItems();
        // 子组件在宽度 width 下的排列，apply 为 false 时只计算；返回总高度
        int layoutItems(int width, bool apply) const;
        // 某项在可用宽度下的宽度：文字项受最大宽度与可用宽度限制，但不小于最小宽度
        int itemWidth(const ShadBreadcrumbItem* item, int availableWidth) const;

        QList<Entry> _entries;
        QList<ShadBreadcrumbItem*> _items;
        // 自动折叠时省略号弹出的菜单
        ShadDropdownMenu* _collapsedMenu{nullptr};
        ShadIconType::IconName _separatorIcon{ShadIconType::ChevronRight};
        int _maximumVisibleItems{0};
        int _maximumItemWidth{0};
};

#endif // SHADBREADCRUMBPRIVATE_H
