#ifndef SHADBUTTONGROUPPRIVATE_H
#define SHADBUTTONGROUPPRIVATE_H

#include <QList>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>
#include <QtCore/qnamespace.h>

#include "ShadIconType.h"
#include "ShadProperty.h"

class QWidget;
class ShadButtonGroup;
class ShadButtonGroupText;

class ShadButtonGroupTextPrivate
{
    SHAD_D_CREATE(ShadButtonGroupText)

    public:
        ShadButtonGroupTextPrivate();
        ~ShadButtonGroupTextPrivate();

        QString _text;
        ShadIconType::IconName _icon{ShadIconType::None};
};

class ShadButtonGroupPrivate
{
    SHAD_D_CREATE(ShadButtonGroup)

    public:
        ShadButtonGroupPrivate();
        ~ShadButtonGroupPrivate();

        // 嵌套模式下各项可视间距 gap-2
        static constexpr int kNestedGap = 8;

        // 一项：组件或分隔线（widget 为空）
        struct Item
        {
            QPointer<QWidget> widget;
            bool isSeparator{false};
        };

        // 参与排列的项：分隔线，以及仍然存在且未隐藏的组件
        QList<int> visibleItems() const;
        bool isNested() const;
        // 主方向上的可视长度（不含焦点环边距），useMinimum 为 true 时输入框取最小宽度
        int itemLength(const Item& item, bool useMinimum) const;
        // 交叉方向上的可视长度：组内最高（纵向时最宽）的组件
        int crossLength() const;
        // 内容尺寸（含四周焦点环边距）
        QSize contentSize(bool useMinimum) const;
        // 按当前尺寸放置子组件与分隔线，并写入拼接信息
        void relayout();
        void insertItem(int index, QWidget* widget, bool isSeparator);
        // 清除已被外部删除的组件
        void pruneItems();

        QList<Item> _items;
        Qt::Orientation _orientation{Qt::Horizontal};
        // 分隔线在组件坐标系中的位置，由 relayout 计算，paintEvent 绘制
        QList<QRect> _separatorRects;
};

#endif // SHADBUTTONGROUPPRIVATE_H
