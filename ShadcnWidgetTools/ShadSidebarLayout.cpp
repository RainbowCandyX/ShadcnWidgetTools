#include "ShadSidebarLayout.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QShortcut>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadSidebar.h"
#include "ShadSidebarLayoutPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadSidebarLayout::ShadSidebarLayout(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadSidebarLayoutPrivate())
{
    Q_D(ShadSidebarLayout);
    d->q_ptr = this;
    d->_layout = new QHBoxLayout(this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->_layout->setSpacing(0);
    d->_insetArea = new ShadSidebarInsetArea(this);
    d->_insetLayout = new QVBoxLayout(d->_insetArea);
    d->_insetLayout->setContentsMargins(0, 0, 0, 0);
    d->_insetLayout->setSpacing(0);
    d->_layout->addWidget(d->_insetArea, 1);

    // SIDEBAR_KEYBOARD_SHORTCUT：Ctrl+B，macOS 上 Qt::CTRL 对应 Cmd
    d->_shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_B), this);
    d->_shortcut->setContext(Qt::WindowShortcut);
    connect(d->_shortcut, &QShortcut::activated, this, [d]()
    {
        if (!d->_sidebar.isNull())
        {
            d->_sidebar->toggle();
        }
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this, d]()
    {
        update();
        d->_insetArea->update();
    });
}

ShadSidebarLayout::~ShadSidebarLayout()
{
    Q_D(ShadSidebarLayout);
    // 私有数据先于子对象释放，断开捕获 d 的连接
    d->disconnectSidebar();
    QObject::disconnect(d->_shortcut, nullptr, this, nullptr);
}

void ShadSidebarLayout::setSidebar(ShadSidebar* sidebar)
{
    Q_D(ShadSidebarLayout);
    if (d->_sidebar == sidebar)
    {
        return;
    }
    d->disconnectSidebar();
    if (!d->_sidebar.isNull())
    {
        d->_layout->removeWidget(d->_sidebar);
        ShadStyleHelper::clearFocusWithin(d->_sidebar);
        d->_sidebar->hide();
        d->_sidebar->deleteLater();
    }
    d->_sidebar = sidebar;
    if (sidebar != nullptr)
    {
        sidebar->setParent(this);
        sidebar->show();
    }
    d->connectSidebar();
    d->arrange();
}

ShadSidebar* ShadSidebarLayout::getSidebar() const
{
    Q_D(const ShadSidebarLayout);
    return d->_sidebar.data();
}

void ShadSidebarLayout::setContentWidget(QWidget* widget)
{
    Q_D(ShadSidebarLayout);
    if (d->_content == widget)
    {
        return;
    }
    if (!d->_content.isNull())
    {
        d->_insetLayout->removeWidget(d->_content);
        ShadStyleHelper::clearFocusWithin(d->_content);
        d->_content->hide();
        d->_content->deleteLater();
    }
    d->_content = widget;
    if (widget != nullptr)
    {
        d->_insetLayout->addWidget(widget);
    }
}

QWidget* ShadSidebarLayout::getContentWidget() const
{
    Q_D(const ShadSidebarLayout);
    return d->_content.data();
}

void ShadSidebarLayout::setShortcutEnabled(bool enabled)
{
    Q_D(ShadSidebarLayout);
    d->_shortcut->setEnabled(enabled);
}

bool ShadSidebarLayout::isShortcutEnabled() const
{
    Q_D(const ShadSidebarLayout);
    return d->_shortcut->isEnabled();
}

bool ShadSidebarLayout::event(QEvent* event)
{
    Q_D(ShadSidebarLayout);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->updateInsetMargins();
    }
    return QWidget::event(event);
}

void ShadSidebarLayout::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadSidebarLayout);
    // Inset 变体：has-data-[variant=inset]:bg-sidebar
    if (!d->_insetArea->isInset())
    {
        return;
    }
    QPainter painter(this);
    painter.fillRect(rect(), ShadStyleHelper::windowSurfaceColor(this, ShadThemeType::Sidebar, ShadStyleHelper::kSidebarEffectOpacity));
}
