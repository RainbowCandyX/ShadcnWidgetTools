#ifndef SHADITEMGROUP_H
#define SHADITEMGROUP_H

#include <QList>
#include <QWidget>

#include "ShadProperty.h"

class QChildEvent;
class ShadItem;

// 列表项分组，对应 shadcn ItemGroup / ItemSeparator：列表项纵向排列，宽度随父布局拉伸
// 间距 gap-4；组内有 Small 尺寸的列表项时为 gap-2.5，有 ExtraSmall 时为 gap-2；列表项改变尺寸后自动更新
// 分隔线为 1px 横线，上下各 my-2，左右与列表项的可视边缘对齐
class ShadItemGroupPrivate;

class SHADCN_EXPORT ShadItemGroup : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadItemGroup)

    public:
        explicit ShadItemGroup(QWidget* parent = nullptr);
        ~ShadItemGroup() override;

        // 添加到末尾，分组接管列表项的父子关系
        void addItem(ShadItem* item);
        // index 超出范围时添加到末尾（分隔线也计入序号）
        void insertItem(int index, ShadItem* item);
        // 从分组中移除但不删除，父组件置空，由调用方负责释放
        void removeItem(ShadItem* item);
        // 在末尾添加分隔线
        void addSeparator();
        // 分组中的列表项，不含分隔线
        QList<ShadItem*> items() const;

    protected:
        void childEvent(QChildEvent* event) override;

    private:
        friend class ShadItem;
        // 按列表项与分隔线的顺序重建布局并计算间距
        void updateLayout();
};

#endif // SHADITEMGROUP_H
