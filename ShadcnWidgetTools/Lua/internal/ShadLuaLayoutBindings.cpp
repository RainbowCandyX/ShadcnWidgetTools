#include <QAbstractButton>
#include <QAbstractScrollArea>
#include <QAction>
#include <QByteArray>
#include <QHBoxLayout>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QPixmap>
#include <QRect>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QString>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadAnchor.h"
#include "ShadAvatar.h"
#include "ShadBackTop.h"
#include "ShadCarousel.h"
#include "ShadCarouselDots.h"
#include "ShadCarouselThumbnails.h"
#include "ShadCollapsible.h"
#include "ShadDef.h"
#include "ShadDockManager.h"
#include "ShadDockPanel.h"
#include "ShadDropdownMenu.h"
#include "ShadExpander.h"
#include "ShadFlowLayout.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadLuaBinder.h"
#include "ShadLuaBindings.h"
#include "ShadLuaInclude.h"
#include "ShadLuaObject.h"
#include "ShadLuaStack.h"
#include "ShadResizable.h"
#include "ShadScrollArea.h"
#include "ShadScrollBar.h"
#include "ShadSidebar.h"
#include "ShadSidebarGroup.h"
#include "ShadSidebarLayout.h"
#include "ShadSidebarMenuButton.h"
#include "ShadSidebarTrigger.h"
#include "ShadStackedWidget.h"
#include "ShadStatusBar.h"
#include "ShadSteps.h"
#include "ShadTabView.h"
#include "ShadToolbar.h"
#include "ShadWindow.h"

namespace
{
    int optionalInt(lua_State* L, int index, int defaultValue)
    {
        qint64 value = 0;
        return ShadLuaStack::getInteger(L, index, value) ? int(value) : defaultValue;
    }

    QString optionalString(lua_State* L, int index)
    {
        QString value;
        ShadLuaStack::getString(L, index, value);
        return value;
    }

    ShadIconType::IconName optionalIcon(lua_State* L, int index)
    {
        return static_cast<ShadIconType::IconName>(optionalInt(L, index, ShadIconType::None));
    }

    bool optionalBool(lua_State* L, int index, bool defaultValue)
    {
        return lua_isnoneornil(L, index) ? defaultValue : lua_toboolean(L, index) != 0;
    }

    template <class T>
    void pushObjects(lua_State* L, const QList<T*>& objects)
    {
        lua_createtable(L, int(objects.size()), 0);
        for (int position = 0; position < objects.size(); ++position)
        {
            ShadLuaObject::push(L, objects.at(position));
            lua_rawseti(L, -2, position + 1);
        }
    }

