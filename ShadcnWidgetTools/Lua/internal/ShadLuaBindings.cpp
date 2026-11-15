#include "ShadLuaBindings.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLayout>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadAccordion.h"
#include "ShadAlert.h"
#include "ShadAvatar.h"
#include "ShadBadge.h"
#include "ShadButton.h"
#include "ShadCalendar.h"
#include "ShadCard.h"
#include "ShadCheckbox.h"
#include "ShadCodeBlock.h"
#include "ShadComboBox.h"
#include "ShadDatePicker.h"
#include "ShadDef.h"
#include "ShadDialog.h"
#include "ShadDiffView.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadLuaBinder.h"
#include "ShadLuaEngine.h"
#include "ShadLuaEnginePrivate.h"
#include "ShadLuaInclude.h"
#include "ShadLuaObject.h"
#include "ShadLuaSignalProxy.h"
#include "ShadLuaStack.h"
#include "ShadMarquee.h"
#include "ShadPagination.h"
#include "ShadProgress.h"
#include "ShadQrCode.h"
#include "ShadRadialProgress.h"
#include "ShadRadioButton.h"
#include "ShadRating.h"
#include "ShadSegmented.h"
#include "ShadSeparator.h"
#include "ShadSkeleton.h"
#include "ShadSlider.h"
#include "ShadSpinBox.h"
#include "ShadSpinner.h"
#include "ShadStatistic.h"
#include "ShadSwitch.h"
#include "ShadTabs.h"
#include "ShadTextarea.h"
#include "ShadTheme.h"
#include "ShadToaster.h"
#include "ShadWidget.h"

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

    // new(orientation, parent) 或 new(parent)
    template <class T>
    T* newWithOrientation(lua_State* L)
    {
        if (lua_type(L, 1) == LUA_TNUMBER)
        {
            return new T(static_cast<Qt::Orientation>(optionalInt(L, 1, Qt::Horizontal)), ShadLuaObject::get<QWidget>(L, 2));
        }
        return new T(Qt::Horizontal, ShadLuaObject::get<QWidget>(L, 1));
    }

    // QObject：信号连接与对象管理
    void registerObject()
    {
        ShadLuaClass<QObject> object;
        object.rawMethod("on", [](lua_State* L, QObject* self) -> int
        {
            QString signalName;
            if (!ShadLuaStack::getString(L, 2, signalName) || lua_type(L, 3) != LUA_TFUNCTION)
            {
                return ShadLuaStack::error(L, QStringLiteral("on 需要信号名称与回调函数，例如 button:on(\"clicked\", function() end)"));
            }
            ShadLuaEngine* engine = ShadLuaEngine::fromState(L);
            if (engine == nullptr)
            {
                return ShadLuaStack::error(L, QStringLiteral("当前 Lua 状态不属于 ShadLuaEngine，无法连接信号"));
            }
            QString message;
            if (!ShadLuaEnginePrivate::get(engine)->_signalProxy->connectSignal(self, signalName.toUtf8(), L, 3, &message))
            {
                return ShadLuaStack::error(L, message);
            }
            return 0;
        });
        object.rawMethod("delete", [](lua_State* L, QObject* self) -> int
        {
            Q_UNUSED(L);
            self->deleteLater();
            return 0;
        });
        object.rawMethod("className", [](lua_State* L, QObject* self) -> int
        {
            ShadLuaStack::pushString(L, QString::fromUtf8(self->metaObject()->className()));
            return 1;
        });
        object.rawMethod("equals", [](lua_State* L, QObject* self) -> int
        {
            lua_pushboolean(L, ShadLuaObject::get(L, 2) == self ? 1 : 0);
            return 1;
        });
    }

    // QWidget、布局、定时器与下拉框基类
    void registerWidgets()
    {
        ShadLuaClass<QWidget> widget("Widget");
        widget.constructor(&ShadLuaBind::newWithParent<QWidget>);
        widget.method("resize", +[](QWidget* self, int width, int height) { self->resize(width, height); });
        widget.method("move", +[](QWidget* self, int x, int y) { self->move(x, y); });
        widget.method("setFixedSize", +[](QWidget* self, int width, int height) { self->setFixedSize(width, height); });
        widget.method("setFixedWidth", &QWidget::setFixedWidth);
        widget.method("setFixedHeight", &QWidget::setFixedHeight);
        widget.method("setMinimumSize", +[](QWidget* self, int width, int height) { self->setMinimumSize(width, height); });
        widget.method("setMaximumSize", +[](QWidget* self, int width, int height) { self->setMaximumSize(width, height); });
        widget.method("setContentsMargins", +[](QWidget* self, int left, int top, int right, int bottom) { self->setContentsMargins(left, top, right, bottom); });
        widget.method("setParent", +[](QWidget* self, QWidget* parent) { self->setParent(parent); });
        widget.method("setLayout", +[](QWidget* self, QLayout* layout) { self->setLayout(layout); });
        widget.method("adjustSize", &QWidget::adjustSize);

        ShadLuaClass<QLayout> layout;
        layout.method("setContentsMargins", +[](QLayout* self, int left, int top, int right, int bottom) { self->setContentsMargins(left, top, right, bottom); });
        layout.method("addWidget", +[](QLayout* self, QWidget* child) { self->addWidget(child); });
        layout.method("removeWidget", +[](QLayout* self, QWidget* child) { self->removeWidget(child); });

        // addWidget(widget, stretch, alignment) 与 addLayout(layout, stretch)
        ShadLuaClass<QBoxLayout> boxLayout;
        boxLayout.rawMethod("addWidget", [](lua_State* L, QBoxLayout* self) -> int
        {
            QWidget* child = ShadLuaObject::get<QWidget>(L, 2);
            if (child == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->addWidget(child, optionalInt(L, 3, 0), Qt::Alignment::fromInt(optionalInt(L, 4, 0)));
            return 0;
        });
        boxLayout.rawMethod("addLayout", [](lua_State* L, QBoxLayout* self) -> int
        {
            QLayout* child = ShadLuaObject::get<QLayout>(L, 2);
            if (child == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->addLayout(child, optionalInt(L, 3, 0));
            return 0;
        });
        boxLayout.rawMethod("addStretch", [](lua_State* L, QBoxLayout* self) -> int
        {
            self->addStretch(optionalInt(L, 2, 0));
            return 0;
        });
        boxLayout.method("addSpacing", &QBoxLayout::addSpacing);
        ShadLuaClass<QVBoxLayout>("VBox").constructor(&ShadLuaBind::newWithParent<QVBoxLayout>);
        ShadLuaClass<QHBoxLayout>("HBox").constructor(&ShadLuaBind::newWithParent<QHBoxLayout>);

        // addWidget(widget, row, column, rowSpan, columnSpan, alignment)
        ShadLuaClass<QGridLayout> gridLayout("Grid");
        gridLayout.constructor(&ShadLuaBind::newWithParent<QGridLayout>);
        gridLayout.rawMethod("addWidget", [](lua_State* L, QGridLayout* self) -> int
        {
            QWidget* child = ShadLuaObject::get<QWidget>(L, 2);
            if (child == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->addWidget(child, optionalInt(L, 3, 0), optionalInt(L, 4, 0), optionalInt(L, 5, 1), optionalInt(L, 6, 1), Qt::Alignment::fromInt(optionalInt(L, 7, 0)));
            return 0;
        });
        gridLayout.rawMethod("addLayout", [](lua_State* L, QGridLayout* self) -> int
        {
            QLayout* child = ShadLuaObject::get<QLayout>(L, 2);
            if (child == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->addLayout(child, optionalInt(L, 3, 0), optionalInt(L, 4, 0), optionalInt(L, 5, 1), optionalInt(L, 6, 1));
            return 0;
        });

        // 没有父对象的定时器归引擎所有，避免被回收后停止
        ShadLuaClass<QTimer>("Timer").constructor([](lua_State* L) -> QTimer*
        {
            QObject* parent = ShadLuaObject::get(L, 1);
            return new QTimer(parent != nullptr ? parent : ShadLuaEngine::fromState(L));
        });

        ShadLuaClass<QComboBox> comboBox;
        comboBox.method("addItem", +[](QComboBox* self, QString text) { self->addItem(text); });
        comboBox.method("addItems", &QComboBox::addItems);
        comboBox.method("itemText", &QComboBox::itemText);
        comboBox.method("removeItem", &QComboBox::removeItem);
    }

    // 组件库的核心组件，只注册构造函数与非属性方法
    void registerComponents()
    {
        ShadLuaClass<ShadTheme>().method("isDark", &ShadTheme::isDark);

        // 脚本创建的窗口不阻止程序退出
        ShadLuaClass<ShadWidget> shadWindow("Window");
        shadWindow.constructor([](lua_State* L) -> ShadWidget*
        {
            ShadWidget* window = new ShadWidget(ShadLuaObject::get<QWidget>(L, 1));
            window->setAttribute(Qt::WA_QuitOnClose, false);
            return window;
        });
        shadWindow.method("setButtonFlags", +[](ShadWidget* self, int flags) { self->setButtonFlags(ShadWindowType::ButtonFlags::fromInt(flags)); });
        shadWindow.method("getButtonFlags", +[](ShadWidget* self) { return self->getButtonFlags().toInt(); });

        ShadLuaClass<ShadLabel>("Label").constructor(&ShadLuaBind::newWithText<ShadLabel>);
        ShadLuaClass<ShadInput>("Input").constructor(&ShadLuaBind::newWithParent<ShadInput>);
        ShadLuaClass<ShadTextarea>("Textarea").constructor(&ShadLuaBind::newWithParent<ShadTextarea>);
        ShadLuaClass<ShadCheckbox>("Checkbox").constructor(&ShadLuaBind::newWithText<ShadCheckbox>);
        ShadLuaClass<ShadSwitch>("Switch").constructor(&ShadLuaBind::newWithText<ShadSwitch>);
        ShadLuaClass<ShadRadioButton>("RadioButton").constructor(&ShadLuaBind::newWithText<ShadRadioButton>);
        ShadLuaClass<ShadComboBox>("ComboBox").constructor(&ShadLuaBind::newWithParent<ShadComboBox>);
        ShadLuaClass<ShadSlider>("Slider").constructor(&newWithOrientation<ShadSlider>);
        ShadLuaClass<ShadProgress>("Progress").constructor(&ShadLuaBind::newWithParent<ShadProgress>);
        ShadLuaClass<ShadSeparator>("Separator").constructor(&newWithOrientation<ShadSeparator>);
        ShadLuaClass<ShadSkeleton>("Skeleton").constructor(&ShadLuaBind::newWithParent<ShadSkeleton>);
        ShadLuaClass<ShadMarquee>("Marquee").constructor(&ShadLuaBind::newWithText<ShadMarquee>);
        ShadLuaClass<ShadSpinBox>("SpinBox").constructor(&ShadLuaBind::newWithParent<ShadSpinBox>);
        ShadLuaClass<ShadRating>("Rating").constructor(&ShadLuaBind::newWithParent<ShadRating>);
        ShadLuaClass<ShadRadialProgress>("RadialProgress").constructor(&ShadLuaBind::newWithParent<ShadRadialProgress>);
        ShadLuaClass<ShadStatistic>("Statistic").constructor(&ShadLuaBind::newWithText<ShadStatistic>);
        ShadLuaClass<ShadQrCode>("QrCode").constructor(&ShadLuaBind::newWithText<ShadQrCode>);
        ShadLuaClass<ShadAvatar>("Avatar").constructor(&ShadLuaBind::newWithText<ShadAvatar>);
        ShadLuaClass<ShadCalendar>("Calendar").constructor(&ShadLuaBind::newWithParent<ShadCalendar>);
        ShadLuaClass<ShadDatePicker>("DatePicker").constructor(&ShadLuaBind::newWithParent<ShadDatePicker>);
        ShadLuaClass<ShadPagination>("Pagination").constructor(&ShadLuaBind::newWithParent<ShadPagination>);

        // addItem(text)
        ShadLuaClass<ShadSegmented> segmented("Segmented");
        segmented.constructor(&ShadLuaBind::newWithParent<ShadSegmented>);
        segmented.rawMethod("addItem", [](lua_State* L, ShadSegmented* self) -> int
        {
            lua_pushinteger(L, self->addItem(optionalString(L, 2)));
            return 1;
        });
        segmented.method("removeItem", &ShadSegmented::removeItem);
        segmented.method("setItemText", &ShadSegmented::setItemText);

        // addItem(value, title, content)
        ShadLuaClass<ShadAccordion> accordion("Accordion");
        accordion.constructor(&ShadLuaBind::newWithParent<ShadAccordion>);
        accordion.rawMethod("addItem", [](lua_State* L, ShadAccordion* self) -> int
        {
            QWidget* content = ShadLuaObject::get<QWidget>(L, 4);
            if (content != nullptr)
            {
                lua_pushboolean(L, self->addItem(optionalString(L, 2), optionalString(L, 3), content) ? 1 : 0);
                return 1;
            }
            ShadLuaObject::push(L, self->addItem(optionalString(L, 2), optionalString(L, 3), optionalString(L, 4)));
            return 1;
        });
        accordion.method("removeItem", &ShadAccordion::removeItem);
        accordion.method("setItemExpanded", &ShadAccordion::setItemExpanded);

        ShadLuaClass<ShadButton> button("Button");
        button.constructor(&ShadLuaBind::newWithText<ShadButton>);
        button.method("setShadIcon", &ShadButton::setShadIcon);

        ShadLuaClass<ShadBadge> badge("Badge");
        badge.constructor(&ShadLuaBind::newWithText<ShadBadge>);
        badge.method("setShadIcon", &ShadBadge::setShadIcon);

        ShadLuaClass<ShadSpinner> spinner("Spinner");
        spinner.constructor(&ShadLuaBind::newWithParent<ShadSpinner>);
        spinner.method("setShadIcon", &ShadSpinner::setShadIcon);

        ShadLuaClass<ShadAlert> alert("Alert");
        alert.constructor(&ShadLuaBind::newWithParent<ShadAlert>);
        alert.method("setShadIcon", &ShadAlert::setShadIcon);

        ShadLuaClass<ShadCard> card("Card");
        card.constructor(&ShadLuaBind::newWithTitle<ShadCard>);
        card.method("contentLayout", &ShadCard::contentLayout);
        card.method("footerLayout", &ShadCard::footerLayout);

        ShadLuaClass<ShadDialog> dialog("Dialog");
        dialog.constructor(&ShadLuaBind::newWithTitle<ShadDialog>);
        dialog.method("contentLayout", &ShadDialog::contentLayout);
        dialog.method("footerLayout", &ShadDialog::footerLayout);
        // 对话框默认只显示关闭按钮
        dialog.method("setButtonFlags", +[](ShadDialog* self, int flags) { self->setButtonFlags(ShadWindowType::ButtonFlags::fromInt(flags)); });
        dialog.method("getButtonFlags", +[](ShadDialog* self) { return self->getButtonFlags().toInt(); });

        // addTab(text, page)
        ShadLuaClass<ShadTabs> tabs("Tabs");
        tabs.constructor(&ShadLuaBind::newWithParent<ShadTabs>);
        tabs.rawMethod("addTab", [](lua_State* L, ShadTabs* self) -> int
        {
            lua_pushinteger(L, self->addTab(optionalString(L, 2), ShadLuaObject::get<QWidget>(L, 3)));
            return 1;
        });
        tabs.method("removeTab", &ShadTabs::removeTab);
        tabs.method("count", &ShadTabs::count);
        tabs.method("widget", &ShadTabs::widget);
        tabs.method("setTabText", &ShadTabs::setTabText);

        // new(host)；add(title, description, type)
        ShadLuaClass<ShadToaster> toaster("Toaster");
        toaster.constructor([](lua_State* L) -> ShadToaster*
        {
            QWidget* host = ShadLuaObject::get<QWidget>(L, 1);
            return host != nullptr ? new ShadToaster(host) : nullptr;
        });
        toaster.rawMethod("add", [](lua_State* L, ShadToaster* self) -> int
        {
            lua_pushinteger(L, self->add(optionalString(L, 2), optionalString(L, 3), static_cast<ShadToastType::Type>(optionalInt(L, 4, ShadToastType::Default))));
            return 1;
        });
        toaster.method("dismiss", &ShadToaster::dismiss);
        toaster.method("dismissAll", &ShadToaster::dismissAll);

        ShadLuaClass<ShadCodeBlock> codeBlock("CodeBlock");
        codeBlock.constructor(&ShadLuaBind::newWithText<ShadCodeBlock>);
        codeBlock.method("appendCode", &ShadCodeBlock::appendCode);
        codeBlock.method("setLineVariant", &ShadCodeBlock::setLineVariant);
        codeBlock.method("clearLineVariants", &ShadCodeBlock::clearLineVariants);
        codeBlock.method("addActionWidget", &ShadCodeBlock::addActionWidget);
        codeBlock.method("footerLayout", &ShadCodeBlock::footerLayout);

        ShadLuaClass<ShadDiffView> diffView("DiffView");
        diffView.constructor(&ShadLuaBind::newWithParent<ShadDiffView>);
        diffView.method("setTexts", &ShadDiffView::setTexts);
        diffView.method("getAddedLineCount", &ShadDiffView::getAddedLineCount);
        diffView.method("getRemovedLineCount", &ShadDiffView::getRemovedLineCount);
        diffView.method("selectedText", &ShadDiffView::selectedText);
    }
} // namespace

namespace ShadLuaBindings
{
    void registerAll()
    {
        static bool isRegistered = false;
        if (isRegistered)
        {
            return;
        }
        isRegistered = true;
        registerObject();
        registerWidgets();
        registerComponents();
        registerDataComponents();
        registerFormComponents();
        registerLayoutComponents();
        registerOverlayComponents();
        registerDisplayComponents();
    }
} // namespace ShadLuaBindings
