#include "ShadTreeViewPrivate.h"

#include <QAbstractAnimation>
#include <QEasingCurve>
#include <QHeaderView>
#include <QObject>
#include <QVariant>
#include <QVariantAnimation>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <utility>

#include "ShadTreeView.h"

ShadTreeViewPrivate::ShadTreeViewPrivate()
{
}

ShadTreeViewPrivate::~ShadTreeViewPrivate()
{
}

int ShadTreeViewPrivate::depthOf(const QModelIndex& index) const
{
    Q_Q(const ShadTreeView);
    const QModelIndex root = q->rootIndex();
    int depth = 0;
    for (QModelIndex parent = index.parent(); parent.isValid() && parent != root; parent = parent.parent())
    {
        ++depth;
    }
    return depth;
}

QRect ShadTreeViewPrivate::rowBackgroundRect(const QRect& rowRect, const QModelIndex& index) const
{
    Q_Q(const ShadTreeView);
    const QHeaderView* header = q->header();
    // 所有可见列的总范围
    int left = rowRect.right() + 1;
    int right = rowRect.left();
    for (int section = 0; section < header->count(); ++section)
    {
        if (header->isSectionHidden(section))
        {
            continue;
        }
        const int position = header->sectionViewportPosition(section);
        left = qMin(left, position);
        right = qMax(right, position + header->sectionSize(section));
    }
    if (left >= right)
    {
        return QRect();
    }
    // 树形列中扣除祖先层级的缩进，背景从该行展开箭头所在的格子开始
    const int treeColumn = q->treePosition();
    const int chevronLevel = q->rootIsDecorated() ? depthOf(index) : depthOf(index) - 1;
    const int offset = qMax(0, chevronLevel) * q->indentation();
    const int treeLeft = header->sectionViewportPosition(treeColumn);
    if (q->isRightToLeft())
    {
        right = qMin(right, treeLeft + header->sectionSize(treeColumn) - offset);
    }
    else
    {
        left = qMax(left, treeLeft + offset);
    }
    return QRect(left, rowRect.top(), qMax(0, right - left), rowRect.height());
}

qreal ShadTreeViewPrivate::chevronProgress(const QModelIndex& index) const
{
    Q_Q(const ShadTreeView);
    if (QVariantAnimation* animation = _chevronAnimations.value(QPersistentModelIndex(index), nullptr))
    {
        return animation->currentValue().toReal();
    }
    return q->isExpanded(index) ? 1.0 : 0.0;
}

void ShadTreeViewPrivate::animateChevron(const QModelIndex& index, bool expanded)
{
    Q_Q(ShadTreeView);
    const QPersistentModelIndex key(index);
    const qreal endValue = expanded ? 1.0 : 0.0;
    QVariantAnimation* animation = _chevronAnimations.value(key, nullptr);
    // 未显示时（例如构造时调用 expandAll）直接按展开状态绘制，不播放动画
    if (animation == nullptr && !q->isVisible())
    {
        return;
    }
    // 动画进行中反向时从当前角度开始
    qreal startValue = 1.0 - endValue;
    if (animation != nullptr)
    {
        startValue = animation->currentValue().toReal();
        animation->stop();
    }
    else
    {
        animation = new QVariantAnimation(q);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        QObject::connect(animation, &QVariantAnimation::valueChanged, q, [q]() { q->viewport()->update(); });
        QObject::connect(animation, &QVariantAnimation::finished, q, [this, animation]()
        {
            _chevronAnimations.removeIf([animation](const QHash<QPersistentModelIndex, QVariantAnimation*>::iterator& it) { return it.value() == animation; });
            animation->deleteLater();
        });
        _chevronAnimations.insert(key, animation);
    }
    animation->setDuration(qMax(1, qRound(kChevronDurationMs * qAbs(endValue - startValue))));
    animation->setStartValue(startValue);
    animation->setEndValue(endValue);
    animation->start();
}

void ShadTreeViewPrivate::clearChevronAnimations()
{
    for (QVariantAnimation* animation : std::as_const(_chevronAnimations))
    {
        QObject::disconnect(animation, nullptr, nullptr, nullptr);
        animation->stop();
        delete animation;
    }
    _chevronAnimations.clear();
}

void ShadTreeViewPrivate::setHoveredIndex(const QModelIndex& index)
{
    // 只记录树形列，整行一起高亮；禁用的节点不高亮
    QModelIndex rowIndex = index.isValid() ? index.siblingAtColumn(0) : QModelIndex();
    if (rowIndex.isValid() && !rowIndex.flags().testFlag(Qt::ItemIsEnabled))
    {
        rowIndex = QModelIndex();
    }
    if (_hoveredIndex == rowIndex)
    {
        return;
    }
    const QModelIndex oldIndex = _hoveredIndex;
    _hoveredIndex = rowIndex;
    updateRow(oldIndex);
    updateRow(rowIndex);
}

void ShadTreeViewPrivate::updateRow(const QModelIndex& index)
{
    Q_Q(ShadTreeView);
    if (!index.isValid())
    {
        return;
    }
    const QRect itemRect = q->visualRect(index);
    if (itemRect.isValid())
    {
        q->viewport()->update(QRect(0, itemRect.top(), q->viewport()->width(), itemRect.height()));
    }
}
