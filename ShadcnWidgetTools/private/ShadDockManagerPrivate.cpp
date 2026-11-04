#include "ShadDockManagerPrivate.h"

#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QEvent>
#include <QJsonArray>
#include <QJsonValue>
#include <QKeyEvent>
#include <QLayout>
#include <QMetaObject>
#include <QMouseEvent>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDockContainer.h"
#include "ShadDockFloatingWindow.h"
#include "ShadDockManager.h"
#include "ShadDockOverlay.h"
#include "ShadDockPanel.h"
#include "ShadDockPanelPrivate.h"
#include "ShadResizable.h"
#include "ShadWidget.h"

namespace
{
    constexpr qreal kDragOpacity = 0.75;
    constexpr int kMinFloatingWidth = 320;
    constexpr int kMaxFloatingWidth = 960;
    constexpr int kMinFloatingHeight = 200;
    constexpr int kMaxFloatingHeight = 720;
} // namespace

ShadDockManagerPrivate::ShadDockManagerPrivate()
{
}

ShadDockManagerPrivate::~ShadDockManagerPrivate()
{
}

void ShadDockManagerPrivate::registerPanel(ShadDockPanel* panel, bool isDocument)
{
    Q_Q(ShadDockManager);
    ShadDockPanelPrivate* panelD = panel->d_func();
    if (panelD->_manager.data() != q)
    {
        if (!panelD->_manager.isNull())
        {
            panelD->_manager->removePanel(panel);
        }
        panelD->_manager = q;
        _panels.append(panel);
    }
    panelD->_isDocument = isDocument;
}

void ShadDockManagerPrivate::placePanel(ShadDockPanel* panel, ShadDockType::DockArea area, ShadDockGroup* relativeGroup)
{
    if (area == ShadDockType::CenterArea)
    {
        ShadDockGroup* group = relativeGroup;
        if (group == nullptr || (group->kind() == ShadDockGroup::Kind::Tool && panel->isDocument()))
        {
            group = documentGroup();
        }
        insertIntoGroup(group, panel, -1);
        group->setCurrentPanel(panel);
        return;
    }
    ShadDockContainer* container = relativeGroup != nullptr ? containerOf(relativeGroup) : nullptr;
    if (container == nullptr)
    {
        container = _mainContainer;
        relativeGroup = nullptr;
    }
    ShadDockGroup* group = createGroup(panel->isDocument() ? ShadDockGroup::Kind::Document : ShadDockGroup::Kind::Tool);
    insertIntoGroup(group, panel, -1);
    container->insertNode(group, relativeGroup, area, defaultExtent(container, relativeGroup, area, group));
    updateWindow(windowOf(container));
}

void ShadDockManagerPrivate::insertIntoGroup(ShadDockGroup* group, ShadDockPanel* panel, int index)
{
    group->insertPanel(index, panel);
    panel->d_func()->_group = group;
    setPanelOpen(panel, true);
    updateWindow(windowOf(group));
}

void ShadDockManagerPrivate::takeFromGroup(ShadDockPanel* panel)
{
    ShadDockPanelPrivate* panelD = panel->d_func();
    ShadDockGroup* group = panelD->_group;
    if (group == nullptr)
    {
        return;
    }
    group->removePanel(panel);
    panelD->_group = nullptr;
    updateWindow(windowOf(group));
}

void ShadDockManagerPrivate::setPanelOpen(ShadDockPanel* panel, bool open)
{
    panel->d_func()->_isOpen = open;
    panel->toggleViewAction()->setChecked(open);
}

void ShadDockManagerPrivate::panelChanged(ShadDockPanel* panel)
{
    ShadDockGroup* group = panel->d_func()->_group;
    if (group != nullptr)
    {
        group->updatePanel(panel);
        updateWindow(windowOf(group));
    }
}

void ShadDockManagerPrivate::panelDestroyed(ShadDockPanel* panel)
{
    ShadDockPanelPrivate* panelD = panel->d_func();
    _panels.removeAll(panel);
    ShadDockGroup* group = panelD->_group;
    panelD->_group = nullptr;
    panelD->_manager = nullptr;
    // 面板组析构中不处理
    if (group != nullptr && group->forgetPanel(panel))
    {
        ShadDockFloatingWindow* window = windowOf(group);
        releaseGroupIfEmpty(group);
        updateWindow(window);
    }
    updateCurrentPanel();
}

