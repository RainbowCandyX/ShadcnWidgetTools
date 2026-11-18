#include "ShadDateTimePanel.h"

#include <QColor>
#include <QDate>
#include <QEvent>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QString>
#include <QTime>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>

#include "ShadButton.h"
#include "ShadCalendar.h"
#include "ShadDef.h"
#include "ShadTheme.h"
#include "ShadTimePanel.h"

namespace
{
    constexpr int kFooterPadding = 8;
    constexpr int kSeparatorWidth = 1;
} // namespace

ShadDateTimePanel::ShadDateTimePanel(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    _body = new QWidget(this);
    QHBoxLayout* bodyLayout = new QHBoxLayout(_body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(kSeparatorWidth);
    _calendar = new ShadCalendar(_body);
    _timePanel = new ShadTimePanel(_body);
    _timePanel->setFooterVisible(false);
    bodyLayout->addWidget(_calendar, 0, Qt::AlignTop);
    bodyLayout->addWidget(_timePanel, 0, Qt::AlignTop);
    mainLayout->addWidget(_body);

    _footer = new QWidget(this);
    QHBoxLayout* footerLayout = new QHBoxLayout(_footer);
    footerLayout->setContentsMargins(kFooterPadding, kFooterPadding, kFooterPadding, kFooterPadding);
    _nowButton = new ShadButton(QString(), _footer);
    _nowButton->setVariant(ShadButtonType::Ghost);
    _nowButton->setButtonSize(ShadButtonType::Small);
    _nowButton->setAutoDefault(false);
    _confirmButton = new ShadButton(QString(), _footer);
    _confirmButton->setButtonSize(ShadButtonType::Small);
    _confirmButton->setAutoDefault(false);
    footerLayout->addWidget(_nowButton);
    footerLayout->addStretch(1);
    footerLayout->addWidget(_confirmButton);
    mainLayout->addWidget(_footer);
    retranslate();

    connect(_calendar, &ShadCalendar::dateClicked, this, [this]()
    {
        if (!_timePanel->time().isValid())
        {
            _timePanel->setTime(QTime(0, 0, 0));
            _timePanel->scrollToSelection();
        }
        updateConfirmButton();
    });
    connect(_timePanel, &ShadTimePanel::timeEdited, this, [this]()
    {
        if (!_calendar->getSelectedDate().isValid())
        {
            _calendar->setSelectedDate(QDate::currentDate());
        }
        updateConfirmButton();
    });
    connect(_timePanel, &ShadTimePanel::confirmed, this, [this]() { confirm(); });
    connect(_confirmButton, &ShadButton::clicked, this, [this]() { confirm(); });
    connect(_nowButton, &ShadButton::clicked, this, [this]()
    {
        setDateTime(QDateTime::currentDateTime());
        confirm();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    updateConfirmButton();
}

ShadDateTimePanel::~ShadDateTimePanel()
{
}

ShadCalendar* ShadDateTimePanel::calendar() const
{
    return _calendar;
}

void ShadDateTimePanel::setSecondsVisible(bool visible)
{
    _timePanel->setSecondsVisible(visible);
    updateConfirmButton();
}

void ShadDateTimePanel::setNowButtonVisible(bool visible)
{
    _nowButton->setVisible(visible);
}

void ShadDateTimePanel::setDateTime(const QDateTime& dateTime)
{
    const bool isValid = dateTime.isValid();
    _calendar->setSelectedDate(isValid ? dateTime.date() : QDate());
    _calendar->setCurrentMonth(isValid ? dateTime.date() : QDate::currentDate());
    _timePanel->setTime(isValid ? dateTime.time() : QTime());
    updateConfirmButton();
}

QDateTime ShadDateTimePanel::dateTime() const
{
    const QDate date = _calendar->getSelectedDate();
    const QTime time = _timePanel->time();
    if (!date.isValid() || !time.isValid())
    {
        return QDateTime();
    }
    return QDateTime(date, time);
}

void ShadDateTimePanel::scrollToSelection()
{
    _timePanel->scrollToSelection();
}

void ShadDateTimePanel::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}

void ShadDateTimePanel::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    const QColor borderColor = shadTheme->getColor(ShadThemeType::Border);
    const QRect bodyRect = _body->geometry();
    const int separatorX = layoutDirection() == Qt::RightToLeft ? _calendar->geometry().left() - kSeparatorWidth : _calendar->geometry().right() + 1;
    painter.fillRect(QRect(bodyRect.left() + separatorX, bodyRect.top(), kSeparatorWidth, bodyRect.height()), borderColor);
    painter.fillRect(QRect(0, _footer->y(), width(), kSeparatorWidth), borderColor);
}

void ShadDateTimePanel::retranslate()
{
    _nowButton->setText(tr("此刻"));
    _confirmButton->setText(tr("确定"));
}

void ShadDateTimePanel::confirm()
{
    const QDateTime value = dateTime();
    if (value.isValid())
    {
        Q_EMIT confirmed(value);
    }
}

void ShadDateTimePanel::updateConfirmButton()
{
    _confirmButton->setEnabled(dateTime().isValid());
}
