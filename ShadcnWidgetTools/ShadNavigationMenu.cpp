#include "ShadNavigationMenu.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QSizePolicy>
#include <QTimer>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <utility>

#include "ShadNavigationMenuPrivate.h"
#include "ShadNavigationMenuTrigger.h"
#include "ShadNavigationMenuViewport.h"

ShadNavigationMenu::ShadNavigationMenu(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadNavigationMenuPrivate())
{
    Q_D(ShadNavigationMenu);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    d->_layout = new QHBoxLayout(this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->_layout->setSpacing(ShadNavigationMenuPrivate::kGap);

    d->_viewport = new ShadNavigationMenuViewport(this);
    d->_viewport->installEventFilter(this);
    connect(d->_viewport, &ShadNavigationMenuViewport::closed, this, [d]() { d->onViewportClosed(); });

    d->_openTimer = new QTimer(this);
    d->_openTimer->setSingleShot(true);
    connect(d->_openTimer, &QTimer::timeout, this, [d]()
    {
        if (d->_pendingIndex >= 0)
        {
            d->openMenu(d->_pendingIndex, false);
        }
    });
    d->_closeTimer = new QTimer(this);
    d->_closeTimer->setSingleShot(true);
    connect(d->_closeTimer, &QTimer::timeout, this, [d]() { d->_viewport->hide(); });
}

ShadNavigationMenu::~ShadNavigationMenu()
{
    Q_D(ShadNavigationMenu);
    // 下拉内容窗口的回调引用私有数据
    d->_viewport->removeEventFilter(this);
    disconnect(d->_viewport, nullptr, this, nullptr);
    delete d->_viewport;
}

void ShadNavigationMenu::addMenu(const QString& title, QWidget* content)
{
    Q_D(ShadNavigationMenu);
    if (content == nullptr)
    {
        return;
    }
    content->setParent(d->_viewport->body());
    content->hide();
    ShadNavigationMenuTrigger* trigger = new ShadNavigationMenuTrigger(title, true, this);
    d->_layout->addWidget(trigger);
    ShadNavigationMenuPrivate::Item item;
    item.trigger = trigger;
    item.content = content;
    d->_items.append(item);
    connect(trigger, &QAbstractButton::clicked, this, [d, trigger]() { d->onTriggerClicked(d->indexOfTrigger(trigger)); });
    connect(trigger, &ShadNavigationMenuTrigger::hoverChanged, this, [d, trigger](bool hovered) { d->onHoverChanged(d->indexOfTrigger(trigger), hovered); });
    d->updateFocusPolicies();
}

void ShadNavigationMenu::setMenuTitle(QWidget* content, const QString& title)
{
    Q_D(ShadNavigationMenu);
    if (content == nullptr)
    {
        return;
    }
    for (const ShadNavigationMenuPrivate::Item& item : std::as_const(d->_items))
    {
        if (item.content == content)
        {
            // QAbstractButton::setText 会重新计算尺寸并重绘
            item.trigger->setText(title);
            return;
        }
    }
}

QAction* ShadNavigationMenu::addLink(const QString& title)
{
    Q_D(ShadNavigationMenu);
    QAction* action = new QAction(title, this);
    ShadNavigationMenuTrigger* trigger = new ShadNavigationMenuTrigger(title, false, this);
    d->_layout->addWidget(trigger);
    ShadNavigationMenuPrivate::Item item;
    item.trigger = trigger;
    item.action = action;
    d->_items.append(item);
    connect(trigger, &QAbstractButton::clicked, this, [d, trigger]() { d->onTriggerClicked(d->indexOfTrigger(trigger)); });
    connect(action, &QAction::changed, trigger, [trigger, action]()
    {
        trigger->setText(action->text());
        trigger->setEnabled(action->isEnabled());
        trigger->setVisible(action->isVisible());
    });
    d->updateFocusPolicies();
    return action;
}

void ShadNavigationMenu::closeMenu()
{
    Q_D(ShadNavigationMenu);
    d->_openTimer->stop();
    d->_pendingIndex = -1;
    d->_viewport->hide();
}

bool ShadNavigationMenu::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadNavigationMenu);
    if (watched != d->_viewport || !d->_viewport->isVisible())
    {
        return QWidget::eventFilter(watched, event);
    }
    switch (event->type())
    {
        case QEvent::MouseMove:
            {
                // 下拉内容打开期间鼠标事件都发给它
                const QPoint globalPos = static_cast<QMouseEvent*>(event)->globalPosition().toPoint();
                d->syncHover();
                const int index = d->triggerAt(globalPos);
                if (d->hasMenu(index) && index != d->_openIndex && index != d->_suppressHoverIndex)
                {
                    d->openMenu(index, false);
                    break;
                }
                if (index == d->_openIndex || d->_viewport->cardRect().contains(d->_viewport->mapFromGlobal(globalPos)))
                {
                    d->_closeTimer->stop();
                }
                else if (!d->_closeTimer->isActive())
                {
                    d->_closeTimer->start(ShadNavigationMenuPrivate::kCloseDelayMs);
                }
                break;
            }
        case QEvent::MouseButtonPress:
            {
                const int index = d->triggerAt(static_cast<QMouseEvent*>(event)->globalPosition().toPoint());
                if (!d->hasMenu(index))
                {
                    break;
                }
                if (index == d->_openIndex)
                {
                    // 避免按下重放到菜单项后重新打开
                    d->_viewport->setAttribute(Qt::WA_NoMouseReplay);
                    d->_suppressHoverIndex = index;
                    d->_viewport->hide();
                }
                else
                {
                    d->openMenu(index, false);
                }
                return true;
            }
        case QEvent::KeyPress:
            {
                if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape)
                {
                    d->_isKeyboardClose = true;
                    d->_suppressHoverIndex = d->_openIndex;
                    d->_viewport->hide();
                    return true;
                }
                break;
            }
        default:
            {
                break;
            }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadNavigationMenu::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadNavigationMenu);
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
                d->focusTrigger(d->wrappedIndex(int(d->_items.size()) - 1, 1), Qt::TabFocusReason);
                break;
            }
        case Qt::Key_End:
            {
                d->focusTrigger(d->wrappedIndex(0, -1), Qt::TabFocusReason);
                break;
            }
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            {
                if (d->hasMenu(index))
                {
                    d->openMenu(index, true);
                }
                else
                {
                    d->onTriggerClicked(index);
                }
                break;
            }
        case Qt::Key_Down:
            {
                if (!d->hasMenu(index))
                {
                    QWidget::keyPressEvent(event);
                    return;
                }
                d->openMenu(index, true);
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
