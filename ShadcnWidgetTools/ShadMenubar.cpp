#include "ShadMenubar.h"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadDropdownMenuPrivate.h"
#include "ShadMenubarPrivate.h"
#include "ShadMenubarTrigger.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadMenubar::ShadMenubar(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadMenubarPrivate())
{
    Q_D(ShadMenubar);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    const int ring = ShadStyleHelper::kFocusRingWidth;
    const int marginX = ring + ShadMenubarPrivate::kBorderWidth + ShadMenubarPrivate::kPadding;
    const int marginY = ring + (ShadMenubarPrivate::kBarHeight - ShadMenubarPrivate::kTriggerHeight) / 2;
    setFixedHeight(ShadMenubarPrivate::kBarHeight + 2 * ring);
    d->_layout = new QHBoxLayout(this);
    d->_layout->setContentsMargins(marginX, marginY, marginX, marginY);
    d->_layout->setSpacing(ShadMenubarPrivate::kGap);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadMenubar::~ShadMenubar()
{
    Q_D(ShadMenubar);
    // 系统菜单栏属于顶层窗口，回调引用私有数据
    delete d->_nativeMenuBar.data();
    if (!d->_nativeWindow.isNull())
    {
        d->_nativeWindow->removeEventFilter(this);
    }
    for (const ShadMenubarPrivate::Menu& entry : d->_menus)
    {
        disconnect(entry.menu, nullptr, this, nullptr);
        delete entry.menu;
    }
    d->_menus.clear();
}

ShadDropdownMenu* ShadMenubar::addMenu(const QString& title)
{
    Q_D(ShadMenubar);
    ShadMenubarTrigger* trigger = new ShadMenubarTrigger(title, this);
    ShadDropdownMenu* menu = new ShadDropdownMenu(this);
    menu->d_func()->setMenubarStyle(true);
    menu->installEventFilter(this);
    d->_layout->addWidget(trigger);
    d->_menus.append({trigger, menu});
    connect(trigger, &ShadMenubarTrigger::triggerPressed, this, [d, trigger]()
    {
        const int index = d->indexOfTrigger(trigger);
        if (index == d->_openIndex)
        {
            d->_menus.at(index).menu->hide();
            return;
        }
        d->openMenu(index, 0);
    });
    connect(menu, &ShadDropdownMenu::aboutToHide, this, [d, menu]() { d->onMenuHidden(menu); });
    d->updateFocusPolicies();
    if (d->usesNativeMenuBar())
    {
        d->requestNativeSync();
    }
    return menu;
}

void ShadMenubar::setMenuTitle(ShadDropdownMenu* menu, const QString& title)
{
    Q_D(ShadMenubar);
    const int index = d->indexOfMenu(menu);
    if (index < 0)
    {
        return;
    }
    // QAbstractButton::setText 会重新计算尺寸并重绘
    d->_menus.at(index).trigger->setText(title);
    if (d->usesNativeMenuBar())
    {
        d->requestNativeSync();
    }
}

void ShadMenubar::removeMenu(ShadDropdownMenu* menu)
{
    Q_D(ShadMenubar);
    const int index = d->indexOfMenu(menu);
    if (index < 0)
    {
        return;
    }
    const ShadMenubarPrivate::Menu entry = d->_menus.at(index);
    disconnect(entry.menu, nullptr, this, nullptr);
    entry.menu->hide();
    d->_menus.removeAt(index);
    if (d->_openIndex == index)
    {
        d->_openIndex = -1;
    }
    else if (d->_openIndex > index)
    {
        --d->_openIndex;
    }
    d->_focusIndex = qBound(0, d->_focusIndex > index ? d->_focusIndex - 1 : d->_focusIndex, qMax(0, int(d->_menus.size()) - 1));
    d->_layout->removeWidget(entry.trigger);
    if (d->usesNativeMenuBar())
    {
        d->syncNativeMenuBar();
    }
    delete entry.trigger;
    delete entry.menu;
    d->updateFocusPolicies();
}

QList<ShadDropdownMenu*> ShadMenubar::getMenus() const
{
    Q_D(const ShadMenubar);
    QList<ShadDropdownMenu*> menus;
    menus.reserve(d->_menus.size());
    for (const ShadMenubarPrivate::Menu& entry : d->_menus)
    {
        menus.append(entry.menu);
    }
    return menus;
}

void ShadMenubar::setNativeMenuBar(bool native)
{
    Q_D(ShadMenubar);
    if (d->_isNativeMenuBar == native)
    {
        return;
    }
    d->_isNativeMenuBar = native;
#ifdef Q_OS_MACOS
    if (native)
    {
        if (d->_openIndex >= 0)
        {
            d->_menus.at(d->_openIndex).menu->hide();
        }
        d->_wasHiddenBeforeNative = isHidden();
        hide();
    }
    else if (!d->_wasHiddenBeforeNative)
    {
        show();
    }
#endif
    d->syncNativeMenuBar();
}

bool ShadMenubar::isNativeMenuBar() const
{
    Q_D(const ShadMenubar);
    return d->_isNativeMenuBar;
}

bool ShadMenubar::event(QEvent* event)
{
    Q_D(ShadMenubar);
    if (event->type() == QEvent::ParentChange && d->usesNativeMenuBar())
    {
        d->requestNativeSync();
    }
    return QWidget::event(event);
}

bool ShadMenubar::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadMenubar);
    if (watched == d->_nativeWindow.data())
    {
        if (event->type() == QEvent::Show && d->usesNativeMenuBar())
        {
            d->requestNativeSync();
        }
        return QWidget::eventFilter(watched, event);
    }
    const int menuIndex = d->indexOfMenu(watched);
    if (menuIndex < 0 || menuIndex != d->_openIndex)
    {
        return QWidget::eventFilter(watched, event);
    }
    ShadDropdownMenu* menu = d->_menus.at(menuIndex).menu;
    switch (event->type())
    {
        case QEvent::MouseMove:
            {
                // 菜单打开期间鼠标事件都发给菜单
                const int index = d->triggerAt(static_cast<QMouseEvent*>(event)->globalPosition().toPoint());
                if (index >= 0 && index != d->_openIndex)
                {
                    d->openMenu(index, 0);
                    return true;
                }
                break;
            }
        case QEvent::MouseButtonPress:
            {
                const int index = d->triggerAt(static_cast<QMouseEvent*>(event)->globalPosition().toPoint());
                if (index < 0)
                {
                    break;
                }
                // 避免按下重放到触发项后重新打开
                menu->setAttribute(Qt::WA_NoMouseReplay);
                if (index == d->_openIndex)
                {
                    menu->hide();
                }
                else
                {
                    d->openMenu(index, 0);
                }
                return true;
            }
        case QEvent::KeyPress:
            {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                if (keyEvent->key() == Qt::Key_Escape || keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)
                {
                    d->_isKeyboardClose = true;
                    break;
                }
                if (keyEvent->key() != Qt::Key_Left && keyEvent->key() != Qt::Key_Right)
                {
                    break;
                }
                // 子菜单触发项上的结束侧方向键交给菜单
                const bool isEndKey = (keyEvent->key() == Qt::Key_Right) == (layoutDirection() == Qt::LeftToRight);
                const ShadDropdownMenuPrivate* menuPrivate = menu->d_func();
                const bool isOnSubmenu = menuPrivate->isSelectable(menuPrivate->_highlightIndex) && menuPrivate->_entries.at(menuPrivate->_highlightIndex).kind == ShadDropdownMenuPrivate::EntryKind::Submenu;
                if (isEndKey && isOnSubmenu)
                {
                    break;
                }
                d->openMenu(d->wrappedIndex(d->_openIndex, isEndKey ? 1 : -1), 1);
                return true;
            }
        default:
            {
                break;
            }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadMenubar::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadMenubar);
    const int index = d->indexOfTrigger(QApplication::focusWidget());
    if (index < 0)
    {
        QWidget::keyPressEvent(event);
        return;
    }
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    switch (event->key())
    {
        case Qt::Key_Left:
        case Qt::Key_Right:
            {
                const bool isNext = (event->key() == Qt::Key_Right) != isRightToLeft;
                d->focusTrigger(d->wrappedIndex(index, isNext ? 1 : -1), Qt::TabFocusReason);
                break;
            }
        case Qt::Key_Home:
            {
                d->focusTrigger(0, Qt::TabFocusReason);
                break;
            }
        case Qt::Key_End:
            {
                d->focusTrigger(int(d->_menus.size()) - 1, Qt::TabFocusReason);
                break;
            }
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
        case Qt::Key_Down:
            {
                d->openMenu(index, 1);
                break;
            }
        case Qt::Key_Up:
            {
                d->openMenu(index, -1);
                break;
            }
        default:
            {
                QWidget::keyPressEvent(event);
                return;
            }
    }
    event->accept();
}

void ShadMenubar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    // rounded-md border shadow-xs
    const QRectF body = QRectF(rect()).adjusted(kFocusRingWidth, kFocusRingWidth, -kFocusRingWidth, -kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    drawShadowXs(&painter, body, radius);
    strokeRoundedRect(&painter, body, radius, shadTheme->getColor(ShadThemeType::Border));
}
