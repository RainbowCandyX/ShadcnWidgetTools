#include "ShadLabel.h"

#include <QAbstractButton>
#include <QContextMenuEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadLabelPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTextContextMenu.h"
#include "ShadTheme.h"

ShadLabel::ShadLabel(QWidget* parent)
    : QLabel(parent), d_ptr(new ShadLabelPrivate())
{
    Q_D(ShadLabel);
    d->q_ptr = this;
    // 对应 text-sm font-medium leading-none
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);
    d->applyThemeColors();
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->applyThemeColors(); });
}

ShadLabel::ShadLabel(const QString& text, QWidget* parent)
    : ShadLabel(parent)
{
    setText(text);
}

ShadLabel::~ShadLabel()
{
}

void ShadLabel::setColorRole(ShadThemeType::ThemeColor colorRole)
{
    Q_D(ShadLabel);
    if (d->_colorRole == colorRole)
    {
        return;
    }
    d->_colorRole = colorRole;
    d->applyThemeColors();
}

ShadThemeType::ThemeColor ShadLabel::getColorRole() const
{
    Q_D(const ShadLabel);
    return d->_colorRole;
}

void ShadLabel::setBuddy(QWidget* buddy)
{
    Q_D(ShadLabel);
    if (!d->_observedBuddy.isNull())
    {
        d->_observedBuddy->removeEventFilter(this);
    }
    QLabel::setBuddy(buddy);
    d->_observedBuddy = buddy;
    if (buddy != nullptr)
    {
        buddy->installEventFilter(this);
    }
    d->applyThemeColors();
}

bool ShadLabel::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadLabel);
    if (watched == d->_observedBuddy.data() && event->type() == QEvent::EnabledChange)
    {
        d->applyThemeColors();
    }
    return QLabel::eventFilter(watched, event);
}

void ShadLabel::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget* buddyWidget = buddy();
    if (event->button() == Qt::LeftButton && buddyWidget != nullptr && buddyWidget->isEnabled() && rect().contains(event->position().toPoint()))
    {
        if (QAbstractButton* button = qobject_cast<QAbstractButton*>(buddyWidget))
        {
            button->click();
        }
        else
        {
            buddyWidget->setFocus(Qt::MouseFocusReason);
        }
    }
    QLabel::mouseReleaseEvent(event);
}

void ShadLabel::contextMenuEvent(QContextMenuEvent* event)
{
    if (!textInteractionFlags().testFlag(Qt::TextSelectableByMouse))
    {
        QLabel::contextMenuEvent(event);
        return;
    }
    const QPoint globalPos = event->reason() == QContextMenuEvent::Mouse ? event->globalPos() : mapToGlobal(rect().center());
    ShadTextContextMenu::show(this, globalPos);
    event->accept();
}
