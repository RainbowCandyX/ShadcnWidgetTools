#include "ShadStatusBarPrivate.h"

#include <QHBoxLayout>
#include <QList>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <initializer_list>

#include "ShadLabel.h"
#include "ShadSeparator.h"
#include "ShadStatusBar.h"
#include "ShadStyleHelper.h"

ShadStatusBarPrivate::ShadStatusBarPrivate()
{
}

ShadStatusBarPrivate::~ShadStatusBarPrivate()
{
}

bool ShadStatusBarPrivate::contains(const QWidget* widget) const
{
    for (const QList<Entry>* entries : {&_startEntries, &_endEntries})
    {
        for (const Entry& entry : *entries)
        {
            if (entry.widget == widget)
            {
                return true;
            }
        }
    }
    return false;
}

void ShadStatusBarPrivate::insertEntry(QWidget* widget, int stretch, bool isPermanent)
{
    Q_Q(ShadStatusBar);
    QWidget* area = isPermanent ? _endArea : _startArea;
    QHBoxLayout* layout = isPermanent ? _endLayout : _startLayout;
    Entry entry;
    entry.widget = widget;
    entry.separator = new ShadSeparator(Qt::Vertical, area);
    entry.separator->setFixedHeight(kSeparatorHeight);
    const int index = isPermanent ? layout->count() : layout->count() - 1;
    layout->insertWidget(index, entry.separator, 0, Qt::AlignVCenter);
    layout->insertWidget(index + 1, widget, stretch, Qt::AlignVCenter);
    widget->installEventFilter(q);
    QObject::connect(widget, &QObject::destroyed, q, [this, widget]() { removeEntry(widget, true); });
    (isPermanent ? _endEntries : _startEntries).append(entry);
    updateSeparators();
}

void ShadStatusBarPrivate::removeEntry(QWidget* widget, bool isDestroyed)
{
    Q_Q(ShadStatusBar);
    for (QList<Entry>* entries : {&_startEntries, &_endEntries})
    {
        for (int i = 0; i < entries->size(); ++i)
        {
            const Entry entry = entries->at(i);
            if (entry.widget != widget)
            {
                continue;
            }
            entries->removeAt(i);
            delete entry.separator;
            if (!isDestroyed)
            {
                widget->removeEventFilter(q);
                QObject::disconnect(widget, nullptr, q, nullptr);
                (entries == &_endEntries ? _endLayout : _startLayout)->removeWidget(widget);
            }
            updateSeparators();
            return;
        }
    }
}

void ShadStatusBarPrivate::updateSeparators()
{
    for (const QList<Entry>* entries : {&_startEntries, &_endEntries})
    {
        bool hasVisibleBefore = false;
        for (const Entry& entry : *entries)
        {
            const bool isVisible = !entry.widget->isHidden();
            entry.separator->setVisible(_isSeparatorVisible && isVisible && hasVisibleBefore);
            hasVisibleBefore = hasVisibleBefore || isVisible;
        }
    }
}

void ShadStatusBarPrivate::updateMessageText()
{
    const QString text = ShadStyleHelper::elideText(_messageLabel->font(), _message, _messageLabel->contentsRect().width());
    _messageLabel->setText(text);
    _messageLabel->setToolTip(text == _message ? QString() : _message);
}
