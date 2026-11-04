#include "ShadToggleGroup.h"

#include <QAbstractButton>
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadToggle.h"
#include "ShadToggleGroupPrivate.h"
#include "ShadTogglePrivate.h"

ShadToggleGroup::ShadToggleGroup(QWidget* parent)
    : ShadToggleGroup(ShadToggleGroupType::Single, parent)
{
}

ShadToggleGroup::ShadToggleGroup(ShadToggleGroupType::SelectionMode selectionMode, QWidget* parent)
    : QWidget(parent), d_ptr(new ShadToggleGroupPrivate())
{
    Q_D(ShadToggleGroup);
    d->q_ptr = this;
    d->_selectionMode = selectionMode;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadToggleGroup::~ShadToggleGroup()
{
}

ShadToggle* ShadToggleGroup::addItem(const QString& text)
{
    return addItem(ShadIconType::None, text);
}

ShadToggle* ShadToggleGroup::addItem(ShadIconType::IconName icon, const QString& text)
{
    Q_D(ShadToggleGroup);
    ShadToggle* item = new ShadToggle(text, this);
    item->setShadIcon(icon);
    item->setVariant(d->_variant);
    item->setToggleSize(d->_size);
    connect(item, &QAbstractButton::toggled, this, [d, item](bool checked) { d->onItemToggled(item, checked); });
    d->_items.append(item);
    // 组件已显示后新增的子组件不会自动显示
    item->show();
    layoutItems();
    updateGeometry();
    return item;
}

ShadToggle* ShadToggleGroup::itemAt(int index) const
{
    Q_D(const ShadToggleGroup);
    if (index < 0 || index >= d->_items.size())
    {
        return nullptr;
    }
    return d->_items.at(index);
}

int ShadToggleGroup::count() const
{
    Q_D(const ShadToggleGroup);
    return d->_items.size();
}

void ShadToggleGroup::setSelectionMode(ShadToggleGroupType::SelectionMode selectionMode)
{
    Q_D(ShadToggleGroup);
    if (d->_selectionMode == selectionMode)
    {
        return;
    }
    d->_selectionMode = selectionMode;
    if (selectionMode == ShadToggleGroupType::Single)
    {
        // 只保留第一个选中项
        const int firstChecked = getCurrentIndex();
        QList<bool> states;
        for (int i = 0; i < d->_items.size(); ++i)
        {
            states.append(i == firstChecked);
        }
        d->applyCheckedStates(states);
    }
    d->_lastCurrentIndex = getCurrentIndex();
}

ShadToggleGroupType::SelectionMode ShadToggleGroup::getSelectionMode() const
{
    Q_D(const ShadToggleGroup);
    return d->_selectionMode;
}

void ShadToggleGroup::setVariant(ShadToggleType::Variant variant)
{
    Q_D(ShadToggleGroup);
    if (d->_variant == variant)
    {
        return;
    }
    d->_variant = variant;
    for (ShadToggle* item : d->_items)
    {
        item->setVariant(variant);
    }
    layoutItems();
    updateGeometry();
    update();
}

ShadToggleType::Variant ShadToggleGroup::getVariant() const
{
    Q_D(const ShadToggleGroup);
    return d->_variant;
}

void ShadToggleGroup::setToggleSize(ShadToggleType::Size size)
{
    Q_D(ShadToggleGroup);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    for (ShadToggle* item : d->_items)
    {
        item->setToggleSize(size);
    }
    layoutItems();
    updateGeometry();
    update();
}

ShadToggleType::Size ShadToggleGroup::getToggleSize() const
{
    Q_D(const ShadToggleGroup);
    return d->_size;
}

void ShadToggleGroup::setCurrentIndex(int index)
{
    Q_D(ShadToggleGroup);
    QList<bool> states;
    for (int i = 0; i < d->_items.size(); ++i)
    {
        states.append(i == index);
    }
    d->applyCheckedStates(states);
}

int ShadToggleGroup::getCurrentIndex() const
{
    Q_D(const ShadToggleGroup);
    for (int i = 0; i < d->_items.size(); ++i)
    {
        if (d->_items.at(i)->isChecked())
        {
            return i;
        }
    }
    return -1;
}

void ShadToggleGroup::setItemChecked(int index, bool checked)
{
    Q_D(ShadToggleGroup);
    if (index < 0 || index >= d->_items.size())
    {
        qWarning("ShadToggleGroup::setItemChecked 索引越界: %d，当前按钮数: %d", index, static_cast<int>(d->_items.size()));
        return;
    }
    QList<bool> states;
    for (int i = 0; i < d->_items.size(); ++i)
    {
        const bool isTarget = i == index;
        if (d->_selectionMode == ShadToggleGroupType::Single && checked)
        {
            states.append(isTarget);
        }
        else
        {
            states.append(isTarget ? checked : d->_items.at(i)->isChecked());
        }
    }
    d->applyCheckedStates(states);
}

bool ShadToggleGroup::isItemChecked(int index) const
{
    Q_D(const ShadToggleGroup);
    if (index < 0 || index >= d->_items.size())
    {
        return false;
    }
    return d->_items.at(index)->isChecked();
}

QList<int> ShadToggleGroup::getCheckedIndexes() const
{
    Q_D(const ShadToggleGroup);
    QList<int> result;
    for (int i = 0; i < d->_items.size(); ++i)
    {
        if (d->_items.at(i)->isChecked())
        {
            result.append(i);
        }
    }
    return result;
}

QSize ShadToggleGroup::sizeHint() const
{
    Q_D(const ShadToggleGroup);
    const QList<ShadToggle*> items = d->visibleItems();
    if (items.isEmpty())
    {
        return QSize(0, 0);
    }
    int width = 0;
    int height = 0;
    for (ShadToggle* item : items)
    {
        const QSize itemSize = item->sizeHint();
        width += itemSize.width();
        height = qMax(height, itemSize.height());
    }
    width -= d->itemOverlap() * (static_cast<int>(items.size()) - 1);
    return QSize(width, height);
}

QSize ShadToggleGroup::minimumSizeHint() const
{
    return sizeHint();
}

bool ShadToggleGroup::event(QEvent* event)
{
    // 子按钮尺寸或显隐变化时会向父组件投递 LayoutRequest；布局方向变化时按钮顺序需要镜像
    if (event->type() == QEvent::LayoutRequest || event->type() == QEvent::LayoutDirectionChange)
    {
        layoutItems();
        updateGeometry();
    }
    return QWidget::event(event);
}

void ShadToggleGroup::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutItems();
}

