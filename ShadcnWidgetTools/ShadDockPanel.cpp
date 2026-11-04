#include "ShadDockPanel.h"

#include <QAction>
#include <QPointer>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadDockFloatingWindow.h"
#include "ShadDockGroup.h"
#include "ShadDockManager.h"
#include "ShadDockManagerPrivate.h"
#include "ShadDockPanelPrivate.h"

ShadDockPanel::ShadDockPanel(const QString& id, QWidget* parent)
    : QWidget(parent), d_ptr(new ShadDockPanelPrivate())
{
    Q_D(ShadDockPanel);
    d->q_ptr = this;
    d->_id = id;
    d->_layout = new QVBoxLayout(this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->_layout->setSpacing(0);
    // 点击空白处也能激活
    setFocusPolicy(Qt::ClickFocus);
    d->_toggleViewAction = new QAction(this);
    d->_toggleViewAction->setCheckable(true);
    connect(d->_toggleViewAction, &QAction::triggered, this, [this, d](bool checked)
    {
        if (d->_manager.isNull())
        {
            d->_toggleViewAction->setChecked(false);
            return;
        }
        if (checked)
        {
            d->_manager->showPanel(this);
        }
        else if (d->_flags.testFlag(ShadDockType::PanelNotClosable))
        {
            d->_toggleViewAction->setChecked(true);
        }
        else
        {
            d->_manager->closePanel(this);
        }
    });
}

ShadDockPanel::ShadDockPanel(const QString& id, ShadIconType::IconName icon, const QString& title, QWidget* widget, QWidget* parent)
    : ShadDockPanel(id, parent)
{
    setIcon(icon);
    setTitle(title);
    setWidget(widget);
}

ShadDockPanel::~ShadDockPanel()
{
    Q_D(ShadDockPanel);
    if (!d->_manager.isNull())
    {
        d->_manager->d_func()->panelDestroyed(this);
    }
}

QString ShadDockPanel::getId() const
{
    Q_D(const ShadDockPanel);
    return d->_id;
}

void ShadDockPanel::setWidget(QWidget* widget)
{
    Q_D(ShadDockPanel);
    if (d->_widget.data() == widget)
    {
        return;
    }
    delete d->_widget.data();
    d->_widget = widget;
    if (widget != nullptr)
    {
        d->_layout->addWidget(widget);
    }
}

QWidget* ShadDockPanel::getWidget() const
{
    Q_D(const ShadDockPanel);
    return d->_widget.data();
}

void ShadDockPanel::setTitle(const QString& title)
{
    Q_D(ShadDockPanel);
    if (d->_title == title)
    {
        return;
    }
    d->_title = title;
    d->_toggleViewAction->setText(title);
    if (!d->_manager.isNull())
    {
        d->_manager->d_func()->panelChanged(this);
    }
    Q_EMIT titleChanged(title);
}

QString ShadDockPanel::getTitle() const
{
    Q_D(const ShadDockPanel);
    return d->_title;
}

void ShadDockPanel::setIcon(ShadIconType::IconName icon)
{
    Q_D(ShadDockPanel);
    if (d->_icon == icon)
    {
        return;
    }
    d->_icon = icon;
    if (!d->_manager.isNull())
    {
        d->_manager->d_func()->panelChanged(this);
    }
    Q_EMIT iconChanged(icon);
}

ShadIconType::IconName ShadDockPanel::getIcon() const
{
    Q_D(const ShadDockPanel);
    return d->_icon;
}

void ShadDockPanel::setTitleToolTip(const QString& toolTip)
{
    Q_D(ShadDockPanel);
    if (d->_titleToolTip == toolTip)
    {
        return;
    }
    d->_titleToolTip = toolTip;
    if (!d->_manager.isNull())
    {
        d->_manager->d_func()->panelChanged(this);
    }
    Q_EMIT titleToolTipChanged(toolTip);
}

QString ShadDockPanel::getTitleToolTip() const
{
    Q_D(const ShadDockPanel);
    return d->_titleToolTip;
}

void ShadDockPanel::setPanelFlags(ShadDockType::PanelFlags flags)
{
    Q_D(ShadDockPanel);
    if (d->_flags == flags)
    {
        return;
    }
    d->_flags = flags;
    if (!d->_manager.isNull())
    {
        d->_manager->d_func()->panelChanged(this);
    }
    Q_EMIT panelFlagsChanged(flags);
}

ShadDockType::PanelFlags ShadDockPanel::getPanelFlags() const
{
    Q_D(const ShadDockPanel);
    return d->_flags;
}

void ShadDockPanel::setPanelFlag(ShadDockType::PanelFlag flag, bool on)
{
    ShadDockType::PanelFlags flags = getPanelFlags();
    flags.setFlag(flag, on);
    setPanelFlags(flags);
}

void ShadDockPanel::setDefaultCloseEnabled(bool enabled)
{
    Q_D(ShadDockPanel);
    d->_isDefaultCloseEnabled = enabled;
}

bool ShadDockPanel::isDefaultCloseEnabled() const
{
    Q_D(const ShadDockPanel);
    return d->_isDefaultCloseEnabled;
}

ShadDockManager* ShadDockPanel::getDockManager() const
{
    Q_D(const ShadDockPanel);
    return d->_manager.data();
}

bool ShadDockPanel::isDocument() const
{
    Q_D(const ShadDockPanel);
    return d->_isDocument;
}

bool ShadDockPanel::isFloating() const
{
    Q_D(const ShadDockPanel);
    return !d->_group.isNull() && qobject_cast<ShadDockFloatingWindow*>(d->_group->window()) != nullptr;
}

bool ShadDockPanel::isOpen() const
{
    Q_D(const ShadDockPanel);
    return d->_isOpen;
}

QAction* ShadDockPanel::toggleViewAction() const
{
    Q_D(const ShadDockPanel);
    return d->_toggleViewAction;
}