void ShadDockManagerPrivate::requestClose(ShadDockPanel* panel)
{
    Q_Q(ShadDockManager);
    if (panel == nullptr || panel->getPanelFlags().testFlag(ShadDockType::PanelNotClosable))
    {
        return;
    }
    const QPointer<ShadDockPanel> guard(panel);
    Q_EMIT panel->closeRequested();
    if (!guard.isNull() && guard->isDefaultCloseEnabled())
    {
        q->closePanel(guard);
    }
}

ShadDockGroup* ShadDockManagerPrivate::createGroup(ShadDockGroup::Kind kind)
{
    ShadDockGroup* group = new ShadDockGroup(kind);
    connect(group, &ShadDockGroup::currentChanged, this, [this, group]()
    {
        updateWindow(windowOf(group));
        if (group == _activeGroup)
        {
            updateCurrentPanel();
        }
    });
    connect(group, &ShadDockGroup::closeRequested, this, [this](ShadDockPanel* panel) { requestClose(panel); });
    connect(group, &ShadDockGroup::panelDragRequested, this, [this, group](ShadDockPanel* panel, const QPoint& grabOffset) { startPanelDrag(group, panel, grabOffset); });
    connect(group, &ShadDockGroup::groupDragRequested, this, [this, group](const QPoint& grabPos) { startGroupDrag(group, grabPos); });
    connect(group, &ShadDockGroup::pressed, this, [this, group]()
    {
        // 等标签栏处理完点击再激活
        const QPointer<ShadDockGroup> guard(group);
        QMetaObject::invokeMethod(this, [this, guard]()
        {
            if (!guard.isNull() && !_drag.isActive)
            {
                activateGroup(guard);
            }
        }, Qt::QueuedConnection);
    });
    return group;
}

ShadDockGroup* ShadDockManagerPrivate::documentGroup() const
{
    ShadDockGroup* group = _lastDocumentGroup;
    // 跳过拖动中隐藏的组
    const bool isHidden = group != nullptr && group->testAttribute(Qt::WA_WState_ExplicitShowHide) && group->isHidden();
    if (group != nullptr && containerOf(group) == _mainContainer && !isHidden)
    {
        return group;
    }
    return _centralGroup;
}

void ShadDockManagerPrivate::releaseGroupIfEmpty(ShadDockGroup* group)
{
    if (group == nullptr || group->count() > 0 || group->isCentral())
    {
        return;
    }
    // 来源组在拖动结束时处理
    if (_drag.isActive && group == _drag.sourceGroup)
    {
        return;
    }
    ShadDockContainer* container = containerOf(group);
    if (_activeGroup == group)
    {
        _activeGroup = nullptr;
    }
    if (container != nullptr)
    {
        container->removeNode(group);
    }
    else
    {
        group->hide();
    }
    group->deleteLater();
    releaseWindowIfEmpty(windowOf(container));
    updateCurrentPanel();
}

ShadDockFloatingWindow* ShadDockManagerPrivate::createFloatingWindow()
{
    Q_Q(ShadDockManager);
    ShadDockFloatingWindow* window = new ShadDockFloatingWindow(q->window());
    _windows.prepend(window);
    connect(window, &ShadDockFloatingWindow::activated, this, [this, window]()
    {
        _windows.removeAll(window);
        _windows.prepend(window);
    });
    connect(window, &ShadWidget::closeRequested, this, [this, window]() { requestCloseWindow(window); });
    connect(window, &ShadDockFloatingWindow::systemMoving, this, [this, window]()
    {
        if (!_drag.isActive)
        {
            if (isWindowMovable(window))
            {
                startWindowDrag(window, QCursor::pos(), true);
            }
            return;
        }
        if (_drag.isSystemMove && _drag.window == window)
        {
            updateDrag(QCursor::pos());
        }
    });
    connect(window, &ShadDockFloatingWindow::systemMoveFinished, this, [this, window]()
    {
        if (_drag.isActive && _drag.isSystemMove && _drag.window == window)
        {
            finishDrag(false);
        }
    });
    connect(window, &ShadDockFloatingWindow::titleBarDragRequested, this, [this, window](const QPoint& pressGlobalPos)
    {
        if (!_drag.isActive && isWindowMovable(window))
        {
            startWindowDrag(window, pressGlobalPos, false);
        }
    });
    return window;
}

