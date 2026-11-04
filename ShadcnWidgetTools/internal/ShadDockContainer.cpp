#include "ShadDockContainer.h"

#include <QList>
#include <QPoint>
#include <QRect>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QtGlobal>

#include "ShadDockGroup.h"
#include "ShadDockOverlay.h"
#include "ShadResizable.h"

namespace
{
    bool isExplicitlyHidden(const QWidget* widget)
    {
        return widget->testAttribute(Qt::WA_WState_ExplicitShowHide) && widget->testAttribute(Qt::WA_WState_Hidden);
    }
} // namespace

ShadDockContainer::ShadDockContainer(QWidget* parent)
    : QWidget(parent)
{
    _layout = new QVBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    _layout->setSpacing(0);
    _root = createSplitter(Qt::Horizontal);
    _layout->addWidget(_root);
}

ShadDockContainer::~ShadDockContainer()
{
}

ShadResizable* ShadDockContainer::rootSplitter() const
{
    return _root;
}

QList<ShadDockGroup*> ShadDockContainer::groups() const
{
    QList<ShadDockGroup*> result;
    collectGroups(_root, &result);
    return result;
}

ShadDockGroup* ShadDockContainer::groupAt(const QPoint& pos) const
{
    for (ShadDockGroup* group : groups())
    {
        if (group->isVisible() && QRect(group->mapTo(this, QPoint(0, 0)), group->size()).contains(pos))
        {
            return group;
        }
    }
    return nullptr;
}

bool ShadDockContainer::isEmpty() const
{
    return groups().isEmpty();
}

void ShadDockContainer::insertNode(QWidget* node, QWidget* target, ShadDockType::DockArea area, int extent)
{
    if (node == nullptr)
    {
        return;
    }
    const Qt::Orientation orientation = area == ShadDockType::TopArea || area == ShadDockType::BottomArea ? Qt::Vertical : Qt::Horizontal;
    // RTL 由分隔条自行镜像
    const bool isAfter = area == ShadDockType::RightArea || area == ShadDockType::BottomArea || area == ShadDockType::CenterArea;
    ShadResizable* parent = target != nullptr && target != _root ? parentSplitter(target) : nullptr;
    if (parent == nullptr)
    {
        if (_root->count() == 0)
        {
            _root->addWidget(node);
            node->show();
            updateStretch();
            return;
        }
        if (_root->count() == 1)
        {
            _root->setOrientation(orientation);
        }
        if (_root->orientation() != orientation)
        {
            ShadResizable* wrapper = createSplitter(_root->orientation());
            const QList<int> sizes = _root->sizes();
            while (_root->count() > 0)
            {
                moveInto(wrapper, wrapper->count(), _root->widget(0));
            }
            applySizes(wrapper, sizes);
            _root->setOrientation(orientation);
            _root->addWidget(wrapper);
            wrapper->show();
        }
        insertSized(_root, isAfter ? _root->count() : 0, node, extent, nullptr);
        updateStretch();
        return;
    }
    if (parent->count() == 1)
    {
        parent->setOrientation(orientation);
    }
    const int targetIndex = parent->indexOf(target);
    if (parent->orientation() == orientation)
    {
        insertSized(parent, isAfter ? targetIndex + 1 : targetIndex, node, extent, target);
        updateStretch();
        return;
    }
    const QList<int> parentSizes = parent->sizes();
    const int span = orientation == Qt::Horizontal ? target->width() : target->height();
    const bool wasHidden = isExplicitlyHidden(target);
    ShadResizable* wrapper = createSplitter(orientation);
    parent->replaceWidget(targetIndex, wrapper);
    wrapper->setVisible(!wasHidden);
    wrapper->addWidget(target);
    target->setVisible(!wasHidden);
    wrapper->insertWidget(isAfter ? 1 : 0, node);
    node->show();
    if (span > 0)
    {
        const int size = qBound(0, extent, span);
        const int rest = qMax(0, span - size - wrapper->handleWidth());
        wrapper->setSizes(isAfter ? QList<int>{rest, size} : QList<int>{size, rest});
    }
    applySizes(parent, parentSizes);
    updateStretch();
}

void ShadDockContainer::removeNode(QWidget* node)
{
    if (node == nullptr)
    {
        return;
    }
    ShadResizable* parent = parentSplitter(node);
    node->hide();
    node->setParent(nullptr);
    if (parent != nullptr)
    {
        simplify(parent);
    }
    updateStretch();
}

QWidget* ShadDockContainer::takeContent()
{
    if (_root->count() == 0)
    {
        return nullptr;
    }
    if (_root->count() == 1)
    {
        QWidget* node = _root->widget(0);
        node->setParent(nullptr);
        return node;
    }
    ShadResizable* wrapper = createSplitter(_root->orientation());
    const QList<int> sizes = _root->sizes();
    while (_root->count() > 0)
    {
        moveInto(wrapper, wrapper->count(), _root->widget(0));
    }
    applySizes(wrapper, sizes);
    return wrapper;
}

void ShadDockContainer::setContent(QWidget* node)
{
    if (node == nullptr)
    {
        return;
    }
    if (_root->count() > 0)
    {
        insertNode(node, nullptr, ShadDockType::RightArea, 0);
        return;
    }
    if (ShadResizable* splitter = qobject_cast<ShadResizable*>(node))
    {
        const QList<int> sizes = splitter->sizes();
        _root->setOrientation(splitter->orientation());
        while (splitter->count() > 0)
        {
            moveInto(_root, _root->count(), splitter->widget(0));
        }
        applySizes(_root, sizes);
        splitter->deleteLater();
    }
    else
    {
        _root->addWidget(node);
        node->show();
    }
    updateStretch();
}