void ShadToggleGroup::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadToggleGroup);
    const QList<ShadToggle*> items = d->visibleItems();
    // outline 变体的阴影画在整组外侧（data-[variant=outline]:shadow-xs），单个按钮时由按钮自己绘制
    if (d->_variant != ShadToggleType::Outline || items.size() < 2)
    {
        return;
    }
    const int margin = ShadStyleHelper::kFocusRingWidth;
    // RTL 时第一个按钮在右侧，取首尾按钮的并集
    const QRect groupRect = items.first()->geometry().united(items.last()->geometry());
    const QRectF bodyRect = QRectF(groupRect).adjusted(margin, margin, -margin, -margin);
    QPainter painter(this);
    painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
    ShadStyleHelper::drawShadowXs(&painter, bodyRect, shadTheme->getRadius(ShadThemeType::Medium));
}

void ShadToggleGroup::layoutItems()
{
    Q_D(ShadToggleGroup);
    using GroupPosition = ShadTogglePrivate::GroupPosition;
    const QList<ShadToggle*> items = d->visibleItems();
    const int overlap = d->itemOverlap();
    int height = 0;
    for (ShadToggle* item : items)
    {
        height = qMax(height, item->sizeHint().height());
    }
    int x = 0;
    for (int i = 0; i < items.size(); ++i)
    {
        ShadToggle* item = items.at(i);
        GroupPosition position = GroupPosition::Middle;
        if (items.size() == 1)
        {
            position = GroupPosition::Standalone;
        }
        else if (i == 0)
        {
            position = GroupPosition::First;
        }
        else if (i == items.size() - 1)
        {
            position = GroupPosition::Last;
        }
        ShadTogglePrivate* itemData = item->d_func();
        if (itemData->_groupPosition != position)
        {
            itemData->_groupPosition = position;
            item->update();
        }
        const int width = item->sizeHint().width();
        // 按 LTR 从左到右排列，RTL 时在组内镜像，第一个按钮位于右侧
        item->setGeometry(ShadStyleHelper::visualRect(layoutDirection(), rect(), QRect(x, 0, width, height)));
        // 尺寸未变化时不会触发 resizeEvent，这里主动刷新遮罩
        itemData->updateGroupMask();
        x += width - overlap;
    }
    update();
}
