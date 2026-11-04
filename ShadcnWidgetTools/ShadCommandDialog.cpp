#include "ShadCommandDialog.h"

#include <QApplication>
#include <QEvent>
#include <QKeyCombination>
#include <QKeyEvent>
#include <QKeySequence>
#include <QKeySequenceEdit>
#include <QLayout>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadCommand.h"
#include "ShadCommandDialogPrivate.h"
#include "ShadCommandPrivate.h"
#include "ShadDef.h"
#include "ShadDialogPrivate.h"
#include "ShadKeySequenceEdit.h"
#include "ShadStyleHelper.h"
#include "ShadWindowFrame.h"

namespace
{
    QKeySequence& defaultShortcut()
    {
        static QKeySequence sequence(Qt::CTRL | Qt::Key_K);
        return sequence;
    }
} // namespace

ShadCommandDialogPrivate::ShadCommandDialogPrivate()
{
}

ShadCommandDialogPrivate::~ShadCommandDialogPrivate()
{
}

bool ShadCommandDialogPrivate::matchesShortcut(const QKeyEvent* event) const
{
    if (_shortcut.isEmpty() || event->isAutoRepeat())
    {
        return false;
    }
    // 录制快捷键时不响应
    const QWidget* focusWidget = QApplication::focusWidget();
    if (qobject_cast<const ShadKeySequenceEdit*>(focusWidget) != nullptr || qobject_cast<const QKeySequenceEdit*>(focusWidget) != nullptr)
    {
        return false;
    }
    const QKeyCombination combination(event->modifiers() & ~Qt::KeypadModifier, Qt::Key(event->key()));
    return QKeySequence(combination) == _shortcut;
}

ShadCommandDialog::ShadCommandDialog(QWidget* parent)
    : ShadDialog(parent), d_ptr(new ShadCommandDialogPrivate())
{
    Q_D(ShadCommandDialog);
    d->q_ptr = this;
    d->_shortcut = defaultShortcut();
    ShadStyleHelper::applyDefaultText(this, "windowTitle", windowTitle(), tr("命令面板"), [this](const QString& text) { setWindowTitle(text); });
    setTitleBarHeight(0);
    setButtonFlags(ShadWindowType::NoButton);
    setProperty(ShadWindowFrame::kResizeDisabledProperty, true);
    // 非模态才能收到外部点击
    ShadDialog::d_ptr->_isNonModalOverlay = true;
    QWidget* body = ShadDialog::d_ptr->_body;
    body->layout()->setContentsMargins(0, 0, 0, 0);
    body->layout()->setSpacing(0);

    d->_command = new ShadCommand(this);
    d->_command->d_func()->setDialogStyle(true);
    contentLayout()->addWidget(d->_command);
    resize(ShadCommandDialogPrivate::kWidth, height());
    connect(d->_command, &ShadCommand::itemTriggered, this, [this]() { accept(); });
    // 快捷键、外部点击与内容高度变化
    qApp->installEventFilter(this);
}

ShadCommandDialog::~ShadCommandDialog()
{
    qApp->removeEventFilter(this);
}

ShadCommand* ShadCommandDialog::command() const
{
    Q_D(const ShadCommandDialog);
    return d->_command;
}

void ShadCommandDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ShadStyleHelper::applyDefaultText(this, "windowTitle", windowTitle(), tr("命令面板"), [this](const QString& text) { setWindowTitle(text); });
    }
    ShadDialog::changeEvent(event);
}

void ShadCommandDialog::setResizable(bool resizable)
{
    Q_D(ShadCommandDialog);
    d->_isResizable = resizable;
    setProperty(ShadWindowFrame::kResizeDisabledProperty, !resizable);
}

bool ShadCommandDialog::isResizable() const
{
    Q_D(const ShadCommandDialog);
    return d->_isResizable;
}

void ShadCommandDialog::setShortcut(const QKeySequence& sequence)
{
    Q_D(ShadCommandDialog);
    d->_shortcut = sequence;
}

QKeySequence ShadCommandDialog::getShortcut() const
{
    Q_D(const ShadCommandDialog);
    return d->_shortcut;
}

void ShadCommandDialog::setDefaultShortcut(const QKeySequence& sequence)
{
    defaultShortcut() = sequence;
}

QKeySequence ShadCommandDialog::getDefaultShortcut()
{
    return defaultShortcut();
}

void ShadCommandDialog::setVisible(bool visible)
{
    Q_D(ShadCommandDialog);
    if (visible && !isVisible())
    {
        d->_command->setSearchText(QString());
        // 每次打开恢复默认尺寸
        ShadDialog::d_ptr->_hasAutoSize = false;
        resize(ShadCommandDialogPrivate::kWidth, height());
        setWindowModality(Qt::NonModal);
    }
    ShadDialog::setVisible(visible);
}

bool ShadCommandDialog::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadCommandDialog);
    switch (event->type())
    {
        case QEvent::KeyPress:
            {
                if (!d->matchesShortcut(static_cast<QKeyEvent*>(event)))
                {
                    break;
                }
                if (isVisible())
                {
                    reject();
                    return true;
                }
                // 其他模态窗口打开时不响应
                if (QApplication::activeModalWidget() == nullptr)
                {
                    open();
                    return true;
                }
                break;
            }
        case QEvent::MouseButtonPress:
            {
                QWidget* widget = qobject_cast<QWidget*>(watched);
                if (isVisible() && !ShadDialog::d_ptr->_isClosing && widget != nullptr && widget->window() != this)
                {
                    reject();
                    return true;
                }
                break;
            }
        case QEvent::WindowDeactivate:
            {
                if (watched == this && isVisible())
                {
                    reject();
                }
                break;
            }
        case QEvent::LayoutRequest:
            {
                if (watched == ShadDialog::d_ptr->_body && isVisible())
                {
                    QMetaObject::invokeMethod(this, [this]() { ShadDialog::d_ptr->autoResize(); }, Qt::QueuedConnection);
                }
                break;
            }
        default:
            {
                break;
            }
    }
    return ShadDialog::eventFilter(watched, event);
}