void ShadDockContainer::clear()
{
    while (_root->count() > 0)
    {
        QWidget* node = _root->widget(0);
        node->hide();
        node->setParent(nullptr);
        node->deleteLater();
    }
    _root->setOrientation(Qt::Horizontal);
}

void ShadDockContainer::updateStretch()
{
    updateStretch(_root);
}

ShadDockOverlay* ShadDockContainer::overlay()
{
    if (_overlay == nullptr)
    {
        _overlay = new ShadDockOverlay(this);
    }
    _overlay->setGeometry(rect());
    _overlay->raise();
    _overlay->show();
    return _overlay;
}

void ShadDockContainer::hideOverlay()
{
    if (_overlay != nullptr)
    {
        _overlay->hide();
    }
}

void ShadDockContainer::applySizes(ShadResizable* splitter, const QList<int>& sizes)
{
    // 未排布时尺寸全为 0
    int total = 0;
    for (const int size : sizes)
    {
        total += size;
    }
    if (total > 0)
    {
        splitter->setSizes(sizes);
    }
}

ShadResizable* ShadDockContainer::parentSplitter(const QWidget* node)
{
    return node != nullptr ? qobject_cast<ShadResizable*>(node->parentWidget()) : nullptr;
}

ShadResizable* ShadDockContainer::createSplitter(Qt::Orientation orientation)
{
    return new ShadResizable(orientation);
}

void ShadDockContainer::moveInto(ShadResizable* splitter, int index, QWidget* widget)
{
    const bool wasHidden = isExplicitlyHidden(widget);
    splitter->insertWidget(index, widget);
    widget->setVisible(!wasHidden);
}

void ShadDockContainer::resizeEvent(QResizeEvent* event)
{
    if (_overlay != nullptr)
    {
        _overlay->setGeometry(rect());
    }
    QWidget::resizeEvent(event);
}

void ShadDockContainer::collectGroups(QWidget* node, QList<ShadDockGroup*>* result) const
{
    if (ShadDockGroup* group = qobject_cast<ShadDockGroup*>(node))
    {
        result->append(group);
        return;
    }
    if (ShadResizable* splitter = qobject_cast<ShadResizable*>(node))
    {
        for (int index = 0; index < splitter->count(); ++index)
        {
            collectGroups(splitter->widget(index), result);
        }
    }
}

bool ShadDockContainer::containsCentral(QWidget* node) const
{
    if (const ShadDockGroup* group = qobject_cast<ShadDockGroup*>(node))
    {
        return group->isCentral();
    }
    if (const ShadResizable* splitter = qobject_cast<ShadResizable*>(node))
    {
        for (int index = 0; index < splitter->count(); ++index)
        {
            if (containsCentral(splitter->widget(index)))
            {
                return true;
            }
        }
    }
    return false;
}

void ShadDockContainer::updateStretch(ShadResizable* splitter)
{
    // 只有中央文档区随窗口伸缩
    bool hasCentral = false;
    for (int index = 0; index < splitter->count(); ++index)
    {
        hasCentral = hasCentral || containsCentral(splitter->widget(index));
    }
    for (int index = 0; index < splitter->count(); ++index)
    {
        QWidget* child = splitter->widget(index);
        splitter->setStretchFactor(index, !hasCentral || containsCentral(child) ? 1 : 0);
        if (ShadResizable* childSplitter = qobject_cast<ShadResizable*>(child))
        {
            updateStretch(childSplitter);
        }
    }
}

void ShadDockContainer::simplify(ShadResizable* splitter)
{
    ShadResizable* current = splitter;
    while (current != nullptr && current != _root)
    {
        ShadResizable* parent = parentSplitter(current);
        if (parent == nullptr)
        {
            return;
        }
        if (current->count() == 0)
        {
            current->hide();
            current->setParent(nullptr);
            current->deleteLater();
            current = parent;
            continue;
        }
        if (current->count() > 1)
        {
            return;
        }
        QWidget* child = current->widget(0);
        const bool wasHidden = isExplicitlyHidden(child);
        const QList<int> sizes = parent->sizes();
        parent->replaceWidget(parent->indexOf(current), child);
        child->setVisible(!wasHidden);
        applySizes(parent, sizes);
        current->deleteLater();
        current = parent;
    }
}

void ShadDockContainer::insertSized(ShadResizable* splitter, int index, QWidget* node, int extent, QWidget* donor)
{
    QList<int> sizes = splitter->sizes();
    int total = 0;
    for (const int size : sizes)
    {
        total += size;
    }
    const int donorIndex = donor != nullptr ? splitter->indexOf(donor) : -1;
    splitter->insertWidget(index, node);
    node->show();
    // 尚未排布
    if (total <= 0)
    {
        return;
    }
    const int handle = splitter->handleWidth();
    int size = qMax(0, extent);
    if (donorIndex >= 0 && donorIndex < sizes.size())
    {
        size = qMin(size, qMax(0, sizes.at(donorIndex) - handle));
        sizes[donorIndex] = qMax(0, sizes.at(donorIndex) - size - handle);
    }
    else
    {
        size = qMin(size, qMax(0, total - handle));
        const qreal factor = qreal(qMax(0, total - size - handle)) / total;
        for (int& value : sizes)
        {
            value = qRound(value * factor);
        }
    }
    sizes.insert(index, size);
    splitter->setSizes(sizes);
}
