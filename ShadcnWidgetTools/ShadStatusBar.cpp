#include "ShadStatusBar.h"

#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QList>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <initializer_list>

#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadStatusBarPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadStatusBar::ShadStatusBar(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadStatusBarPrivate())
{
    Q_D(ShadStatusBar);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setMinimumHeight(ShadStatusBarPrivate::kMinHeight);
    ShadStyleHelper::applyFont(this, ShadStatusBarPrivate::kFontSize, QFont::Normal);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(ShadStatusBarPrivate::kPaddingX, 1, ShadStatusBarPrivate::kPaddingX, 0);
    layout->setSpacing(ShadStatusBarPrivate::kSpacing);

    d->_startArea = new QWidget(this);
    d->_startLayout = new QHBoxLayout(d->_startArea);
    d->_startLayout->setContentsMargins(0, 0, 0, 0);
    d->_startLayout->setSpacing(ShadStatusBarPrivate::kSpacing);
    d->_startLayout->addStretch();

    d->_messageLabel = new ShadLabel(this);
    d->_messageLabel->setColorRole(ShadThemeType::MutedForeground);
    ShadStyleHelper::applyFont(d->_messageLabel, ShadStatusBarPrivate::kFontSize, QFont::Normal);
    d->_messageLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    d->_messageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    d->_messageLabel->hide();
    d->_messageLabel->installEventFilter(this);

    d->_endArea = new QWidget(this);
    d->_endLayout = new QHBoxLayout(d->_endArea);
    d->_endLayout->setContentsMargins(0, 0, 0, 0);
    d->_endLayout->setSpacing(ShadStatusBarPrivate::kSpacing);

    layout->addWidget(d->_startArea, 1);
    layout->addWidget(d->_messageLabel, 1);
    layout->addWidget(d->_endArea, 0);

    d->_messageTimer = new QTimer(this);
    d->_messageTimer->setSingleShot(true);
    connect(d->_messageTimer, &QTimer::timeout, this, &ShadStatusBar::clearMessage);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadStatusBar::~ShadStatusBar()
{
    Q_D(ShadStatusBar);
    d->_messageLabel->removeEventFilter(this);
    for (const QList<ShadStatusBarPrivate::Entry>* entries : {&d->_startEntries, &d->_endEntries})
    {
        for (const ShadStatusBarPrivate::Entry& entry : *entries)
        {
            QObject::disconnect(entry.widget, nullptr, this, nullptr);
            entry.widget->removeEventFilter(this);
        }
    }
}

void ShadStatusBar::addWidget(QWidget* widget, int stretch)
{
    Q_D(ShadStatusBar);
    if (widget == nullptr || d->contains(widget))
    {
        return;
    }
    d->insertEntry(widget, stretch, false);
}

void ShadStatusBar::addPermanentWidget(QWidget* widget, int stretch)
{
    Q_D(ShadStatusBar);
    if (widget == nullptr || d->contains(widget))
    {
        return;
    }
    d->insertEntry(widget, stretch, true);
}

ShadLabel* ShadStatusBar::addText(const QString& text)
{
    ShadLabel* label = new ShadLabel(text, this);
    label->setColorRole(ShadThemeType::MutedForeground);
    ShadStyleHelper::applyFont(label, ShadStatusBarPrivate::kFontSize, QFont::Normal);
    addWidget(label);
    return label;
}

void ShadStatusBar::removeWidget(QWidget* widget)
{
    Q_D(ShadStatusBar);
    if (widget == nullptr || !d->contains(widget))
    {
        return;
    }
    d->removeEntry(widget, false);
    widget->hide();
}

void ShadStatusBar::showMessage(const QString& message, int timeout)
{
    Q_D(ShadStatusBar);
    if (message.isEmpty())
    {
        clearMessage();
        return;
    }
    if (timeout > 0)
    {
        d->_messageTimer->start(timeout);
    }
    else
    {
        d->_messageTimer->stop();
    }
    if (d->_message == message)
    {
        return;
    }
    const bool wasShowing = !d->_message.isEmpty();
    d->_message = message;
    if (!wasShowing)
    {
        ShadStyleHelper::clearFocusWithin(d->_startArea);
        d->_startArea->hide();
        d->_messageLabel->show();
    }
    d->updateMessageText();
    Q_EMIT messageChanged(message);
}

void ShadStatusBar::clearMessage()
{
    Q_D(ShadStatusBar);
    d->_messageTimer->stop();
    if (d->_message.isEmpty())
    {
        return;
    }
    d->_message.clear();
    d->_messageLabel->hide();
    d->_messageLabel->clear();
    d->_messageLabel->setToolTip(QString());
    d->_startArea->show();
    Q_EMIT messageChanged(QString());
}

QString ShadStatusBar::currentMessage() const
{
    Q_D(const ShadStatusBar);
    return d->_message;
}

void ShadStatusBar::setSeparatorVisible(bool visible)
{
    Q_D(ShadStatusBar);
    if (d->_isSeparatorVisible == visible)
    {
        return;
    }
    d->_isSeparatorVisible = visible;
    d->updateSeparators();
}

bool ShadStatusBar::isSeparatorVisible() const
{
    Q_D(const ShadStatusBar);
    return d->_isSeparatorVisible;
}

bool ShadStatusBar::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadStatusBar);
    if (watched == d->_messageLabel)
    {
        if (event->type() == QEvent::Resize || event->type() == QEvent::FontChange)
        {
            d->updateMessageText();
        }
    }
    else if (event->type() == QEvent::ShowToParent || event->type() == QEvent::HideToParent)
    {
        d->updateSeparators();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadStatusBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(QRect(0, 0, width(), 1), shadTheme->getColor(ShadThemeType::Border));
}