void ShadDockManagerPrivate::releaseWindowIfEmpty(ShadDockFloatingWindow* window)
{
    if (window == nullptr || !window->container()->isEmpty())
    {
        return;
    }
    if (_drag.isActive && _drag.window == window)
    {
        return;
    }
    _windows.removeAll(window);
    window->hide();
    window->deleteLater();
}

void ShadDockManagerPrivate::updateWindow(ShadDockFloatingWindow* window)
{
    if (window == nullptr || !_windows.contains(window))
    {
        return;
    }
    const QList<ShadDockGroup*> groups = window->container()->groups();
    ShadDockGroup* titleGroup = !_activeGroup.isNull() && windowOf(_activeGroup) == window ? _activeGroup.data() : groups.value(0);
    const ShadDockPanel* current = titleGroup != nullptr ? titleGroup->currentPanel() : nullptr;
    window->setWindowTitle(current != nullptr ? current->getTitle() : QString());
    bool isClosable = true;
    for (const ShadDockPanel* panel : panelsIn(window->container()))
    {
        isClosable = isClosable && !panel->getPanelFlags().testFlag(ShadDockType::PanelNotClosable);
    }
    ShadWindowType::ButtonFlags flags = ShadWindowType::MaximizeButton;
    flags.setFlag(ShadWindowType::CloseButton, isClosable);
    if (window->getButtonFlags() != flags)
    {
        window->setButtonFlags(flags);
    }
}

void ShadDockManagerPrivate::requestCloseWindow(ShadDockFloatingWindow* window)
{
    QList<QPointer<ShadDockPanel>> guards;
    for (ShadDockPanel* panel : panelsIn(window->container()))
    {
        guards.append(panel);
    }
    for (const QPointer<ShadDockPanel>& panel : guards)
    {
        if (!panel.isNull())
        {
            requestClose(panel);
        }
    }
}

bool ShadDockManagerPrivate::isWindowMovable(const ShadDockFloatingWindow* window) const
{
    const QList<ShadDockPanel*> list = panelsIn(window->container());
    if (list.isEmpty())
    {
        return false;
    }
    for (const ShadDockPanel* panel : list)
    {
        if (panel->getPanelFlags().testFlag(ShadDockType::PanelNotMovable))
        {
            return false;
        }
    }
    return true;
}

QList<ShadDockContainer*> ShadDockManagerPrivate::containers() const
{
    QList<ShadDockContainer*> result;
    for (const QPointer<ShadDockFloatingWindow>& window : _windows)
    {
        if (!window.isNull())
        {
            result.append(window->container());
        }
    }
    result.append(_mainContainer);
    return result;
}

ShadDockContainer* ShadDockManagerPrivate::containerOf(const QWidget* widget)
{
    for (QWidget* ancestor = widget != nullptr ? widget->parentWidget() : nullptr; ancestor != nullptr; ancestor = ancestor->parentWidget())
    {
        if (ShadDockContainer* container = qobject_cast<ShadDockContainer*>(ancestor))
        {
            return container;
        }
    }
    return nullptr;
}

ShadDockFloatingWindow* ShadDockManagerPrivate::windowOf(const QWidget* widget)
{
    return widget != nullptr ? qobject_cast<ShadDockFloatingWindow*>(widget->window()) : nullptr;
}

QList<ShadDockPanel*> ShadDockManagerPrivate::panelsIn(const ShadDockContainer* container)
{
    QList<ShadDockPanel*> result;
    for (const ShadDockGroup* group : container->groups())
    {
        result.append(group->panels());
    }
    return result;
}

void ShadDockManagerPrivate::setActiveGroup(ShadDockGroup* group)
{
    if (_activeGroup != group)
    {
        ShadDockFloatingWindow* oldWindow = windowOf(_activeGroup);
        if (!_activeGroup.isNull())
        {
            _activeGroup->setActive(false);
        }
        _activeGroup = group;
        if (group != nullptr)
        {
            group->setActive(true);
            if (group->kind() == ShadDockGroup::Kind::Document && containerOf(group) == _mainContainer)
            {
                _lastDocumentGroup = group;
            }
        }
        updateWindow(oldWindow);
        updateWindow(windowOf(group));
    }
    updateCurrentPanel();
}

