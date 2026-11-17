#include "ShadAutoComplete.h"

#include <QAbstractItemModel>
#include <QEvent>
#include <QFocusEvent>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>
#include <QStringListModel>
#include <QWheelEvent>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadAutoCompletePopup.h"
#include "ShadAutoCompletePrivate.h"

ShadAutoComplete::ShadAutoComplete(QWidget* parent)
    : ShadInput(parent), _private(new ShadAutoCompletePrivate())
{
    ShadAutoCompletePrivate* d = _private.data();
    d->q = this;
    d->_defaultModel = new QStringListModel(this);
    d->_popup = new ShadAutoCompletePopup(this);
    d->_popup->setMaxVisibleItems(d->_maxVisibleItems);
    connect(d->_popup, &ShadAutoCompletePopup::rowClicked, this, [this, d](int row)
    {
        d->select(row);
        activateWindow();
        setFocus(Qt::PopupFocusReason);
    });
    connect(d->_popup, &ShadAutoCompletePopup::popupHidden, this, [d]() { d->watchOutside(false); });
    connect(this, &QLineEdit::textEdited, this, [this, d](const QString& text)
    {
        Q_EMIT searchTextChanged(text);
        if (text.isEmpty())
        {
            hidePopup();
            return;
        }
        d->refresh(true);
    });
    connect(this, &QLineEdit::textChanged, this, [this](const QString& text)
    {
        if (text.isEmpty())
        {
            hidePopup();
        }
    });
    d->connectModel();
}

ShadAutoComplete::~ShadAutoComplete()
{
    ShadAutoCompletePrivate* d = _private.data();
    QObject::disconnect(d->_popup, nullptr, this, nullptr);
    d->_popup->hide();
    d->watchOutside(false);
    d->disconnectModel();
}

void ShadAutoComplete::setOptions(const QStringList& options)
{
    ShadAutoCompletePrivate* d = _private.data();
    if (!d->_customModel.isNull())
    {
        d->disconnectModel();
        d->_customModel = nullptr;
        d->connectModel();
    }
    d->_defaultModel->setStringList(options);
}

QStringList ShadAutoComplete::getOptions() const
{
    return _private->_defaultModel->stringList();
}

void ShadAutoComplete::setModel(QAbstractItemModel* model)
{
    ShadAutoCompletePrivate* d = _private.data();
    if (d->_customModel == model)
    {
        return;
    }
    d->disconnectModel();
    d->_customModel = model;
    d->connectModel();
    if (d->_popup->isVisible())
    {
        d->refresh(false);
    }
}

QAbstractItemModel* ShadAutoComplete::model() const
{
    return _private->activeModel();
}

void ShadAutoComplete::setFilterMode(ShadAutoCompleteType::FilterMode mode)
{
    ShadAutoCompletePrivate* d = _private.data();
    if (d->_filterMode == mode)
    {
        return;
    }
    d->_filterMode = mode;
    if (d->_popup->isVisible())
    {
        d->refresh(false);
    }
}

ShadAutoCompleteType::FilterMode ShadAutoComplete::getFilterMode() const
{
    return _private->_filterMode;
}

void ShadAutoComplete::setMaxVisibleItems(int count)
{
    ShadAutoCompletePrivate* d = _private.data();
    d->_maxVisibleItems = qMax(1, count);
    d->_popup->setMaxVisibleItems(d->_maxVisibleItems);
}

int ShadAutoComplete::getMaxVisibleItems() const
{
    return _private->_maxVisibleItems;
}

bool ShadAutoComplete::isPopupVisible() const
{
    return _private->_popup->isVisible();
}

void ShadAutoComplete::showPopup()
{
    _private->refresh(true);
}

void ShadAutoComplete::hidePopup()
{
    _private->_popup->hide();
}

bool ShadAutoComplete::eventFilter(QObject* watched, QEvent* event)
{
    ShadAutoCompletePrivate* d = _private.data();
    if (d->_isWatchingOutside)
    {
        const QEvent::Type type = event->type();
        if (type == QEvent::MouseButtonPress || type == QEvent::MouseButtonDblClick || type == QEvent::Wheel)
        {
            const QPoint globalPos = type == QEvent::Wheel ? static_cast<QWheelEvent*>(event)->globalPosition().toPoint() : static_cast<QMouseEvent*>(event)->globalPosition().toPoint();
            if (!d->_popup->geometry().contains(globalPos) && !rect().contains(mapFromGlobal(globalPos)))
            {
                hidePopup();
            }
        }
        else if (watched == d->_watchedWindow)
        {
            if (type == QEvent::Move || type == QEvent::Resize)
            {
                d->_popup->updatePopupGeometry();
            }
            else if (type == QEvent::WindowDeactivate && !d->_popup->underMouse())
            {
                hidePopup();
            }
        }
    }
    return ShadInput::eventFilter(watched, event);
}

void ShadAutoComplete::keyPressEvent(QKeyEvent* event)
{
    ShadAutoCompletePrivate* d = _private.data();
    if (d->_popup->isVisible())
    {
        switch (event->key())
        {
            case Qt::Key_Up:
            case Qt::Key_Down:
                d->_popup->moveHighlight(event->key() == Qt::Key_Down ? 1 : -1);
                event->accept();
                return;
            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
                {
                    const int step = qMax(1, d->_maxVisibleItems - 1);
                    d->_popup->moveHighlight(event->key() == Qt::Key_PageDown ? step : -step);
                    event->accept();
                    return;
                }
            case Qt::Key_Return:
            case Qt::Key_Enter:
                {
                    const int row = d->_popup->highlightedRow();
                    if (row >= 0)
                    {
                        d->select(row);
                        event->accept();
                        return;
                    }
                    hidePopup();
                    break;
                }
            case Qt::Key_Escape:
                hidePopup();
                event->accept();
                return;
            case Qt::Key_Tab:
            case Qt::Key_Backtab:
                hidePopup();
                break;
            default:
                break;
        }
    }
    else if (event->key() == Qt::Key_Down && event->modifiers() == Qt::NoModifier)
    {
        d->refresh(true);
        if (d->_popup->isVisible())
        {
            event->accept();
            return;
        }
    }
    ShadInput::keyPressEvent(event);
}

void ShadAutoComplete::focusOutEvent(QFocusEvent* event)
{
    ShadAutoCompletePrivate* d = _private.data();
    if (!d->_popup->underMouse())
    {
        hidePopup();
    }
    ShadInput::focusOutEvent(event);
}

void ShadAutoComplete::hideEvent(QHideEvent* event)
{
    hidePopup();
    ShadInput::hideEvent(event);
}
