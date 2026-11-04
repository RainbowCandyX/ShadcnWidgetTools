#include "ShadFlowLayout.h"

#include <QLayoutItem>
#include <QMargins>
#include <QWidget>
#include <QtAlgorithms>
#include <QtGlobal>

#include "ShadFlowLayoutPrivate.h"

ShadFlowLayout::ShadFlowLayout(QWidget* parent)
    : QLayout(parent), d_ptr(new ShadFlowLayoutPrivate())
{
    Q_D(ShadFlowLayout);
    d->q_ptr = this;
    setContentsMargins(0, 0, 0, 0);
}

ShadFlowLayout::~ShadFlowLayout()
{
    // QLayout 不会删除自己持有的布局项，需要子类负责；析构中不调用 takeAt，避免触发 invalidate
    Q_D(ShadFlowLayout);
    qDeleteAll(d->_items);
    d->_items.clear();
}

void ShadFlowLayout::setHorizontalSpacing(int spacing)
{
    Q_D(ShadFlowLayout);
    d->_horizontalSpacing = qMax(0, spacing);
    invalidate();
}

int ShadFlowLayout::getHorizontalSpacing() const
{
    Q_D(const ShadFlowLayout);
    return d->_horizontalSpacing;
}

void ShadFlowLayout::setVerticalSpacing(int spacing)
{
    Q_D(ShadFlowLayout);
    d->_verticalSpacing = qMax(0, spacing);
    invalidate();
}

int ShadFlowLayout::getVerticalSpacing() const
{
    Q_D(const ShadFlowLayout);
    return d->_verticalSpacing;
}

void ShadFlowLayout::addItem(QLayoutItem* item)
{
    Q_D(ShadFlowLayout);
    d->_items.append(item);
    invalidate();
}

int ShadFlowLayout::count() const
{
    Q_D(const ShadFlowLayout);
    return d->_items.size();
}

QLayoutItem* ShadFlowLayout::itemAt(int index) const
{
    Q_D(const ShadFlowLayout);
    return d->_items.value(index, nullptr);
}

QLayoutItem* ShadFlowLayout::takeAt(int index)
{
    Q_D(ShadFlowLayout);
    if (index < 0 || index >= d->_items.size())
    {
        return nullptr;
    }
    QLayoutItem* item = d->_items.takeAt(index);
    invalidate();
    return item;
}

Qt::Orientations ShadFlowLayout::expandingDirections() const
{
    return {};
}

bool ShadFlowLayout::hasHeightForWidth() const
{
    return true;
}

int ShadFlowLayout::heightForWidth(int width) const
{
    Q_D(const ShadFlowLayout);
    return d->doLayout(QRect(0, 0, width, 0), true);
}

QSize ShadFlowLayout::minimumSize() const
{
    Q_D(const ShadFlowLayout);
    QSize size(0, 0);
    for (QLayoutItem* item : d->_items)
    {
        if (!item->isEmpty())
        {
            size = size.expandedTo(item->sizeHint());
        }
    }
    const QMargins margins = contentsMargins();
    return size + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
}

QSize ShadFlowLayout::sizeHint() const
{
    return minimumSize();
}

void ShadFlowLayout::setGeometry(const QRect& rect)
{
    Q_D(ShadFlowLayout);
    QLayout::setGeometry(rect);
    d->doLayout(rect, false);
}