void ShadDockManagerPrivate::activateGroup(ShadDockGroup* group)
{
    if (group == nullptr || containerOf(group) == nullptr)
    {
        return;
    }
    setActiveGroup(group);
    ShadDockPanel* panel = group->currentPanel();
    if (panel == nullptr)
    {
        return;
    }
    const QWidget* focus = QApplication::focusWidget();
    if (focus != nullptr && (focus == panel || panel->isAncestorOf(focus)))
    {
        return;
    }
    QWidget* target = panel->focusWidget() != nullptr ? panel->focusWidget() : panel;
    target->setFocus(Qt::OtherFocusReason);
}

void ShadDockManagerPrivate::updateCurrentPanel()
{
    Q_Q(ShadDockManager);
    ShadDockPanel* panel = _activeGroup.isNull() ? nullptr : _activeGroup->currentPanel();
    if (_currentPanel.data() == panel)
    {
        return;
    }
    _currentPanel = panel;
    Q_EMIT q->currentPanelChanged(panel);
}

int ShadDockManagerPrivate::defaultExtent(const ShadDockContainer* container, const ShadDockGroup* relativeGroup, ShadDockType::DockArea area, const ShadDockGroup* group) const
{
    const bool isHorizontal = area == ShadDockType::LeftArea || area == ShadDockType::RightArea;
    const QSize hint = group->sizeHint();
    const int preferred = isHorizontal ? hint.width() : hint.height();
    const QWidget* reference = relativeGroup != nullptr ? static_cast<const QWidget*>(relativeGroup) : container;
    const int span = isHorizontal ? reference->width() : reference->height();
    return span > 0 ? qMin(preferred, span / (relativeGroup != nullptr ? 2 : 3)) : preferred;
}

void ShadDockManagerPrivate::startPanelDrag(ShadDockGroup* group, ShadDockPanel* panel, const QPoint& grabOffset)
{
    if (_drag.isActive || panel == nullptr || panel->getPanelFlags().testFlag(ShadDockType::PanelNotMovable))
    {
        return;
    }
    const int index = group->indexOf(panel);
    if (index < 0)
    {
        return;
    }
    _drag = DragState();
    _drag.anchorOffset = grabOffset;
    ShadDockFloatingWindow* window = windowOf(group);
    // 浮动窗口中唯一的面板直接移动窗口
    if (window != nullptr && window->container()->groups().size() == 1 && group->count() == 1)
    {
        _drag.anchorGroup = group;
        _drag.anchorIndex = index;
        beginDrag(window, false);
        return;
    }
    const bool isGroupKept = group->count() > 1 || group->isCentral();
    ShadDockFloatingWindow* floating = detachToWindow({panel}, panel->isDocument() ? ShadDockGroup::Kind::Document : ShadDockGroup::Kind::Tool, group->size());
    _drag.sourceGroup = group;
    _drag.sourceIndex = index;
    _drag.panels.append(panel);
    if (!isGroupKept)
    {
        group->hide();
    }
    _drag.anchorGroup = floating->container()->groups().value(0);
    _drag.anchorIndex = 0;
    beginDrag(floating, false);
}

void ShadDockManagerPrivate::startGroupDrag(ShadDockGroup* group, const QPoint& grabPos)
{
    if (_drag.isActive || group->count() == 0)
    {
        return;
    }
    const QList<ShadDockPanel*> list = group->panels();
    for (const ShadDockPanel* panel : list)
    {
        if (panel->getPanelFlags().testFlag(ShadDockType::PanelNotMovable))
        {
            return;
        }
    }
    _drag = DragState();
    _drag.anchorOffset = grabPos;
    _drag.isHeaderAnchor = true;
    ShadDockFloatingWindow* window = windowOf(group);
    if (window != nullptr && window->container()->groups().size() == 1)
    {
        _drag.anchorGroup = group;
        beginDrag(window, false);
        return;
    }
    ShadDockPanel* current = group->currentPanel();
    ShadDockFloatingWindow* floating = detachToWindow(list, group->kind(), group->size());
    ShadDockGroup* newGroup = floating->container()->groups().value(0);
    newGroup->setCurrentPanel(current);
    _drag.sourceGroup = group;
    _drag.sourceIndex = 0;
    for (ShadDockPanel* panel : list)
    {
        _drag.panels.append(panel);
    }
    if (!group->isCentral())
    {
        group->hide();
    }
    _drag.anchorGroup = newGroup;
    beginDrag(floating, false);
}

