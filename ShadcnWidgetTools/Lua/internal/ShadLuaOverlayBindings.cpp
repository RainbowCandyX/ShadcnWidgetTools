#include <QAbstractButton>
#include <QAction>
#include <QActionGroup>
#include <QColor>
#include <QKeySequence>
#include <QLineEdit>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadAlertDialog.h"
#include "ShadButton.h"
#include "ShadColorDialog.h"
#include "ShadColorPicker.h"
#include "ShadCommand.h"
#include "ShadCommandDialog.h"
#include "ShadContextMenu.h"
#include "ShadDef.h"
#include "ShadDrawer.h"
#include "ShadDropdownMenu.h"
#include "ShadHoverCard.h"
#include "ShadIconType.h"
#include "ShadInputDialog.h"
#include "ShadLoadingOverlay.h"
#include "ShadLuaBinder.h"
#include "ShadLuaBindings.h"
#include "ShadLuaEngine.h"
#include "ShadLuaInclude.h"
#include "ShadLuaObject.h"
#include "ShadLuaStack.h"
#include "ShadMenubar.h"
#include "ShadNavigationMenu.h"
#include "ShadNavigationMenuLink.h"
#include "ShadNotification.h"
#include "ShadPopconfirm.h"
#include "ShadPopover.h"
#include "ShadTooltip.h"

namespace
{
    int optionalInt(lua_State* L, int index, int defaultValue)
    {
        qint64 value = 0;
        return ShadLuaStack::getInteger(L, index, value) ? int(value) : defaultValue;
    }

    double optionalNumber(lua_State* L, int index, double defaultValue)
    {
        double value = 0.0;
        return ShadLuaStack::getNumber(L, index, value) ? value : defaultValue;
    }

    QString optionalString(lua_State* L, int index)
    {
        QString value;
        ShadLuaStack::getString(L, index, value);
        return value;
    }

    bool optionalBool(lua_State* L, int index, bool defaultValue)
    {
        return lua_isnoneornil(L, index) ? defaultValue : lua_toboolean(L, index) != 0;
    }

    ShadIconType::IconName optionalIcon(lua_State* L, int index)
    {
        return static_cast<ShadIconType::IconName>(optionalInt(L, index, ShadIconType::None));
    }

    // 弹出方向与对齐方式
    ShadPlacementType::Side optionalSide(lua_State* L, int index, ShadPlacementType::Side defaultValue)
    {
        return static_cast<ShadPlacementType::Side>(optionalInt(L, index, defaultValue));
    }

    ShadPlacementType::Align optionalAlign(lua_State* L, int index)
    {
        return static_cast<ShadPlacementType::Align>(optionalInt(L, index, ShadPlacementType::Center));
    }