    // 滚动区域、可调整大小的面板组与页面堆叠
    void registerContainers()
    {
        ShadLuaClass<QScrollArea> scrollAreaBase;
        scrollAreaBase.method("setWidget", +[](QScrollArea* self, QWidget* widget) { self->setWidget(widget); });
        scrollAreaBase.method("widget", +[](QScrollArea* self) { return self->widget(); });

        ShadLuaClass<ShadScrollArea> scrollArea("ScrollArea");
        scrollArea.constructor(&ShadLuaBind::newWithParent<ShadScrollArea>);
        scrollArea.method("getOverlayScrollBar", &ShadScrollArea::getOverlayScrollBar);

        // new(orientation, parent) 或 new(parent)
        ShadLuaClass<ShadScrollBar>("ScrollBar").constructor(&ShadLuaBind::newWithEnum<ShadScrollBar, Qt::Orientation>);

        ShadLuaClass<QSplitter> splitter;
        splitter.method("addWidget", +[](QSplitter* self, QWidget* widget)
        {
            if (widget != nullptr)
            {
                self->addWidget(widget);
            }
        });
        splitter.method("insertWidget", +[](QSplitter* self, int index, QWidget* widget)
        {
            if (widget != nullptr)
            {
                self->insertWidget(index, widget);
            }
        });
        splitter.method("count", +[](QSplitter* self) { return self->count(); });
        splitter.method("widget", +[](QSplitter* self, int index) { return self->widget(index); });
        splitter.method("indexOf", +[](QSplitter* self, QWidget* widget) { return self->indexOf(widget); });
        splitter.method("setSizes", +[](QSplitter* self, QList<int> sizes) { self->setSizes(sizes); });
        splitter.method("sizes", +[](QSplitter* self) { return self->sizes(); });
        splitter.method("setStretchFactor", +[](QSplitter* self, int index, int stretch) { self->setStretchFactor(index, stretch); });
        splitter.method("setCollapsible", +[](QSplitter* self, int index, bool collapsible) { self->setCollapsible(index, collapsible); });

        // new(orientation, parent) 或 new(parent)
        ShadLuaClass<ShadResizable>("Resizable").constructor(&ShadLuaBind::newWithEnum<ShadResizable, Qt::Orientation>);

        // addWidget 与 insertWidget 在 page 不是组件时返回 -1
        ShadLuaClass<QStackedWidget> stackedWidget;
        stackedWidget.method("addWidget", +[](QStackedWidget* self, QWidget* page) { return page != nullptr ? self->addWidget(page) : -1; });
        stackedWidget.method("insertWidget", +[](QStackedWidget* self, int index, QWidget* page) { return page != nullptr ? self->insertWidget(index, page) : -1; });
        stackedWidget.method("removeWidget", +[](QStackedWidget* self, QWidget* page)
        {
            if (page != nullptr)
            {
                self->removeWidget(page);
            }
        });
        stackedWidget.method("count", +[](QStackedWidget* self) { return self->count(); });
        stackedWidget.method("widget", +[](QStackedWidget* self, int index) { return self->widget(index); });
        stackedWidget.method("indexOf", +[](QStackedWidget* self, QWidget* page) { return self->indexOf(page); });
        stackedWidget.method("currentWidget", +[](QStackedWidget* self) { return self->currentWidget(); });

        ShadLuaClass<ShadStackedWidget>("StackedWidget").constructor(&ShadLuaBind::newWithParent<ShadStackedWidget>);

        ShadLuaClass<ShadCollapsible> collapsible("Collapsible");
        collapsible.constructor(&ShadLuaBind::newWithParent<ShadCollapsible>);
        collapsible.method("headerLayout", &ShadCollapsible::headerLayout);
        collapsible.method("contentLayout", &ShadCollapsible::contentLayout);
        collapsible.method("setTrigger", &ShadCollapsible::setTrigger);
        collapsible.method("getTrigger", &ShadCollapsible::getTrigger);

        ShadLuaClass<ShadExpander> expander("Expander");
        expander.constructor(&ShadLuaBind::newWithParent<ShadExpander>);
        expander.method("headerLayout", &ShadExpander::headerLayout);
        expander.method("addItem", &ShadExpander::addItem);
        expander.method("removeItem", &ShadExpander::removeItem);
        expander.rawMethod("getItems", [](lua_State* L, ShadExpander* self) -> int
        {
            pushObjects(L, self->getItems());
            return 1;
        });

        // 流式布局：new(parent)
        ShadLuaClass<ShadFlowLayout> flowLayout("FlowLayout");
        flowLayout.constructor(&ShadLuaBind::newWithParent<ShadFlowLayout>);
        flowLayout.method("setHorizontalSpacing", &ShadFlowLayout::setHorizontalSpacing);
        flowLayout.method("getHorizontalSpacing", &ShadFlowLayout::getHorizontalSpacing);
        flowLayout.method("setVerticalSpacing", &ShadFlowLayout::setVerticalSpacing);
        flowLayout.method("getVerticalSpacing", &ShadFlowLayout::getVerticalSpacing);

        ShadLuaClass<ShadCarousel> carousel("Carousel");
        carousel.constructor(&ShadLuaBind::newWithParent<ShadCarousel>);
        carousel.method("addItem", &ShadCarousel::addItem);
        carousel.method("insertItem", &ShadCarousel::insertItem);
        carousel.method("removeItem", &ShadCarousel::removeItem);
        carousel.method("count", &ShadCarousel::count);
        carousel.method("item", &ShadCarousel::item);
        carousel.method("indexOf", &ShadCarousel::indexOf);
        carousel.method("scrollPrev", &ShadCarousel::scrollPrev);
        carousel.method("scrollNext", &ShadCarousel::scrollNext);
        // scrollTo(index, animated)
        carousel.rawMethod("scrollTo", [](lua_State* L, ShadCarousel* self) -> int
        {
            self->scrollTo(optionalInt(L, 2, 0), optionalBool(L, 3, true));
            return 0;
        });
        carousel.method("canScrollPrev", &ShadCarousel::canScrollPrev);
        carousel.method("canScrollNext", &ShadCarousel::canScrollNext);
        carousel.method("snapCount", &ShadCarousel::snapCount);

        // new(carousel, parent)
        ShadLuaClass<ShadCarouselDots> carouselDots("CarouselDots");
        carouselDots.constructor([](lua_State* L) -> ShadCarouselDots* { return new ShadCarouselDots(ShadLuaObject::get<ShadCarousel>(L, 1), ShadLuaObject::get<QWidget>(L, 2)); });
        carouselDots.method("setCarousel", &ShadCarouselDots::setCarousel);
        carouselDots.method("getCarousel", &ShadCarouselDots::getCarousel);

        // new(carousel, parent)
        ShadLuaClass<ShadCarouselThumbnails> carouselThumbnails("CarouselThumbnails");
        carouselThumbnails.constructor([](lua_State* L) -> ShadCarouselThumbnails* { return new ShadCarouselThumbnails(ShadLuaObject::get<ShadCarousel>(L, 1), ShadLuaObject::get<QWidget>(L, 2)); });
        carouselThumbnails.method("setCarousel", &ShadCarouselThumbnails::setCarousel);
        carouselThumbnails.method("getCarousel", &ShadCarouselThumbnails::getCarousel);
        // addThumbnail(path)
        carouselThumbnails.rawMethod("addThumbnail", [](lua_State* L, ShadCarouselThumbnails* self) -> int
        {
            lua_pushinteger(L, self->addThumbnail(QPixmap(optionalString(L, 2))));
            return 1;
        });
        // setThumbnail(index, path)
        carouselThumbnails.rawMethod("setThumbnail", [](lua_State* L, ShadCarouselThumbnails* self) -> int
        {
            self->setThumbnail(optionalInt(L, 2, -1), QPixmap(optionalString(L, 3)));
            return 0;
        });
        carouselThumbnails.method("removeThumbnail", &ShadCarouselThumbnails::removeThumbnail);
        carouselThumbnails.method("clear", &ShadCarouselThumbnails::clear);
        carouselThumbnails.method("count", &ShadCarouselThumbnails::count);
    }