void ShadDockManagerPrivate::startWindowDrag(ShadDockFloatingWindow* window, const QPoint& pressGlobalPos, bool isSystemMove)
{
    if (_drag.isActive || window == nullptr)
    {
        return;
    }
    _drag = DragState();
    _drag.anchor = window->mapFromGlobal(pressGlobalPos);
    beginDrag(window, isSystemMove);
}

ShadDockFloatingWindow* ShadDockManagerPrivate::detachToWindow(const QList<ShadDockPanel*>& panels, ShadDockGroup::Kind kind, const QSize& size)
{
    ShadDockFloatingWindow* window = createFloatingWindow();
    ShadDockGroup* group = createGroup(kind);
    window->container()->setContent(group);
    for (ShadDockPanel* panel : panels)
    {
        takeFromGroup(panel);
        insertIntoGroup(group, panel, -1);
    }
    const int width = qBound(kMinFloatingWidth, size.width(), kMaxFloatingWidth);
    const int height = qBound(kMinFloatingHeight, size.height(), kMaxFloatingHeight);
    window->resize(width, height + window->getTitleBarHeight());
    return window;
}

void ShadDockManagerPrivate::beginDrag(ShadDockFloatingWindow* window, bool isSystemMove)
{
    _drag.isActive = true;
    _drag.isSystemMove = isSystemMove;
    _drag.window = window;
    for (const ShadDockPanel* panel : panelsIn(window->container()))
    {
        _drag.hasDocuments = _drag.hasDocuments || panel->isDocument();
    }
    window->setWindowOpacity(kDragOpacity);
    if (!isSystemMove)
    {
        const auto updateAnchor = [this, window]()
        {
            ShadDockGroup* group = _drag.anchorGroup;
            if (group == nullptr)
            {
                return;
            }
            const QPoint local = _drag.isHeaderAnchor ? group->headerAnchor(_drag.anchorOffset) : group->dragAnchor(_drag.anchorIndex, _drag.anchorOffset);
            _drag.anchor = group->mapTo(window, local);
        };
        if (!window->isVisible())
        {
            // 不抢走原窗口的鼠标
            window->setAttribute(Qt::WA_ShowWithoutActivating, true);
            if (window->layout() != nullptr)
            {
                window->layout()->activate();
            }
            updateAnchor();
            alignWindow(QCursor::pos());
            window->setVisible(true);
        }
        // 显示后布局才确定
        updateAnchor();
        alignWindow(QCursor::pos());
        qApp->installEventFilter(this);
    }
    updateDrag(QCursor::pos());
}

void ShadDockManagerPrivate::alignWindow(const QPoint& globalPos)
{
    ShadDockFloatingWindow* window = _drag.window;
    if (window == nullptr)
    {
        return;
    }
    const QPoint delta = globalPos - window->mapToGlobal(_drag.anchor);
    if (!delta.isNull())
    {
        window->move(window->pos() + delta);
    }
}

void ShadDockManagerPrivate::updateDrag(const QPoint& globalPos)
{
    if (!_drag.isActive)
    {
        return;
    }
    if (_drag.window.isNull())
    {
        finishDrag(true);
        return;
    }
    if (!_drag.isSystemMove)
    {
        alignWindow(globalPos);
    }
    DropTarget target;
    ShadDockContainer* container = nullptr;
    const QList<ShadDockContainer*> candidates = containers();
    for (ShadDockContainer* candidate : candidates)
    {
        if (candidate == _drag.window->container() || !candidate->isVisible() || candidate->window()->isMinimized())
        {
            continue;
        }
        if (candidate->rect().contains(candidate->mapFromGlobal(globalPos)))
        {
            container = candidate;
            break;
        }
    }
    for (ShadDockContainer* candidate : candidates)
    {
        if (candidate != container)
        {
            candidate->hideOverlay();
        }
    }
    ShadDockGroup* indicatorGroup = nullptr;
    int indicatorIndex = -1;
    if (container != nullptr)
    {
        const QPoint pos = container->mapFromGlobal(globalPos);
        ShadDockGroup* group = container->groupAt(pos);
        // 文档不能放进工具面板组
        const bool isCenterAllowed = group != nullptr && (group->kind() == ShadDockGroup::Kind::Document || !_drag.hasDocuments);
        ShadDockOverlay* overlay = container->overlay();
        overlay->setGroupRect(group != nullptr ? QRect(group->mapTo(container, QPoint(0, 0)), group->size()) : QRect(), isCenterAllowed);
        const ShadDockOverlay::Hit hit = overlay->hitTest(pos);
        overlay->setHover(hit);
        target.container = container;
        if (hit.isValid)
        {
            target.isValid = true;
            target.area = hit.area;
            target.group = hit.isEdge ? nullptr : group;
        }
        else if (isCenterAllowed && group->dropStripRect().contains(group->mapFrom(container, pos)))
        {
            target.isValid = true;
            target.area = ShadDockType::CenterArea;
            target.group = group;
            target.tabIndex = group->insertionIndexAt(group->mapFrom(container, pos));
            indicatorGroup = group;
            indicatorIndex = target.tabIndex;
        }
        overlay->setPreview(target.isValid ? previewRect(container, &target) : QRect());
    }
    if (!_drag.indicatorGroup.isNull() && _drag.indicatorGroup != indicatorGroup)
    {
        _drag.indicatorGroup->setDropIndicator(-1);
    }
    if (indicatorGroup != nullptr)
    {
        indicatorGroup->setDropIndicator(indicatorIndex);
    }
    _drag.indicatorGroup = indicatorGroup;
    _drag.target = target;
}