    QKeySequence optionalKeySequence(lua_State* L, int index)
    {
        return QKeySequence(optionalString(L, index), QKeySequence::PortableText);
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

    // index 必须是正的栈位置
    bool readNumbers(lua_State* L, int index, QList<qreal>& values)
    {
        if (lua_type(L, index) != LUA_TTABLE)
        {
            return false;
        }
        const int length = ShadLuaStack::tableLength(L, index);
        values.clear();
        values.reserve(length);
        for (int position = 1; position <= length; ++position)
        {
            double number = 0.0;
            lua_rawgeti(L, index, position);
            const bool isNumber = ShadLuaStack::getNumber(L, -1, number);
            lua_pop(L, 1);
            if (!isNumber)
            {
                return false;
            }
            values.append(number);
        }
        return true;
    }

    // 对话框；以 shad.<名称>.<函数> 调用的静态函数会阻塞到对话框关闭
    void registerDialogs()
    {
        // new(title, description, parent) 或 new(parent)
        ShadLuaClass<ShadAlertDialog> alertDialog("AlertDialog");
        alertDialog.constructor(&ShadLuaBind::newWithTitle<ShadAlertDialog>);
        alertDialog.method("setMediaIcon", &ShadAlertDialog::setMediaIcon);
        alertDialog.method("getMediaIcon", &ShadAlertDialog::getMediaIcon);
        alertDialog.method("setMediaImagePath", +[](ShadAlertDialog* self, QString path) { self->setMediaPixmap(QPixmap(path)); });
        alertDialog.method("actionButton", &ShadAlertDialog::actionButton);
        alertDialog.method("cancelButton", &ShadAlertDialog::cancelButton);
        // shad.AlertDialog.confirm(parent, title, description, actionText, cancelText, actionVariant)
        alertDialog.staticMethod("confirm", [](lua_State* L) -> int
        {
            const bool isConfirmed = ShadAlertDialog::confirm(ShadLuaObject::get<QWidget>(L, 1), optionalString(L, 2), optionalString(L, 3), optionalString(L, 4), optionalString(L, 5), static_cast<ShadButtonType::Variant>(optionalInt(L, 6, ShadButtonType::Default)));
            lua_pushboolean(L, isConfirmed ? 1 : 0);
            return 1;
        });

        ShadLuaClass<ShadInputDialog> inputDialog("InputDialog");
        inputDialog.constructor(&ShadLuaBind::newWithParent<ShadInputDialog>);
        inputDialog.method("setIntRange", &ShadInputDialog::setIntRange);
        inputDialog.method("getIntMinimum", &ShadInputDialog::getIntMinimum);
        inputDialog.method("getIntMaximum", &ShadInputDialog::getIntMaximum);
        inputDialog.method("setIntStep", &ShadInputDialog::setIntStep);
        inputDialog.method("getIntStep", &ShadInputDialog::getIntStep);
        inputDialog.method("setDoubleRange", &ShadInputDialog::setDoubleRange);
        inputDialog.method("getDoubleMinimum", &ShadInputDialog::getDoubleMinimum);
        inputDialog.method("getDoubleMaximum", &ShadInputDialog::getDoubleMaximum);
        inputDialog.method("setDoubleDecimals", &ShadInputDialog::setDoubleDecimals);
        inputDialog.method("getDoubleDecimals", &ShadInputDialog::getDoubleDecimals);
        inputDialog.method("setDoubleStep", &ShadInputDialog::setDoubleStep);
        inputDialog.method("getDoubleStep", &ShadInputDialog::getDoubleStep);
        // 以下四个静态函数在取消时返回 nil
        // shad.InputDialog.getText(parent, title, description, label, text, echoMode)
        inputDialog.staticMethod("getText", [](lua_State* L) -> int
        {
            bool isAccepted = false;
            const QString text = ShadInputDialog::getText(ShadLuaObject::get<QWidget>(L, 1), optionalString(L, 2), optionalString(L, 3), optionalString(L, 4), optionalString(L, 5), &isAccepted, static_cast<QLineEdit::EchoMode>(optionalInt(L, 6, QLineEdit::Normal)));
            if (!isAccepted)
            {
                lua_pushnil(L);
                return 1;
            }
            ShadLuaStack::pushString(L, text);
            return 1;
        });
        // shad.InputDialog.getMultiLineText(parent, title, description, label, text)
        inputDialog.staticMethod("getMultiLineText", [](lua_State* L) -> int
        {
            bool isAccepted = false;
            const QString text = ShadInputDialog::getMultiLineText(ShadLuaObject::get<QWidget>(L, 1), optionalString(L, 2), optionalString(L, 3), optionalString(L, 4), optionalString(L, 5), &isAccepted);
            if (!isAccepted)
            {
                lua_pushnil(L);
                return 1;
            }
            ShadLuaStack::pushString(L, text);
            return 1;
        });
        // shad.InputDialog.getInt(parent, title, description, label, value, minimum, maximum, step)
        inputDialog.staticMethod("getInt", [](lua_State* L) -> int
        {
            bool isAccepted = false;
            const int value = ShadInputDialog::getInt(ShadLuaObject::get<QWidget>(L, 1), optionalString(L, 2), optionalString(L, 3), optionalString(L, 4), optionalInt(L, 5, 0), optionalInt(L, 6, -2147483647), optionalInt(L, 7, 2147483647), optionalInt(L, 8, 1), &isAccepted);
            if (!isAccepted)
            {
                lua_pushnil(L);
                return 1;
            }
            lua_pushinteger(L, value);
            return 1;
        });
        // shad.InputDialog.getDouble(parent, title, description, label, value, minimum, maximum, decimals, step)
        inputDialog.staticMethod("getDouble", [](lua_State* L) -> int
        {
            bool isAccepted = false;
            const double value = ShadInputDialog::getDouble(ShadLuaObject::get<QWidget>(L, 1), optionalString(L, 2), optionalString(L, 3), optionalString(L, 4), optionalNumber(L, 5, 0.0), optionalNumber(L, 6, -2147483647.0), optionalNumber(L, 7, 2147483647.0), optionalInt(L, 8, 2), &isAccepted, optionalNumber(L, 9, 1.0));
            if (!isAccepted)
            {
                lua_pushnil(L);
                return 1;
            }
            lua_pushnumber(L, value);
            return 1;
        });

        // new(initialColor, parent) 或 new(parent)
        ShadLuaClass<ShadColorDialog> colorDialog("ColorDialog");
        colorDialog.constructor([](lua_State* L) -> ShadColorDialog*
        {
            QColor initial;
            if (ShadLuaStack::getColor(L, 1, initial))
            {
                return new ShadColorDialog(initial, ShadLuaObject::get<QWidget>(L, 2));
            }
            return new ShadColorDialog(ShadLuaObject::get<QWidget>(L, 1));
        });
        colorDialog.method("colorPicker", &ShadColorDialog::colorPicker);
        colorDialog.method("okButton", &ShadColorDialog::okButton);
        colorDialog.method("cancelButton", &ShadColorDialog::cancelButton);
        // shad.ColorDialog.getColor(initialColor, parent, title)
        colorDialog.staticMethod("getColor", [](lua_State* L) -> int
        {
            QColor initial(Qt::white);
            ShadLuaStack::getColor(L, 1, initial);
            ShadLuaStack::pushColor(L, ShadColorDialog::getColor(initial, ShadLuaObject::get<QWidget>(L, 2), optionalString(L, 3)));
            return 1;
        });

        // 命令面板，addItem 返回 QAction
        ShadLuaClass<ShadCommand> command("Command");
        command.constructor(&ShadLuaBind::newWithParent<ShadCommand>);
        command.method("addGroup", &ShadCommand::addGroup);
        command.method("setGroupHeading", &ShadCommand::setGroupHeading);
        // addItem(text, icon, shortcutText)
        command.rawMethod("addItem", [](lua_State* L, ShadCommand* self) -> int
        {
            ShadLuaObject::push(L, self->addItem(optionalString(L, 2), optionalIcon(L, 3), optionalString(L, 4)));
            return 1;
        });
        command.method("setItemKeywords", &ShadCommand::setItemKeywords);
        command.method("addSeparator", &ShadCommand::addSeparator);
        command.method("clear", &ShadCommand::clear);

        // command() 返回对话框中的命令面板
        ShadLuaClass<ShadCommandDialog> commandDialog("CommandDialog");
        commandDialog.constructor(&ShadLuaBind::newWithParent<ShadCommandDialog>);
        commandDialog.method("command", &ShadCommandDialog::command);
        commandDialog.staticMethod("setDefaultShortcut", [](lua_State* L) -> int
        {
            ShadCommandDialog::setDefaultShortcut(optionalKeySequence(L, 1));
            return 0;
        });
        commandDialog.staticMethod("getDefaultShortcut", [](lua_State* L) -> int
        {
            ShadLuaStack::pushString(L, ShadCommandDialog::getDefaultShortcut().toString(QKeySequence::PortableText));
            return 1;
        });
    }

    // 抽屉、浮层、悬停卡片与气泡确认
    void registerPopups()
    {
        // new(title, description, parent) 或 new(parent)
        ShadLuaClass<ShadDrawer> drawer("Drawer");
        drawer.constructor(&ShadLuaBind::newWithTitle<ShadDrawer>);
        drawer.method("contentLayout", &ShadDrawer::contentLayout);
        drawer.method("footerLayout", &ShadDrawer::footerLayout);
        drawer.method("isOpen", &ShadDrawer::isOpen);
        // setSnapPoints({0.5, 1})
        drawer.rawMethod("setSnapPoints", [](lua_State* L, ShadDrawer* self) -> int
        {
            QList<qreal> snapPoints;
            if (!readNumbers(L, 2, snapPoints))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->setSnapPoints(snapPoints);
            return 0;
        });
        drawer.rawMethod("getSnapPoints", [](lua_State* L, ShadDrawer* self) -> int
        {
            const QList<qreal> snapPoints = self->getSnapPoints();
            lua_createtable(L, int(snapPoints.size()), 0);
            for (int position = 0; position < snapPoints.size(); ++position)
            {
                lua_pushnumber(L, snapPoints.at(position));
                lua_rawseti(L, -2, position + 1);
            }
            return 1;
        });

        ShadLuaClass<ShadLoadingOverlay> loadingOverlay("LoadingOverlay");
        loadingOverlay.constructor(&ShadLuaBind::newWithParent<ShadLoadingOverlay>);
        loadingOverlay.method("setTarget", &ShadLoadingOverlay::setTarget);
        loadingOverlay.method("getTarget", &ShadLoadingOverlay::getTarget);

        ShadLuaClass<ShadPopover> popover("Popover");
        popover.constructor(&ShadLuaBind::newWithParent<ShadPopover>);
        popover.method("contentLayout", &ShadPopover::contentLayout);
        // popup(anchor, side, align)；toggle(anchor, side, align)
        popover.rawMethod("popup", [](lua_State* L, ShadPopover* self) -> int
        {
            self->popup(ShadLuaObject::get<QWidget>(L, 2), optionalSide(L, 3, ShadPlacementType::Bottom), optionalAlign(L, 4));
            return 0;
        });
        popover.rawMethod("toggle", [](lua_State* L, ShadPopover* self) -> int
        {
            self->toggle(ShadLuaObject::get<QWidget>(L, 2), optionalSide(L, 3, ShadPlacementType::Bottom), optionalAlign(L, 4));
            return 0;
        });

        ShadLuaClass<ShadHoverCard> hoverCard("HoverCard");
        hoverCard.constructor(&ShadLuaBind::newWithParent<ShadHoverCard>);
        hoverCard.method("setTrigger", &ShadHoverCard::setTrigger);
        hoverCard.method("getTrigger", &ShadHoverCard::getTrigger);
        hoverCard.method("contentLayout", &ShadHoverCard::contentLayout);
        hoverCard.method("setSide", &ShadHoverCard::setSide);
        hoverCard.method("getSide", &ShadHoverCard::getSide);
        hoverCard.method("setAlign", &ShadHoverCard::setAlign);
        hoverCard.method("getAlign", &ShadHoverCard::getAlign);
        hoverCard.method("popup", &ShadHoverCard::popup);

        // new(title, description, parent) 或 new(parent)；attach(button, side, align)
        ShadLuaClass<ShadPopconfirm> popconfirm("Popconfirm");
        popconfirm.constructor(&ShadLuaBind::newWithTitle<ShadPopconfirm>);
        popconfirm.method("setShadIcon", &ShadPopconfirm::setShadIcon);
        popconfirm.method("getShadIcon", &ShadPopconfirm::getShadIcon);
        popconfirm.method("confirmButton", &ShadPopconfirm::confirmButton);
        popconfirm.method("cancelButton", &ShadPopconfirm::cancelButton);
        popconfirm.rawMethod("attach", [](lua_State* L, ShadPopconfirm* self) -> int
        {
            self->attach(ShadLuaObject::get<QAbstractButton>(L, 2), optionalSide(L, 3, ShadPlacementType::Top), optionalAlign(L, 4));
            return 0;
        });
    }

    // 提示是全局对象，通过 shad.Tooltip.<函数> 调用
    void registerTooltip()
    {
        ShadLuaRegistry::addFunction("Tooltip", "install", [](lua_State* L) -> int
        {
            Q_UNUSED(L);
            ShadTooltip::getInstance()->install();
            return 0;
        });
        ShadLuaRegistry::addFunction("Tooltip", "uninstall", [](lua_State* L) -> int
        {
            Q_UNUSED(L);
            ShadTooltip::getInstance()->uninstall();
            return 0;
        });
        ShadLuaRegistry::addFunction("Tooltip", "isInstalled", [](lua_State* L) -> int
        {
            lua_pushboolean(L, ShadTooltip::getInstance()->isInstalled() ? 1 : 0);
            return 1;
        });
        // setShowDelay(milliseconds) 或 setShowDelay(widget, milliseconds)
        ShadLuaRegistry::addFunction("Tooltip", "setShowDelay", [](lua_State* L) -> int
        {
            QWidget* widget = ShadLuaObject::get<QWidget>(L, 1);
            if (widget != nullptr)
            {
                ShadTooltip::setShowDelay(widget, optionalInt(L, 2, -1));
                return 0;
            }
            ShadTooltip::getInstance()->setShowDelay(optionalInt(L, 1, 0));
            return 0;
        });
        // getShowDelay() 或 getShowDelay(widget)
        ShadLuaRegistry::addFunction("Tooltip", "getShowDelay", [](lua_State* L) -> int
        {
            QWidget* widget = ShadLuaObject::get<QWidget>(L, 1);
            lua_pushinteger(L, widget != nullptr ? ShadTooltip::getShowDelay(widget) : ShadTooltip::getInstance()->getShowDelay());
            return 1;
        });
        // setSide(widget, side)
        ShadLuaRegistry::addFunction("Tooltip", "setSide", [](lua_State* L) -> int
        {
            QWidget* widget = ShadLuaObject::get<QWidget>(L, 1);
            if (widget == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            ShadTooltip::setSide(widget, optionalSide(L, 2, ShadPlacementType::Top));
            return 0;
        });
        ShadLuaRegistry::addFunction("Tooltip", "getSide", [](lua_State* L) -> int
        {
            QWidget* widget = ShadLuaObject::get<QWidget>(L, 1);
            if (widget == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            lua_pushinteger(L, ShadTooltip::getSide(widget));
            return 1;
        });
        // setShortcut(widget, keys)，只用于显示
        ShadLuaRegistry::addFunction("Tooltip", "setShortcut", [](lua_State* L) -> int
        {
            QWidget* widget = ShadLuaObject::get<QWidget>(L, 1);
            if (widget == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            if (lua_type(L, 2) == LUA_TSTRING)
            {
                ShadTooltip::setShortcut(widget, optionalKeySequence(L, 2));
                return 0;
            }
            QStringList keys;
            if (!lua_isnoneornil(L, 2) && !ShadLuaStack::getStringList(L, 2, keys))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            ShadTooltip::setShortcut(widget, keys);
            return 0;
        });
        ShadLuaRegistry::addFunction("Tooltip", "getShortcut", [](lua_State* L) -> int
        {
            QWidget* widget = ShadLuaObject::get<QWidget>(L, 1);
            if (widget == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            ShadLuaStack::pushStringList(L, ShadTooltip::getShortcut(widget));
            return 1;
        });
        // showText(widget, {x, y, 宽, 高}, text)
        ShadLuaRegistry::addFunction("Tooltip", "showText", [](lua_State* L) -> int
        {
            QWidget* widget = ShadLuaObject::get<QWidget>(L, 1);
            if (widget == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            ShadTooltip::getInstance()->showText(widget, ShadLuaStack::getVariant(L, 2, QMetaType::fromType<QRect>()).toRect(), optionalString(L, 3));
            return 0;
        });
        ShadLuaRegistry::addFunction("Tooltip", "hideText", [](lua_State* L) -> int
        {
            Q_UNUSED(L);
            ShadTooltip::getInstance()->hideText();
            return 0;
        });
    }

    // 菜单，菜单项是 QAction
    void registerMenus()
    {
        // 没有父对象的互斥分组归引擎所有
        ShadLuaClass<QActionGroup>("ActionGroup").constructor([](lua_State* L) -> QActionGroup*
        {
            QObject* parent = ShadLuaObject::get(L, 1);
            return new QActionGroup(parent != nullptr ? parent : ShadLuaEngine::fromState(L));
        });

        ShadLuaClass<ShadDropdownMenu> dropdownMenu("DropdownMenu");
        dropdownMenu.constructor(&ShadLuaBind::newWithParent<ShadDropdownMenu>);
        // addItem(text, icon, shortcutText)
        dropdownMenu.rawMethod("addItem", [](lua_State* L, ShadDropdownMenu* self) -> int
        {
            ShadLuaObject::push(L, self->addItem(optionalString(L, 2), optionalIcon(L, 3), optionalString(L, 4)));
            return 1;
        });
        // addCheckableItem(text, checked)
        dropdownMenu.rawMethod("addCheckableItem", [](lua_State* L, ShadDropdownMenu* self) -> int
        {
            ShadLuaObject::push(L, self->addCheckableItem(optionalString(L, 2), optionalBool(L, 3, false)));
            return 1;
        });
        // addRadioItem(text, group, checked)
        dropdownMenu.rawMethod("addRadioItem", [](lua_State* L, ShadDropdownMenu* self) -> int
        {
            ShadLuaObject::push(L, self->addRadioItem(optionalString(L, 2), ShadLuaObject::get<QActionGroup>(L, 3), optionalBool(L, 4, false)));
            return 1;
        });
        // addMenu(text, icon)
        dropdownMenu.rawMethod("addMenu", [](lua_State* L, ShadDropdownMenu* self) -> int
        {
            ShadLuaObject::push(L, self->addMenu(optionalString(L, 2), optionalIcon(L, 3)));
            return 1;
        });
        dropdownMenu.method("menuAction", &ShadDropdownMenu::menuAction);
        // addLabel(text, inset)
        dropdownMenu.rawMethod("addLabel", [](lua_State* L, ShadDropdownMenu* self) -> int
        {
            lua_pushinteger(L, self->addLabel(optionalString(L, 2), optionalBool(L, 3, false)));
            return 1;
        });
        dropdownMenu.method("setLabelText", &ShadDropdownMenu::setLabelText);
        dropdownMenu.method("addWidget", &ShadDropdownMenu::addWidget);
        dropdownMenu.method("addSeparator", &ShadDropdownMenu::addSeparator);
        dropdownMenu.method("clear", &ShadDropdownMenu::clear);
        dropdownMenu.method("setItemDestructive", &ShadDropdownMenu::setItemDestructive);
        dropdownMenu.method("setItemInset", &ShadDropdownMenu::setItemInset);
        dropdownMenu.method("setMatchAnchorWidth", &ShadDropdownMenu::setMatchAnchorWidth);
        dropdownMenu.method("isMatchAnchorWidth", &ShadDropdownMenu::isMatchAnchorWidth);
        // popup(anchor, alignment)、popup(anchor, side, align) 或 popup({x, y, 宽, 高}, alignment)
        dropdownMenu.rawMethod("popup", [](lua_State* L, ShadDropdownMenu* self) -> int
        {
            if (lua_type(L, 2) == LUA_TTABLE)
            {
                self->popup(ShadLuaStack::getVariant(L, 2, QMetaType::fromType<QRect>()).toRect(), Qt::Alignment::fromInt(optionalInt(L, 3, Qt::AlignLeft)));
                return 0;
            }
            QWidget* anchor = ShadLuaObject::get<QWidget>(L, 2);
            if (anchor == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            if (lua_type(L, 4) == LUA_TNUMBER)
            {
                self->popup(anchor, optionalSide(L, 3, ShadPlacementType::Bottom), optionalAlign(L, 4));
                return 0;
            }
            self->popup(anchor, Qt::Alignment::fromInt(optionalInt(L, 3, Qt::AlignLeft)));
            return 0;
        });
        // popupAt(x, y)
        dropdownMenu.method("popupAt", +[](ShadDropdownMenu* self, int x, int y) { self->popupAt(QPoint(x, y)); });

        // 右键菜单，用 attachTo(widget) 绑定组件
        ShadLuaClass<ShadContextMenu> contextMenu("ContextMenu");
        contextMenu.constructor(&ShadLuaBind::newWithParent<ShadContextMenu>);
        contextMenu.method("attachTo", &ShadContextMenu::attachTo);
        contextMenu.method("detachFrom", &ShadContextMenu::detachFrom);

        // addMenu(title)
        ShadLuaClass<ShadMenubar> menubar("Menubar");
        menubar.constructor(&ShadLuaBind::newWithParent<ShadMenubar>);
        menubar.method("addMenu", &ShadMenubar::addMenu);
        menubar.method("removeMenu", &ShadMenubar::removeMenu);
        menubar.method("setMenuTitle", &ShadMenubar::setMenuTitle);
        menubar.rawMethod("getMenus", [](lua_State* L, ShadMenubar* self) -> int
        {
            pushObjects(L, self->getMenus());
            return 1;
        });

        // addMenu(title, content)；addLink(title) 返回 QAction
        ShadLuaClass<ShadNavigationMenu> navigationMenu("NavigationMenu");
        navigationMenu.constructor(&ShadLuaBind::newWithParent<ShadNavigationMenu>);
        navigationMenu.method("addMenu", &ShadNavigationMenu::addMenu);
        navigationMenu.method("setMenuTitle", &ShadNavigationMenu::setMenuTitle);
        navigationMenu.method("addLink", &ShadNavigationMenu::addLink);
        navigationMenu.method("closeMenu", &ShadNavigationMenu::closeMenu);

        // new(title, description, parent) 或 new(parent)
        ShadLuaClass<ShadNavigationMenuLink>("NavigationMenuLink").constructor(&ShadLuaBind::newWithTitle<ShadNavigationMenuLink>);
    }

    using NotifyFunction = void (*)(const QString&, const QString&, int, ShadNotificationType::Position, QWidget*);

    // 参数为 title、text、duration、position、parent
    int notify(lua_State* L, NotifyFunction function)
    {
        function(optionalString(L, 1), optionalString(L, 2), optionalInt(L, 3, 3000), static_cast<ShadNotificationType::Position>(optionalInt(L, 4, ShadNotificationType::TopRight)), ShadLuaObject::get<QWidget>(L, 5));
        return 0;
    }

    // 通知没有构造函数，通过 shad.Notification.info(...) 调用
    void registerNotification()
    {
        ShadLuaRegistry::addFunction("Notification", "info", [](lua_State* L) -> int { return notify(L, &ShadNotification::info); });
        ShadLuaRegistry::addFunction("Notification", "success", [](lua_State* L) -> int { return notify(L, &ShadNotification::success); });
        ShadLuaRegistry::addFunction("Notification", "warning", [](lua_State* L) -> int { return notify(L, &ShadNotification::warning); });
        ShadLuaRegistry::addFunction("Notification", "error", [](lua_State* L) -> int { return notify(L, &ShadNotification::error); });
        // closeAll(parent)
        ShadLuaRegistry::addFunction("Notification", "closeAll", [](lua_State* L) -> int
        {
            ShadNotification::closeAll(ShadLuaObject::get<QWidget>(L, 1));
            return 0;
        });
    }
} // namespace

namespace ShadLuaBindings
{
    void registerOverlayComponents()
    {
        registerDialogs();
        registerPopups();
        registerTooltip();
        registerMenus();
        registerNotification();
    }
} // namespace ShadLuaBindings