    // 标签视图
    void registerTabView()
    {
        ShadLuaClass<ShadTabView> tabView("TabView");
        tabView.constructor(&ShadLuaBind::newWithParent<ShadTabView>);
        // addTab(text, page) 或 addTab(icon, text, page)，page 不是组件时返回 -1
        tabView.rawMethod("addTab", [](lua_State* L, ShadTabView* self) -> int
        {
            if (lua_type(L, 2) == LUA_TNUMBER)
            {
                lua_pushinteger(L, self->addTab(optionalIcon(L, 2), optionalString(L, 3), ShadLuaObject::get<QWidget>(L, 4)));
                return 1;
            }
            lua_pushinteger(L, self->addTab(optionalString(L, 2), ShadLuaObject::get<QWidget>(L, 3)));
            return 1;
        });
        tabView.method("insertTab", &ShadTabView::insertTab);
        tabView.method("removeTab", &ShadTabView::removeTab);
        tabView.method("moveTab", &ShadTabView::moveTab);
        tabView.method("count", &ShadTabView::count);
        tabView.method("widget", &ShadTabView::widget);
        tabView.method("indexOf", &ShadTabView::indexOf);
        tabView.method("setTabText", &ShadTabView::setTabText);
        tabView.method("tabText", &ShadTabView::tabText);
        tabView.method("setTabIcon", &ShadTabView::setTabIcon);
        tabView.method("tabIcon", &ShadTabView::tabIcon);
        tabView.method("setTabFlags", +[](ShadTabView* self, int index, int flags) { self->setTabFlags(index, ShadTabViewType::TabFlags::fromInt(flags)); });
        tabView.method("tabFlags", +[](ShadTabView* self, int index) { return self->tabFlags(index).toInt(); });
        // setTabFlag(index, flag, on)
        tabView.rawMethod("setTabFlag", [](lua_State* L, ShadTabView* self) -> int
        {
            self->setTabFlag(optionalInt(L, 2, -1), static_cast<ShadTabViewType::TabFlag>(optionalInt(L, 3, 0)), optionalBool(L, 4, true));
            return 0;
        });
        tabView.method("setTabToolTip", &ShadTabView::setTabToolTip);
        tabView.method("tabToolTip", &ShadTabView::tabToolTip);
        tabView.method("setTabData", &ShadTabView::setTabData);
        tabView.method("tabData", &ShadTabView::tabData);
        tabView.method("tabBar", &ShadTabView::tabBar);
    }