void ShadDockManagerPrivate::finishDrag(bool isCancelled)
{
    Q_Q(ShadDockManager);
    if (!_drag.isActive)
    {
        return;
    }
    if (!_drag.isSystemMove)
    {
        qApp->removeEventFilter(this);
    }
    clearDropFeedback();
    const DragState drag = _drag;
    _drag = DragState();
    ShadDockFloatingWindow* window = drag.window;
    if (window != nullptr)
    {
        window->setWindowOpacity(1.0);
        window->setAttribute(Qt::WA_ShowWithoutActivating, false);
    }
    ShadDockGroup* source = drag.sourceGroup;
    const bool hasTarget = drag.target.isValid && !drag.target.container.isNull() && (drag.target.area != ShadDockType::CenterArea || !drag.target.group.isNull());
    if (window != nullptr && !isCancelled && hasTarget)
    {
        releaseGroupIfEmpty(source);
        dock(window, drag.target);
        return;
    }
    bool isFloatingAllowed = true;
    for (const QPointer<ShadDockPanel>& panel : drag.panels)
    {
        isFloatingAllowed = isFloatingAllowed && (panel.isNull() || !panel->getPanelFlags().testFlag(ShadDockType::PanelNotFloatable));
    }
    if (window != nullptr && !isFloatingAllowed && source != nullptr && containerOf(source) != nullptr)
    {
        int index = drag.sourceIndex;
        ShadDockPanel* first = nullptr;
        for (const QPointer<ShadDockPanel>& panel : drag.panels)
        {
            if (panel.isNull())
            {
                continue;
            }
            first = first != nullptr ? first : panel.data();
            takeFromGroup(panel);
            insertIntoGroup(source, panel, index);
            index = index >= 0 ? index + 1 : index;
        }
        source->show();
        for (ShadDockGroup* group : window->container()->groups())
        {
            releaseGroupIfEmpty(group);
        }
        q->setCurrentPanel(first);
        return;
    }
    releaseGroupIfEmpty(source);
    if (window != nullptr)
    {
        updateWindow(window);
        window->raise();
        window->activateWindow();
    }
}

void ShadDockManagerPrivate::clearDropFeedback()
{
    for (ShadDockContainer* container : containers())
    {
        container->hideOverlay();
    }
    if (!_drag.indicatorGroup.isNull())
    {
        _drag.indicatorGroup->setDropIndicator(-1);
    }
    _drag.indicatorGroup = nullptr;
}

