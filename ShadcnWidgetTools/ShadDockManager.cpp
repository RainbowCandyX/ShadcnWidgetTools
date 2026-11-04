#include "ShadDockManager.h"

#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QPoint>
#include <QPointer>
#include <QSet>
#include <QSize>
#include <QVBoxLayout>

#include "ShadDockContainer.h"
#include "ShadDockFloatingWindow.h"
#include "ShadDockGroup.h"
#include "ShadDockManagerPrivate.h"
#include "ShadDockPanel.h"
#include "ShadDockPanelPrivate.h"
#include "ShadResizable.h"

ShadDockManager::ShadDockManager(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadDockManagerPrivate())
{
    Q_D(ShadDockManager);
    d->q_ptr = this;
    d->_layout = new QVBoxLayout(this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->_layout->setSpacing(0);
    d->_mainContainer = new ShadDockContainer(this);
    d->_layout->addWidget(d->_mainContainer);
    ShadDockGroup* central = d->createGroup(ShadDockGroup::Kind::Document);
    central->setCentral(true);
    d->_centralGroup = central;
    d->_mainContainer->setContent(central);
    connect(qApp, &QApplication::focusChanged, d, [d](QWidget* old, QWidget* now)
    {
        Q_UNUSED(old);
        for (QWidget* widget = now; widget != nullptr; widget = widget->parentWidget())
        {
            if (ShadDockGroup* group = qobject_cast<ShadDockGroup*>(widget))
            {
                if (d->containers().contains(ShadDockManagerPrivate::containerOf(group)))
                {
                    d->setActiveGroup(group);
                }
                return;
            }
        }
    });
}

ShadDockManager::~ShadDockManager()
{
    Q_D(ShadDockManager);
    if (d->_drag.isActive && !d->_drag.isSystemMove)
    {
        qApp->removeEventFilter(d);
    }
    d->_drag = ShadDockManagerPrivate::DragState();
    // 避免面板析构时回调
    for (ShadDockPanel* panel : d->_panels)
    {
        panel->d_func()->_manager = nullptr;
    }
    const QList<QPointer<ShadDockFloatingWindow>> windows = d->_windows;
    d->_windows.clear();
    for (const QPointer<ShadDockFloatingWindow>& window : windows)
    {
        delete window.data();
    }
}

void ShadDockManager::addPanel(ShadDockPanel* panel, ShadDockType::DockArea area, ShadDockPanel* relativeTo)
{
    Q_D(ShadDockManager);
    if (panel == nullptr)
    {
        return;
    }
    const bool isNew = panel->d_func()->_manager.data() != this;
    d->registerPanel(panel, !isNew && panel->isDocument());
    if (isNew)
    {
        panel->d_func()->_defaultArea = area;
    }
    ShadDockGroup* relativeGroup = relativeTo != nullptr && relativeTo != panel && relativeTo->d_func()->_manager.data() == this ? relativeTo->d_func()->_group.data() : nullptr;
    ShadDockGroup* oldGroup = panel->d_func()->_group;
    d->takeFromGroup(panel);
    d->placePanel(panel, area, relativeGroup);
    d->releaseGroupIfEmpty(oldGroup);
}

void ShadDockManager::addDocument(ShadDockPanel* panel)
{
    Q_D(ShadDockManager);
    if (panel == nullptr)
    {
        return;
    }
    d->registerPanel(panel, true);
    ShadDockGroup* oldGroup = panel->d_func()->_group;
    d->takeFromGroup(panel);
    ShadDockGroup* group = d->documentGroup();
    d->insertIntoGroup(group, panel, -1);
    group->setCurrentPanel(panel);
    d->releaseGroupIfEmpty(oldGroup);
}

void ShadDockManager::tabifyPanel(ShadDockPanel* panel, ShadDockPanel* existing)
{
    addPanel(panel, ShadDockType::CenterArea, existing);
}

void ShadDockManager::floatPanel(ShadDockPanel* panel, const QRect& geometry)
{
    Q_D(ShadDockManager);
    if (panel == nullptr)
    {
        return;
    }
    const bool isNew = panel->d_func()->_manager.data() != this;
    d->registerPanel(panel, !isNew && panel->isDocument());
    ShadDockGroup* oldGroup = panel->d_func()->_group;
    const ShadDockGroup::Kind kind = panel->isDocument() ? ShadDockGroup::Kind::Document : ShadDockGroup::Kind::Tool;
    ShadDockFloatingWindow* window = d->detachToWindow({panel}, kind, QSize(ShadDockManagerPrivate::kDefaultFloatingWidth, ShadDockManagerPrivate::kDefaultFloatingHeight));
    QRect rect = geometry;
    if (!rect.isValid())
    {
        rect = QRect(QPoint(0, 0), window->size());
        rect.moveCenter(this->window()->geometry().center());
    }
    window->setGeometry(rect);
    window->setVisible(true);
    d->releaseGroupIfEmpty(oldGroup);
}

void ShadDockManager::removePanel(ShadDockPanel* panel)
{
    Q_D(ShadDockManager);
    if (panel == nullptr || panel->d_func()->_manager.data() != this)
    {
        return;
    }
    ShadDockGroup* oldGroup = panel->d_func()->_group;
    d->takeFromGroup(panel);
    panel->hide();
    panel->setParent(nullptr);
    d->setPanelOpen(panel, false);
    panel->d_func()->_manager = nullptr;
    d->_panels.removeAll(panel);
    d->releaseGroupIfEmpty(oldGroup);
    d->updateCurrentPanel();
}

void ShadDockManager::closePanel(ShadDockPanel* panel)
{
    Q_D(ShadDockManager);
    if (panel == nullptr || panel->d_func()->_manager.data() != this || !panel->isOpen())
    {
        return;
    }
    ShadDockPanelPrivate* panelD = panel->d_func();
    ShadDockGroup* group = panelD->_group;
    ShadDockFloatingWindow* window = ShadDockManagerPrivate::windowOf(group);
    panelD->_lastGroup = group;
    panelD->_lastIndex = group != nullptr ? group->indexOf(panel) : -1;
    panelD->_wasFloating = window != nullptr;
    if (window != nullptr)
    {
        panelD->_lastFloatingGeometry = window->geometry();
    }
    d->takeFromGroup(panel);
    // 关闭的面板仍由本组件持有
    panel->setParent(this);
    panel->hide();
    d->setPanelOpen(panel, false);
    d->releaseGroupIfEmpty(group);
    d->updateCurrentPanel();
    Q_EMIT panel->closed();
}

void ShadDockManager::showPanel(ShadDockPanel* panel)
{
    Q_D(ShadDockManager);
    if (panel == nullptr || panel->d_func()->_manager.data() != this)
    {
        return;
    }
    if (panel->isOpen())
    {
        setCurrentPanel(panel);
        return;
    }
    ShadDockPanelPrivate* panelD = panel->d_func();
    ShadDockGroup* lastGroup = panelD->_lastGroup;
    if (lastGroup != nullptr && ShadDockManagerPrivate::containerOf(lastGroup) != nullptr)
    {
        d->insertIntoGroup(lastGroup, panel, panelD->_lastIndex);
    }
    else if (panelD->_wasFloating && !panel->getPanelFlags().testFlag(ShadDockType::PanelNotFloatable))
    {
        floatPanel(panel, panelD->_lastFloatingGeometry);
    }
    else if (panel->isDocument())
    {
        d->insertIntoGroup(d->documentGroup(), panel, -1);
    }
    else
    {
        d->placePanel(panel, panelD->_defaultArea, nullptr);
    }
    setCurrentPanel(panel);
}

void ShadDockManager::setCurrentPanel(ShadDockPanel* panel)
{
    Q_D(ShadDockManager);
    if (panel == nullptr || panel->d_func()->_manager.data() != this || !panel->isOpen())
    {
        return;
    }
    ShadDockGroup* group = panel->d_func()->_group;
    if (group == nullptr)
    {
        return;
    }
    group->setCurrentPanel(panel);
    QWidget* host = ShadDockManagerPrivate::windowOf(group);
    if (host != nullptr)
    {
        host->raise();
    }
    else
    {
        host = window();
    }
    host->activateWindow();
    d->activateGroup(group);
}

ShadDockPanel* ShadDockManager::getCurrentPanel() const
{
    Q_D(const ShadDockManager);
    return d->_currentPanel.data();
}

QList<ShadDockPanel*> ShadDockManager::panels() const
{
    Q_D(const ShadDockManager);
    return d->_panels;
}

ShadDockPanel* ShadDockManager::findPanel(const QString& id) const
{
    Q_D(const ShadDockManager);
    for (ShadDockPanel* panel : d->_panels)
    {
        if (panel->getId() == id)
        {
            return panel;
        }
    }
    return nullptr;
}

QByteArray ShadDockManager::saveState() const
{
    Q_D(const ShadDockManager);
    QJsonObject root;
    root.insert(QStringLiteral("version"), ShadDockManagerPrivate::kStateVersion);
    root.insert(QStringLiteral("main"), d->saveNode(d->_mainContainer->rootSplitter()));
    // 保持窗口的层叠顺序
    QJsonArray floating;
    for (int index = int(d->_windows.size()) - 1; index >= 0; --index)
    {
        const ShadDockFloatingWindow* window = d->_windows.at(index);
        if (window == nullptr)
        {
            continue;
        }
        const QRect geometry = window->geometry();
        QJsonObject item;
        item.insert(QStringLiteral("geometry"), QJsonArray{geometry.x(), geometry.y(), geometry.width(), geometry.height()});
        item.insert(QStringLiteral("root"), d->saveNode(window->container()->rootSplitter()));
        floating.append(item);
    }
    root.insert(QStringLiteral("floating"), floating);
    QJsonArray closed;
    for (const ShadDockPanel* panel : d->_panels)
    {
        if (!panel->isOpen())
        {
            closed.append(panel->getId());
        }
    }
    root.insert(QStringLiteral("closed"), closed);
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

bool ShadDockManager::restoreState(const QByteArray& state)
{
    Q_D(ShadDockManager);
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(state, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
    {
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("version")).toInt() != ShadDockManagerPrivate::kStateVersion || !root.value(QStringLiteral("main")).isObject())
    {
        return false;
    }
    d->finishDrag(true);
    for (ShadDockPanel* panel : d->_panels)
    {
        d->takeFromGroup(panel);
        panel->setParent(this);
        panel->hide();
    }
    const QList<QPointer<ShadDockFloatingWindow>> windows = d->_windows;
    d->_windows.clear();
    for (const QPointer<ShadDockFloatingWindow>& window : windows)
    {
        if (!window.isNull())
        {
            window->container()->clear();
            window->hide();
            window->deleteLater();
        }
    }
    d->_mainContainer->clear();
    d->_centralGroup = nullptr;
    d->_activeGroup = nullptr;
    d->_lastDocumentGroup = nullptr;

    QSet<const ShadDockPanel*> placed;
    d->restoreRoot(d->_mainContainer, root.value(QStringLiteral("main")).toObject(), true, &placed);
    if (d->_centralGroup.isNull())
    {
        ShadDockGroup* central = d->createGroup(ShadDockGroup::Kind::Document);
        central->setCentral(true);
        d->_centralGroup = central;
        d->_mainContainer->insertNode(central, nullptr, ShadDockType::RightArea, 0);
    }
    for (const QJsonValue& value : root.value(QStringLiteral("floating")).toArray())
    {
        const QJsonObject item = value.toObject();
        ShadDockFloatingWindow* window = d->createFloatingWindow();
        d->restoreRoot(window->container(), item.value(QStringLiteral("root")).toObject(), false, &placed);
        if (window->container()->isEmpty())
        {
            d->_windows.removeAll(window);
            delete window;
            continue;
        }
        const QJsonArray geometry = item.value(QStringLiteral("geometry")).toArray();
        if (geometry.size() == 4)
        {
            window->setGeometry(QRect(geometry.at(0).toInt(), geometry.at(1).toInt(), geometry.at(2).toInt(), geometry.at(3).toInt()));
        }
        window->setVisible(true);
        d->updateWindow(window);
    }
    QSet<QString> closedIds;
    for (const QJsonValue& value : root.value(QStringLiteral("closed")).toArray())
    {
        closedIds.insert(value.toString());
    }
    // 未记录的面板放回默认位置
    const QList<ShadDockPanel*> panelList = d->_panels;
    for (ShadDockPanel* panel : panelList)
    {
        if (placed.contains(panel))
        {
            continue;
        }
        if (closedIds.contains(panel->getId()))
        {
            d->setPanelOpen(panel, false);
        }
        else if (panel->isDocument())
        {
            d->insertIntoGroup(d->documentGroup(), panel, -1);
        }
        else
        {
            d->placePanel(panel, panel->d_func()->_defaultArea, nullptr);
        }
    }
    d->updateCurrentPanel();
    return true;
}
