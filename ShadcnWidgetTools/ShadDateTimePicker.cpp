#include "ShadDateTimePicker.h"

#include <QDate>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QMetaObject>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>

#include "ShadCalendar.h"
#include "ShadDatePickerPrivate.h"
#include "ShadDateTimePanel.h"
#include "ShadDateTimePickerPrivate.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadPickerTrigger.h"
#include "ShadPopover.h"
#include "ShadPopoverPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    constexpr int kSampleHour = 22;
    constexpr int kSampleMinute = 58;
    constexpr int kSampleSecond = 28;
} // namespace

ShadDateTimePicker::ShadDateTimePicker(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadDateTimePickerPrivate())
{
    Q_D(ShadDateTimePicker);
    d->q_ptr = this;
    ShadStyleHelper::applyDefaultText(this, "placeholderText", d->_placeholderText, tr("选择日期时间"), [this](const QString& text) { setPlaceholderText(text); });
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->_hoverTransition = new ShadTransition(this);

    d->_popover = new ShadPopover(this);
    d->_popover->setContentPadding(0);
    d->_panel = new ShadDateTimePanel(d->_popover);
    d->_popover->contentLayout()->addWidget(d->_panel);
    connect(d->_panel, &ShadDateTimePanel::confirmed, this, [this](const QDateTime& dateTime)
    {
        hidePopup();
        setDateTime(dateTime);
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadDateTimePicker::~ShadDateTimePicker()
{
}

void ShadDateTimePicker::setDateTime(const QDateTime& dateTime)
{
    Q_D(ShadDateTimePicker);
    const QDateTime value = d->normalized(dateTime);
    if (d->_dateTime == value)
    {
        return;
    }
    d->_dateTime = value;
    updateGeometry();
    update();
    Q_EMIT dateTimeChanged(value);
}

QDateTime ShadDateTimePicker::getDateTime() const
{
    Q_D(const ShadDateTimePicker);
    return d->_dateTime;
}

void ShadDateTimePicker::setSecondsVisible(bool visible)
{
    Q_D(ShadDateTimePicker);
    if (d->_isSecondsVisible == visible)
    {
        return;
    }
    d->_isSecondsVisible = visible;
    d->_panel->setSecondsVisible(visible);
    setDateTime(d->_dateTime);
    updateGeometry();
    update();
}

bool ShadDateTimePicker::isSecondsVisible() const
{
    Q_D(const ShadDateTimePicker);
    return d->_isSecondsVisible;
}

void ShadDateTimePicker::setNowButtonVisible(bool visible)
{
    Q_D(ShadDateTimePicker);
    d->_isNowButtonVisible = visible;
    d->_panel->setNowButtonVisible(visible);
}

bool ShadDateTimePicker::isNowButtonVisible() const
{
    Q_D(const ShadDateTimePicker);
    return d->_isNowButtonVisible;
}

void ShadDateTimePicker::setPlaceholderText(const QString& text)
{
    Q_D(ShadDateTimePicker);
    d->_placeholderText = text;
    updateGeometry();
    update();
}

QString ShadDateTimePicker::getPlaceholderText() const
{
    Q_D(const ShadDateTimePicker);
    return d->_placeholderText;
}

void ShadDateTimePicker::setDisplayFormat(const QString& format)
{
    Q_D(ShadDateTimePicker);
    d->_displayFormat = format;
    updateGeometry();
    update();
}

QString ShadDateTimePicker::getDisplayFormat() const
{
    Q_D(const ShadDateTimePicker);
    return d->_displayFormat;
}

QString ShadDateTimePicker::getDisplayText() const
{
    Q_D(const ShadDateTimePicker);
    return d->displayText();
}

void ShadDateTimePicker::setCalendarIconVisible(bool visible)
{
    Q_D(ShadDateTimePicker);
    d->_isCalendarIconVisible = visible;
    updateGeometry();
    update();
}

bool ShadDateTimePicker::isCalendarIconVisible() const
{
    Q_D(const ShadDateTimePicker);
    return d->_isCalendarIconVisible;
}

void ShadDateTimePicker::setChevronVisible(bool visible)
{
    Q_D(ShadDateTimePicker);
    d->_isChevronVisible = visible;
    updateGeometry();
    update();
}

bool ShadDateTimePicker::isChevronVisible() const
{
    Q_D(const ShadDateTimePicker);
    return d->_isChevronVisible;
}

ShadCalendar* ShadDateTimePicker::calendar() const
{
    Q_D(const ShadDateTimePicker);
    return d->_panel->calendar();
}

bool ShadDateTimePicker::isPopupVisible() const
{
    Q_D(const ShadDateTimePicker);
    return d->_popover->isVisible();
}

QSize ShadDateTimePicker::sizeHint() const
{
    Q_D(const ShadDateTimePicker);
    const QString sampleText = d->formatDateTime(QDateTime(QDate(ShadDatePickerPrivate::kSampleYear, ShadDatePickerPrivate::kSampleMonth, ShadDatePickerPrivate::kSampleDay), QTime(kSampleHour, kSampleMinute, kSampleSecond)));
    return ShadPickerTrigger::sizeHint(font(), {d->_placeholderText, sampleText, d->displayText()}, d->_isCalendarIconVisible, d->_isChevronVisible);
}

QSize ShadDateTimePicker::minimumSizeHint() const
{
    return ShadPickerTrigger::minimumSizeHint();
}

void ShadDateTimePicker::clear()
{
    setDateTime(QDateTime());
}

void ShadDateTimePicker::showPopup()
{
    Q_D(ShadDateTimePicker);
    if (d->_popover->isVisible())
    {
        return;
    }
    d->_popover->setLocale(locale());
    d->_panel->setDateTime(d->_dateTime);
    d->_popover->setContentWidth(d->_panel->sizeHint().width() + 2 * ShadPopoverPrivate::kBorderWidth);
    d->_popover->popup(this, ShadPlacementType::Bottom, ShadPlacementType::Start);
    d->_panel->scrollToSelection();
    ShadDateTimePanel* panel = d->_panel;
    QMetaObject::invokeMethod(panel, [panel]() { panel->scrollToSelection(); }, Qt::QueuedConnection);
    d->_panel->calendar()->setFocus(Qt::PopupFocusReason);
}

void ShadDateTimePicker::hidePopup()
{
    Q_D(ShadDateTimePicker);
    d->_popover->hide();
}

bool ShadDateTimePicker::event(QEvent* event)
{
    switch (event->type())
    {
        case QEvent::LocaleChange:
        case QEvent::FontChange:
        case QEvent::LayoutDirectionChange:
        case QEvent::EnabledChange:
            updateGeometry();
            update();
            break;
        case QEvent::LanguageChange:
            ShadStyleHelper::applyDefaultText(this, "placeholderText", getPlaceholderText(), tr("选择日期时间"), [this](const QString& text) { setPlaceholderText(text); });
            break;
        default:
            break;
    }
    return QWidget::event(event);
}

void ShadDateTimePicker::enterEvent(QEnterEvent* event)
{
    Q_D(ShadDateTimePicker);
    d->_hoverTransition->animateTo(1.0);
    QWidget::enterEvent(event);
}

void ShadDateTimePicker::leaveEvent(QEvent* event)
{
    Q_D(ShadDateTimePicker);
    d->_hoverTransition->animateTo(0.0);
    QWidget::leaveEvent(event);
}

void ShadDateTimePicker::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadDateTimePicker);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    update();
    QWidget::focusInEvent(event);
}

void ShadDateTimePicker::focusOutEvent(QFocusEvent* event)
{
    update();
    QWidget::focusOutEvent(event);
}

void ShadDateTimePicker::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Down:
            showPopup();
            event->accept();
            return;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
}

void ShadDateTimePicker::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadDateTimePicker);
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    d->_isFocusVisible = false;
    setFocus(Qt::MouseFocusReason);
    showPopup();
    event->accept();
}

void ShadDateTimePicker::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadDateTimePicker);
    ShadPickerTrigger::Options options;
    options.text = d->displayText();
    options.placeholderText = d->_placeholderText;
    options.leadingIcon = d->_isCalendarIconVisible ? ShadIconType::CalendarClock : ShadIconType::None;
    options.hasChevron = d->_isChevronVisible;
    options.hoverProgress = d->_hoverTransition->value();
    options.showFocusRing = hasFocus() && d->_isFocusVisible;
    QPainter painter(this);
    ShadPickerTrigger::paint(&painter, this, options);
}