QRect ShadDockManagerPrivate::previewRect(const ShadDockContainer* container, DropTarget* target) const
{
    const QRect bounds = target->group.isNull() ? container->rect() : QRect(target->group->mapTo(container, QPoint(0, 0)), target->group->size());
    if (target->area == ShadDockType::CenterArea)
    {
        return bounds;
    }
    const ShadDockType::DockArea area = ShadDockOverlay::physicalArea(target->area, container->layoutDirection());
    const bool isHorizontal = area == ShadDockType::LeftArea || area == ShadDockType::RightArea;
    const int span = isHorizontal ? bounds.width() : bounds.height();
    const QSize floatingSize = _drag.window->container()->size();
    const int preferred = isHorizontal ? floatingSize.width() : floatingSize.height();
    const int size = qMax(1, qMin(preferred, target->group.isNull() ? span / 3 : span / 2));
    target->extent = size;
    switch (area)
    {
        case ShadDockType::LeftArea:
            return QRect(bounds.left(), bounds.top(), size, bounds.height());
        case ShadDockType::RightArea:
            return QRect(bounds.right() - size + 1, bounds.top(), size, bounds.height());
        case ShadDockType::TopArea:
            return QRect(bounds.left(), bounds.top(), bounds.width(), size);
        default:
            return QRect(bounds.left(), bounds.bottom() - size + 1, bounds.width(), size);
    }
}

void ShadDockManagerPrivate::dock(ShadDockFloatingWindow* window, const DropTarget& target)
{
    Q_Q(ShadDockManager);
    ShadDockContainer* floatingContainer = window->container();
    const QList<ShadDockPanel*> moved = panelsIn(floatingContainer);
    const ShadDockGroup* focusGroup = !_activeGroup.isNull() && windowOf(_activeGroup) == window ? _activeGroup.data() : floatingContainer->groups().value(0);
    ShadDockPanel* focusPanel = focusGroup != nullptr ? focusGroup->currentPanel() : nullptr;
    ShadDockContainer* container = target.container;
    if (target.area == ShadDockType::CenterArea)
    {
        ShadDockGroup* group = target.group;
        int index = target.tabIndex;
        for (ShadDockPanel* panel : moved)
        {
            takeFromGroup(panel);
            insertIntoGroup(group, panel, index);
            index = index >= 0 ? index + 1 : index;
        }
        for (ShadDockGroup* emptyGroup : floatingContainer->groups())
        {
            releaseGroupIfEmpty(emptyGroup);
        }
    }
    else
    {
        container->insertNode(floatingContainer->takeContent(), target.group, target.area, target.extent);
        releaseWindowIfEmpty(window);
    }
    updateWindow(windowOf(container));
    q->setCurrentPanel(focusPanel != nullptr ? focusPanel : moved.value(0));
}

QJsonObject ShadDockManagerPrivate::saveNode(QWidget* node) const
{
    QJsonObject object;
    if (const ShadDockGroup* group = qobject_cast<const ShadDockGroup*>(node))
    {
        object.insert(QStringLiteral("type"), QStringLiteral("group"));
        object.insert(QStringLiteral("kind"), group->kind() == ShadDockGroup::Kind::Tool ? QStringLiteral("tool") : QStringLiteral("document"));
        if (group->isCentral())
        {
            object.insert(QStringLiteral("central"), true);
        }
        QJsonArray ids;
        for (const ShadDockPanel* panel : group->panels())
        {
            ids.append(panel->getId());
        }
        object.insert(QStringLiteral("panels"), ids);
        if (const ShadDockPanel* current = group->currentPanel())
        {
            object.insert(QStringLiteral("current"), current->getId());
        }
        return object;
    }
    const ShadResizable* splitter = qobject_cast<const ShadResizable*>(node);
    if (splitter == nullptr)
    {
        return object;
    }
    object.insert(QStringLiteral("type"), QStringLiteral("splitter"));
    object.insert(QStringLiteral("orientation"), splitter->orientation() == Qt::Vertical ? QStringLiteral("vertical") : QStringLiteral("horizontal"));
    const QList<int> values = splitter->sizes();
    QJsonArray sizes;
    QJsonArray children;
    for (int index = 0; index < splitter->count(); ++index)
    {
        QWidget* child = splitter->widget(index);
        // 跳过拖动中隐藏的组
        if (child->testAttribute(Qt::WA_WState_ExplicitShowHide) && child->testAttribute(Qt::WA_WState_Hidden))
        {
            continue;
        }
        children.append(saveNode(child));
        sizes.append(values.value(index));
    }
    object.insert(QStringLiteral("sizes"), sizes);
    object.insert(QStringLiteral("children"), children);
    return object;
}

