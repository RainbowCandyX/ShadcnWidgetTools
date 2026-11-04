#include "ShadItemGroup.h"

#include <QBoxLayout>
#include <QChildEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QObject>
#include <QSizePolicy>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadItem.h"
#include "ShadItemGroupPrivate.h"
#include "ShadMaxWidthBoxLayout.h"
#include "ShadSeparator.h"
#include "ShadStyleHelper.h"

ShadItemGroup::ShadItemGroup(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadItemGroupPrivate())
{
    Q_D(ShadItemGroup);
    d->q_ptr = this;
    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    // 设置最大宽度后，列表项的描述按实际宽度计算行数
    d->_layout = new ShadMaxWidthBoxLayout(QBoxLayout::TopToBottom, this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->_layout->setSpacing(0);
}

ShadItemGroup::~ShadItemGroup()
{
}

void ShadItemGroup::addItem(ShadItem* item)
{
    Q_D(ShadItemGroup);
    insertItem(d->_entries.size(), item);
}

void ShadItemGroup::insertItem(int index, ShadItem* item)
{
    Q_D(ShadItemGroup);
    if (item == nullptr || d->_entries.contains(item))
    {
        return;
    }
    const int position = (index < 0 || index > d->_entries.size()) ? d->_entries.size() : index;
    item->setParent(this);
    d->_entries.insert(position, item);
    updateLayout();
    item->show();
}

void ShadItemGroup::removeItem(ShadItem* item)
{
    Q_D(ShadItemGroup);
    if (item == nullptr || !d->_entries.contains(item))
    {
        return;
    }
    d->_entries.removeAll(item);
    d->_layout->removeWidget(item);
    ShadStyleHelper::clearFocusWithin(item);
    item->hide();
    item->setParent(nullptr);
    updateLayout();
}

void ShadItemGroup::addSeparator()
{
    Q_D(ShadItemGroup);
    // 左右扣除列表项的焦点环边距，与列表项的可视边缘对齐
    QWidget* separator = new QWidget(this);
    QHBoxLayout* separatorLayout = new QHBoxLayout(separator);
    const int ring = ShadStyleHelper::kFocusRingWidth;
    separatorLayout->setContentsMargins(ring, 0, ring, 0);
    separatorLayout->addWidget(new ShadSeparator(Qt::Horizontal, separator));
    d->_entries.append(separator);
    d->_separators.append(separator);
    updateLayout();
    separator->show();
}

QList<ShadItem*> ShadItemGroup::items() const
{
    Q_D(const ShadItemGroup);
    QList<ShadItem*> result;
    for (QWidget* entry : d->_entries)
    {
        if (ShadItem* item = qobject_cast<ShadItem*>(entry))
        {
            result.append(item);
        }
    }
    return result;
}

void ShadItemGroup::childEvent(QChildEvent* event)
{
    Q_D(ShadItemGroup);
    QWidget::childEvent(event);
    if (event->type() != QEvent::ChildRemoved)
    {
        return;
    }
    // 列表项或分隔线被删除或改了父组件时移出列表；此时子对象可能正在析构，只比较指针
    QObject* child = event->child();
    const auto isChild = [child](QWidget* entry) { return static_cast<QObject*>(entry) == child; };
    const bool isRemoved = d->_entries.removeIf(isChild) > 0;
    d->_separators.removeIf(isChild);
    if (isRemoved)
    {
        updateLayout();
    }
}

void ShadItemGroup::updateLayout()
{
    Q_D(ShadItemGroup);
    // 删除布局项不会删除其中的组件
    while (QLayoutItem* layoutItem = d->_layout->takeAt(0))
    {
        delete layoutItem;
    }
    const int gap = ShadItemGroupPrivate::gap(d->smallestSize());
    // 列表项四周有焦点环边距，相邻处扣除；分隔线上下各加 my-2
    const auto edgeOffset = [d](QWidget* entry) { return d->_separators.contains(entry) ? ShadItemGroupPrivate::kSeparatorMargin : -ShadStyleHelper::kFocusRingWidth; };
    QWidget* previous = nullptr;
    for (QWidget* entry : d->_entries)
    {
        if (previous != nullptr)
        {
            d->_layout->addSpacing(qMax(0, gap + edgeOffset(previous) + edgeOffset(entry)));
        }
        d->_layout->addWidget(entry);
        previous = entry;
    }
}