    // 停靠布局
    void registerDock()
    {
        // new(id, parent) 或 new(id, icon, title, widget, parent)
        ShadLuaClass<ShadDockPanel> dockPanel("DockPanel");
        dockPanel.constructor([](lua_State* L) -> ShadDockPanel*
        {
            QString id;
            if (!ShadLuaStack::getString(L, 1, id))
            {
                return nullptr;
            }
            if (lua_type(L, 2) == LUA_TNUMBER)
            {
                return new ShadDockPanel(id, optionalIcon(L, 2), optionalString(L, 3), ShadLuaObject::get<QWidget>(L, 4), ShadLuaObject::get<QWidget>(L, 5));
            }
            return new ShadDockPanel(id, ShadLuaObject::get<QWidget>(L, 2));
        });
        dockPanel.method("getId", &ShadDockPanel::getId);
        dockPanel.method("setWidget", &ShadDockPanel::setWidget);
        dockPanel.method("getWidget", &ShadDockPanel::getWidget);
        dockPanel.method("setIcon", &ShadDockPanel::setIcon);
        dockPanel.method("getIcon", &ShadDockPanel::getIcon);
        dockPanel.method("setTitleToolTip", &ShadDockPanel::setTitleToolTip);
        dockPanel.method("getTitleToolTip", &ShadDockPanel::getTitleToolTip);
        dockPanel.method("setPanelFlags", +[](ShadDockPanel* self, int flags) { self->setPanelFlags(ShadDockType::PanelFlags::fromInt(flags)); });
        dockPanel.method("getPanelFlags", +[](ShadDockPanel* self) { return self->getPanelFlags().toInt(); });
        // setPanelFlag(flag, on)
        dockPanel.rawMethod("setPanelFlag", [](lua_State* L, ShadDockPanel* self) -> int
        {
            self->setPanelFlag(static_cast<ShadDockType::PanelFlag>(optionalInt(L, 2, 0)), optionalBool(L, 3, true));
            return 0;
        });
        dockPanel.method("getDockManager", &ShadDockPanel::getDockManager);
        dockPanel.method("isDocument", &ShadDockPanel::isDocument);
        dockPanel.method("isFloating", &ShadDockPanel::isFloating);
        dockPanel.method("isOpen", &ShadDockPanel::isOpen);
        // toggleViewAction() 返回 QAction
        dockPanel.method("toggleViewAction", &ShadDockPanel::toggleViewAction);

        ShadLuaClass<ShadDockManager> dockManager("DockManager");
        dockManager.constructor(&ShadLuaBind::newWithParent<ShadDockManager>);
        // addPanel(panel, area, relativeTo)
        dockManager.rawMethod("addPanel", [](lua_State* L, ShadDockManager* self) -> int
        {
            ShadDockPanel* panel = ShadLuaObject::get<ShadDockPanel>(L, 2);
            if (panel == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->addPanel(panel, static_cast<ShadDockType::DockArea>(optionalInt(L, 3, ShadDockType::CenterArea)), ShadLuaObject::get<ShadDockPanel>(L, 4));
            return 0;
        });
        dockManager.method("addDocument", &ShadDockManager::addDocument);
        dockManager.method("tabifyPanel", &ShadDockManager::tabifyPanel);
        // floatPanel(panel, {x, y, 宽, 高})
        dockManager.rawMethod("floatPanel", [](lua_State* L, ShadDockManager* self) -> int
        {
            ShadDockPanel* panel = ShadLuaObject::get<ShadDockPanel>(L, 2);
            if (panel == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->floatPanel(panel, ShadLuaStack::getVariant(L, 3, QMetaType::fromType<QRect>()).toRect());
            return 0;
        });
        dockManager.method("removePanel", &ShadDockManager::removePanel);
        dockManager.method("closePanel", &ShadDockManager::closePanel);
        dockManager.method("showPanel", &ShadDockManager::showPanel);
        dockManager.method("setCurrentPanel", &ShadDockManager::setCurrentPanel);
        dockManager.method("getCurrentPanel", &ShadDockManager::getCurrentPanel);
        dockManager.rawMethod("panels", [](lua_State* L, ShadDockManager* self) -> int
        {
            pushObjects(L, self->panels());
            return 1;
        });
        dockManager.method("findPanel", &ShadDockManager::findPanel);
        // saveState() 返回 Base64 文本
        dockManager.method("saveState", +[](ShadDockManager* self) { return QString::fromLatin1(self->saveState().toBase64()); });
        dockManager.method("restoreState", +[](ShadDockManager* self, QString state) { return self->restoreState(QByteArray::fromBase64(state.toLatin1())); });
    }

    // 侧栏
    void registerSidebar()
    {
        ShadLuaClass<ShadSidebar> sidebar("Sidebar");
        sidebar.constructor(&ShadLuaBind::newWithParent<ShadSidebar>);
        sidebar.method("toggle", &ShadSidebar::toggle);
        sidebar.method("isIconCollapsed", &ShadSidebar::isIconCollapsed);
        sidebar.method("setExpandedWidthRange", &ShadSidebar::setExpandedWidthRange);
        sidebar.method("getMinimumExpandedWidth", &ShadSidebar::getMinimumExpandedWidth);
        sidebar.method("getMaximumExpandedWidth", &ShadSidebar::getMaximumExpandedWidth);
        sidebar.method("headerLayout", &ShadSidebar::headerLayout);
        sidebar.method("footerLayout", &ShadSidebar::footerLayout);
        sidebar.method("contentLayout", &ShadSidebar::contentLayout);
        // addGroup(label)
        sidebar.rawMethod("addGroup", [](lua_State* L, ShadSidebar* self) -> int
        {
            ShadLuaObject::push(L, self->addGroup(optionalString(L, 2)));
            return 1;
        });
        sidebar.method("setActiveButton", &ShadSidebar::setActiveButton);
        sidebar.method("getActiveButton", &ShadSidebar::getActiveButton);

        // new(label, parent) 或 new(parent)
        ShadLuaClass<ShadSidebarGroup> sidebarGroup("SidebarGroup");
        sidebarGroup.constructor(&ShadLuaBind::newWithText<ShadSidebarGroup>);
        sidebarGroup.method("setActionIcon", &ShadSidebarGroup::setActionIcon);
        sidebarGroup.method("getActionIcon", &ShadSidebarGroup::getActionIcon);
        sidebarGroup.rawMethod("getActionRect", [](lua_State* L, ShadSidebarGroup* self) -> int
        {
            ShadLuaStack::pushVariant(L, QVariant(self->getActionRect()));
            return 1;
        });
        // addItem(icon, text)
        sidebarGroup.method("addItem", &ShadSidebarGroup::addItem);
        sidebarGroup.method("addWidget", &ShadSidebarGroup::addWidget);
        sidebarGroup.method("menuLayout", &ShadSidebarGroup::menuLayout);

        ShadLuaClass<ShadSidebarLayout> sidebarLayout("SidebarLayout");
        sidebarLayout.constructor(&ShadLuaBind::newWithParent<ShadSidebarLayout>);
        sidebarLayout.method("setSidebar", &ShadSidebarLayout::setSidebar);
        sidebarLayout.method("getSidebar", &ShadSidebarLayout::getSidebar);
        sidebarLayout.method("setContentWidget", &ShadSidebarLayout::setContentWidget);
        sidebarLayout.method("getContentWidget", &ShadSidebarLayout::getContentWidget);

        // new(icon, text, parent)、new(text, parent) 或 new(parent)
        ShadLuaClass<ShadSidebarMenuButton> menuButton("SidebarMenuButton");
        menuButton.constructor([](lua_State* L) -> ShadSidebarMenuButton*
        {
            if (lua_type(L, 1) == LUA_TNUMBER)
            {
                return new ShadSidebarMenuButton(optionalIcon(L, 1), optionalString(L, 2), ShadLuaObject::get<QWidget>(L, 3));
            }
            if (lua_type(L, 1) == LUA_TSTRING)
            {
                return new ShadSidebarMenuButton(ShadIconType::None, optionalString(L, 1), ShadLuaObject::get<QWidget>(L, 2));
            }
            return new ShadSidebarMenuButton(ShadLuaObject::get<QWidget>(L, 1));
        });
        menuButton.method("setShadIcon", &ShadSidebarMenuButton::setShadIcon);
        menuButton.method("getShadIcon", &ShadSidebarMenuButton::getShadIcon);
        menuButton.method("setAvatar", &ShadSidebarMenuButton::setAvatar);
        menuButton.method("getAvatar", &ShadSidebarMenuButton::getAvatar);
        menuButton.method("setActionIcon", &ShadSidebarMenuButton::setActionIcon);
        menuButton.method("getActionIcon", &ShadSidebarMenuButton::getActionIcon);
        menuButton.method("setActionAlwaysVisible", &ShadSidebarMenuButton::setActionAlwaysVisible);
        menuButton.method("isActionAlwaysVisible", &ShadSidebarMenuButton::isActionAlwaysVisible);
        menuButton.rawMethod("getActionRect", [](lua_State* L, ShadSidebarMenuButton* self) -> int
        {
            ShadLuaStack::pushVariant(L, QVariant(self->getActionRect()));
            return 1;
        });
        // addSubItem(text, icon)，按钮需要先加入布局
        menuButton.rawMethod("addSubItem", [](lua_State* L, ShadSidebarMenuButton* self) -> int
        {
            ShadLuaObject::push(L, self->addSubItem(optionalString(L, 2), optionalIcon(L, 3)));
            return 1;
        });
        menuButton.method("hasSubItems", &ShadSidebarMenuButton::hasSubItems);
        menuButton.method("isSubItem", &ShadSidebarMenuButton::isSubItem);

        // new(sidebar, parent) 或 new(parent)
        ShadLuaClass<ShadSidebarTrigger> sidebarTrigger("SidebarTrigger");
        sidebarTrigger.constructor([](lua_State* L) -> ShadSidebarTrigger*
        {
            ShadSidebar* sidebar = ShadLuaObject::get<ShadSidebar>(L, 1);
            if (sidebar != nullptr)
            {
                return new ShadSidebarTrigger(sidebar, ShadLuaObject::get<QWidget>(L, 2));
            }
            return new ShadSidebarTrigger(ShadLuaObject::get<QWidget>(L, 1));
        });
        sidebarTrigger.method("setSidebar", &ShadSidebarTrigger::setSidebar);
        sidebarTrigger.method("getSidebar", &ShadSidebarTrigger::getSidebar);
    }

    // 工具栏、状态栏与带标题栏自定义区域的窗口
    void registerWindowParts()
    {
        // new(orientation, parent) 或 new(parent)
        ShadLuaClass<ShadToolbar> toolbar("Toolbar");
        toolbar.constructor(&ShadLuaBind::newWithEnum<ShadToolbar, Qt::Orientation>);
        // addAction(icon, text) 或 addAction(action, icon)
        toolbar.rawMethod("addAction", [](lua_State* L, ShadToolbar* self) -> int
        {
            QAction* action = ShadLuaObject::get<QAction>(L, 2);
            if (action != nullptr)
            {
                self->addAction(action, optionalIcon(L, 3));
                return 0;
            }
            ShadLuaObject::push(L, self->addAction(optionalIcon(L, 2), optionalString(L, 3)));
            return 1;
        });
        // addMenu(icon, text)；setMenuTitle(menu, title)
        toolbar.method("addMenu", &ShadToolbar::addMenu);
        toolbar.method("setMenuTitle", &ShadToolbar::setMenuTitle);
        toolbar.method("addWidget", &ShadToolbar::addWidget);
        toolbar.method("addSeparator", &ShadToolbar::addSeparator);
        toolbar.method("addStretch", &ShadToolbar::addStretch);
        toolbar.method("removeAction", &ShadToolbar::removeAction);
        toolbar.method("clear", &ShadToolbar::clear);

        ShadLuaClass<ShadStatusBar> statusBar("StatusBar");
        statusBar.constructor(&ShadLuaBind::newWithParent<ShadStatusBar>);
        // addWidget(widget, stretch) 与 addPermanentWidget(widget, stretch)
        statusBar.rawMethod("addWidget", [](lua_State* L, ShadStatusBar* self) -> int
        {
            self->addWidget(ShadLuaObject::get<QWidget>(L, 2), optionalInt(L, 3, 0));
            return 0;
        });
        statusBar.rawMethod("addPermanentWidget", [](lua_State* L, ShadStatusBar* self) -> int
        {
            self->addPermanentWidget(ShadLuaObject::get<QWidget>(L, 2), optionalInt(L, 3, 0));
            return 0;
        });
        statusBar.method("addText", &ShadStatusBar::addText);
        statusBar.method("removeWidget", &ShadStatusBar::removeWidget);
        // showMessage(message, timeout)
        statusBar.rawMethod("showMessage", [](lua_State* L, ShadStatusBar* self) -> int
        {
            self->showMessage(optionalString(L, 2), optionalInt(L, 3, 0));
            return 0;
        });
        statusBar.method("clearMessage", &ShadStatusBar::clearMessage);
        statusBar.method("currentMessage", &ShadStatusBar::currentMessage);

        // ShadWindow 在 Lua 中叫 MainWindow，shad.Window 已用于 ShadWidget
        ShadLuaClass<ShadWindow> mainWindow("MainWindow");
        mainWindow.constructor([](lua_State* L) -> ShadWindow*
        {
            ShadWindow* window = new ShadWindow(ShadLuaObject::get<QWidget>(L, 1));
            window->setAttribute(Qt::WA_QuitOnClose, false);
            return window;
        });
        mainWindow.method("setCentralWidget", &ShadWindow::setCentralWidget);
        mainWindow.method("getCentralWidget", &ShadWindow::getCentralWidget);
        mainWindow.method("setTitleBarWidget", &ShadWindow::setTitleBarWidget);
        mainWindow.method("getTitleBarWidget", &ShadWindow::getTitleBarWidget);
        mainWindow.method("setButtonFlags", +[](ShadWindow* self, int flags) { self->setButtonFlags(ShadWindowType::ButtonFlags::fromInt(flags)); });
        mainWindow.method("getButtonFlags", +[](ShadWindow* self) { return self->getButtonFlags().toInt(); });
    }

    void registerNavigation()
    {
        ShadLuaClass<ShadAnchor> anchor("Anchor");
        anchor.constructor(&ShadLuaBind::newWithParent<ShadAnchor>);
        anchor.method("setScrollArea", &ShadAnchor::setScrollArea);
        anchor.method("getScrollArea", &ShadAnchor::getScrollArea);
        anchor.rawMethod("addLink", [](lua_State* L, ShadAnchor* self) -> int
        {
            lua_pushinteger(L, self->addLink(optionalString(L, 2), ShadLuaObject::get<QWidget>(L, 3), optionalInt(L, 4, 0)));
            return 1;
        });
        anchor.method("setLinkTitle", &ShadAnchor::setLinkTitle);
        anchor.method("getLinkTitle", &ShadAnchor::getLinkTitle);
        anchor.method("getLinkTarget", &ShadAnchor::getLinkTarget);
        anchor.method("count", &ShadAnchor::count);
        anchor.method("clear", &ShadAnchor::clear);

        ShadLuaClass<ShadBackTop> backTop("BackTop");
        backTop.constructor([](lua_State* L) -> ShadBackTop* { return new ShadBackTop(ShadLuaObject::get<QAbstractScrollArea>(L, 1)); });
        backTop.method("setTarget", &ShadBackTop::setTarget);
        backTop.method("getTarget", &ShadBackTop::getTarget);

        ShadLuaClass<ShadSteps> steps("Steps");
        steps.constructor(&ShadLuaBind::newWithParent<ShadSteps>);
        steps.rawMethod("addStep", [](lua_State* L, ShadSteps* self) -> int
        {
            lua_pushinteger(L, self->addStep(optionalString(L, 2), optionalString(L, 3)));
            return 1;
        });
        steps.method("setStepTitle", &ShadSteps::setStepTitle);
        steps.method("getStepTitle", &ShadSteps::getStepTitle);
        steps.method("setStepDescription", &ShadSteps::setStepDescription);
        steps.method("getStepDescription", &ShadSteps::getStepDescription);
        steps.method("setStepIcon", &ShadSteps::setStepIcon);
        steps.method("getStepIcon", &ShadSteps::getStepIcon);
        steps.method("setStepDisabled", &ShadSteps::setStepDisabled);
        steps.method("isStepDisabled", &ShadSteps::isStepDisabled);
        steps.method("getStepStatus", &ShadSteps::getStepStatus);
        steps.method("count", &ShadSteps::count);
        steps.method("clear", &ShadSteps::clear);
    }
} // namespace

namespace ShadLuaBindings
{
    void registerLayoutComponents()
    {
        registerContainers();
        registerTabView();
        registerDock();
        registerSidebar();
        registerWindowParts();
        registerNavigation();
    }
} // namespace ShadLuaBindings