QWidget* ShadDockManagerPrivate::buildNode(const QJsonObject& object, bool isMain, QSet<const ShadDockPanel*>* placed)
{
    Q_Q(ShadDockManager);
    const QString type = object.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("group"))
    {
        const ShadDockGroup::Kind kind = object.value(QStringLiteral("kind")).toString() == QStringLiteral("tool") ? ShadDockGroup::Kind::Tool : ShadDockGroup::Kind::Document;
        const bool isCentral = isMain && kind == ShadDockGroup::Kind::Document && _centralGroup.isNull() && object.value(QStringLiteral("central")).toBool();
        ShadDockGroup* group = createGroup(kind);
        group->setCentral(isCentral);
        for (const QJsonValue& value : object.value(QStringLiteral("panels")).toArray())
        {
            ShadDockPanel* panel = q->findPanel(value.toString());
            if (panel == nullptr || placed->contains(panel))
            {
                continue;
            }
            placed->insert(panel);
            takeFromGroup(panel);
            insertIntoGroup(group, panel, -1);
        }
        if (group->count() == 0 && !isCentral)
        {
            delete group;
            return nullptr;
        }
        group->setCurrentPanel(q->findPanel(object.value(QStringLiteral("current")).toString()));
        if (isCentral)
        {
            _centralGroup = group;
        }
        return group;
    }
    if (type != QStringLiteral("splitter"))
    {
        return nullptr;
    }
    const Qt::Orientation orientation = object.value(QStringLiteral("orientation")).toString() == QStringLiteral("vertical") ? Qt::Vertical : Qt::Horizontal;
    const QJsonArray children = object.value(QStringLiteral("children")).toArray();
    const QJsonArray sizeValues = object.value(QStringLiteral("sizes")).toArray();
    QList<QWidget*> nodes;
    QList<int> sizes;
    for (int index = 0; index < children.size(); ++index)
    {
        QWidget* node = children.at(index).isObject() ? buildNode(children.at(index).toObject(), isMain, placed) : nullptr;
        if (node != nullptr)
        {
            nodes.append(node);
            sizes.append(sizeValues.at(index).toInt());
        }
    }
    if (nodes.size() <= 1)
    {
        return nodes.value(0);
    }
    ShadResizable* splitter = ShadDockContainer::createSplitter(orientation);
    for (QWidget* node : nodes)
    {
        splitter->addWidget(node);
        node->show();
    }
    ShadDockContainer::applySizes(splitter, sizes);
    return splitter;
}

void ShadDockManagerPrivate::restoreRoot(ShadDockContainer* container, const QJsonObject& object, bool isMain, QSet<const ShadDockPanel*>* placed)
{
    if (object.value(QStringLiteral("type")).toString() != QStringLiteral("splitter"))
    {
        container->setContent(buildNode(object, isMain, placed));
        return;
    }
    ShadResizable* root = container->rootSplitter();
    root->setOrientation(object.value(QStringLiteral("orientation")).toString() == QStringLiteral("vertical") ? Qt::Vertical : Qt::Horizontal);
    const QJsonArray children = object.value(QStringLiteral("children")).toArray();
    const QJsonArray sizeValues = object.value(QStringLiteral("sizes")).toArray();
    QList<int> sizes;
    for (int index = 0; index < children.size(); ++index)
    {
        QWidget* node = children.at(index).isObject() ? buildNode(children.at(index).toObject(), isMain, placed) : nullptr;
        if (node != nullptr)
        {
            root->addWidget(node);
            node->show();
            sizes.append(sizeValues.at(index).toInt());
        }
    }
    ShadDockContainer::applySizes(root, sizes);
    container->updateStretch();
}

bool ShadDockManagerPrivate::eventFilter(QObject* watched, QEvent* event)
{
    if (!_drag.isActive || _drag.isSystemMove)
    {
        return QObject::eventFilter(watched, event);
    }
    switch (event->type())
    {
        case QEvent::MouseMove:
            {
                const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                // 补偿错过的松开事件
                if (!mouseEvent->buttons().testFlag(Qt::LeftButton))
                {
                    finishDrag(false);
                    return false;
                }
                updateDrag(mouseEvent->globalPosition().toPoint());
                return true;
            }
        case QEvent::MouseButtonRelease:
            if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
            {
                finishDrag(false);
            }
            return false;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
            return true;
        case QEvent::KeyPress:
            if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape)
            {
                finishDrag(true);
                return true;
            }
            break;
        case QEvent::ApplicationDeactivate:
            finishDrag(true);
            break;
        default:
            break;
    }
    return QObject::eventFilter(watched, event);
}
