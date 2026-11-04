#include "ShadDatePicker.h"

#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QtGlobal>
#include <utility>

#include "ShadCalendar.h"
#include "ShadDatePickerPrivate.h"
#include "ShadIconType.h"
#include "ShadPickerTrigger.h"
#include "ShadPopover.h"
#include "ShadPopoverPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

ShadDatePicker::ShadDatePicker(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadDatePickerPrivate())
{
    Q_D(ShadDatePicker);
    d->q_ptr = this;
    ShadStyleHelper::applyDefaultText(this, "placeholderText", d->_placeholderText, tr("选择日期"), [this](const QString& text) { setPlaceholderText(text); });
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->_hoverTransition = new ShadTransition(this);

    // PopoverContent className="w-auto p-0"：内边距由日历自身的 p-3 提供
    d->_popover = new ShadPopover(this);
    d->_popover->setContentPadding(0);
    d->_calendar = new ShadCalendar(d->_popover);
    d->_popover->contentLayout()->addWidget(d->_calendar);
    connect(d->_calendar, &ShadCalendar::dateClicked, this, [d]() { d->onCalendarDateClicked(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    d->syncCalendar();
}

ShadDatePicker::ShadDatePicker(ShadDatePickerType::Mode mode, QWidget* parent)
    : ShadDatePicker(parent)
{
    setMode(mode);
}

ShadDatePicker::~ShadDatePicker()
{
    Q_D(ShadDatePicker);
    // 私有数据先于子组件释放，断开捕获 d 的连接
    QObject::disconnect(d->_calendar, nullptr, this, nullptr);
}

void ShadDatePicker::setMode(ShadDatePickerType::Mode mode)
{
    Q_D(ShadDatePicker);
    if (d->_mode == mode)
    {
        return;
    }
    d->_mode = mode;
    d->_date = QDate();
    d->_rangeStart = QDate();
    d->_rangeEnd = QDate();
    d->syncCalendar();
    updateGeometry();
    update();
}

ShadDatePickerType::Mode ShadDatePicker::getMode() const
{
    Q_D(const ShadDatePicker);
    return d->_mode;
}

void ShadDatePicker::setDate(const QDate& date)
{
    Q_D(ShadDatePicker);
    const QDate value = date.isValid() ? date : QDate();
    if (d->_mode != ShadDatePickerType::SingleDate || d->_date == value)
    {
        return;
    }
    d->_date = value;
    d->syncCalendar();
    updateGeometry();
    update();
    Q_EMIT dateChanged(value);
}

QDate ShadDatePicker::getDate() const
{
    Q_D(const ShadDatePicker);
    return d->_date;
}

void ShadDatePicker::setDateRange(const QDate& start, const QDate& end)
{
    Q_D(ShadDatePicker);
    if (d->_mode != ShadDatePickerType::DateRange)
    {
        return;
    }
    QDate rangeStart = start.isValid() ? start : QDate();
    QDate rangeEnd = rangeStart.isValid() && end.isValid() ? end : QDate();
    if (rangeEnd.isValid() && rangeEnd < rangeStart)
    {
        std::swap(rangeStart, rangeEnd);
    }
    if (d->_rangeStart == rangeStart && d->_rangeEnd == rangeEnd)
    {
        return;
    }
    d->_rangeStart = rangeStart;
    d->_rangeEnd = rangeEnd;
    d->syncCalendar();
    updateGeometry();
    update();
    Q_EMIT rangeChanged(rangeStart, rangeEnd);
}

QDate ShadDatePicker::getRangeStart() const
{
    Q_D(const ShadDatePicker);
    return d->_rangeStart;
}

QDate ShadDatePicker::getRangeEnd() const
{
    Q_D(const ShadDatePicker);
    return d->_rangeEnd;
}

void ShadDatePicker::clear()
{
    Q_D(ShadDatePicker);
    if (d->_mode == ShadDatePickerType::SingleDate)
    {
        setDate(QDate());
    }
    else
    {
        setDateRange(QDate(), QDate());
    }
}

void ShadDatePicker::setPlaceholderText(const QString& text)
{
    Q_D(ShadDatePicker);
    d->_placeholderText = text;
    updateGeometry();
    update();
}

QString ShadDatePicker::getPlaceholderText() const
{
    Q_D(const ShadDatePicker);
    return d->_placeholderText;
}

void ShadDatePicker::setDisplayFormat(const QString& format)
{
    Q_D(ShadDatePicker);
    d->_displayFormat = format;
    updateGeometry();
    update();
}

QString ShadDatePicker::getDisplayFormat() const
{
    Q_D(const ShadDatePicker);
    return d->_displayFormat;
}

QString ShadDatePicker::getDisplayText() const
{
    Q_D(const ShadDatePicker);
    return d->displayText();
}

void ShadDatePicker::setCalendarIconVisible(bool visible)
{
    Q_D(ShadDatePicker);
    d->_isCalendarIconVisible = visible;
    updateGeometry();
    update();
}

bool ShadDatePicker::isCalendarIconVisible() const
{
    Q_D(const ShadDatePicker);
    return d->_isCalendarIconVisible;
}

void ShadDatePicker::setChevronVisible(bool visible)
{
    Q_D(ShadDatePicker);
    d->_isChevronVisible = visible;
    updateGeometry();
    update();
}

bool ShadDatePicker::isChevronVisible() const
{
    Q_D(const ShadDatePicker);
    return d->_isChevronVisible;
}

void ShadDatePicker::setCloseOnSelect(bool closeOnSelect)
{
    Q_D(ShadDatePicker);
    d->_isCloseOnSelect = closeOnSelect;
}

bool ShadDatePicker::isCloseOnSelect() const
{
    Q_D(const ShadDatePicker);
    return d->_isCloseOnSelect;
}

ShadCalendar* ShadDatePicker::calendar() const
{
    Q_D(const ShadDatePicker);
    return d->_calendar;
}

void ShadDatePicker::showPopup()
{
    Q_D(ShadDatePicker);
    if (d->_popover->isVisible())
    {
        return;
    }
    // 浮层是独立窗口，不继承触发框的 locale，这里同步；布局方向由 popup 按锚点同步
    d->_popover->setLocale(locale());
    // 对应 defaultMonth={date}：打开时显示已选日期所在的月份，没有选择时显示今天所在的月份
    const QDate selected = d->_mode == ShadDatePickerType::SingleDate ? d->_date : d->_rangeStart;
    d->_calendar->setCurrentMonth(selected.isValid() ? selected : QDate::currentDate());
    // 浮层宽度为日历宽度加左右边框（w-auto）
    d->_popover->setContentWidth(d->_calendar->sizeHint().width() + 2 * ShadPopoverPrivate::kBorderWidth);
    d->_popover->popup(this, ShadPlacementType::Bottom, ShadPlacementType::Start);
    d->_calendar->setFocus(Qt::PopupFocusReason);
}

void ShadDatePicker::hidePopup()
{
    Q_D(ShadDatePicker);
    d->_popover->hide();
}

bool ShadDatePicker::isPopupVisible() const
{
    Q_D(const ShadDatePicker);
    return d->_popover->isVisible();
}

QSize ShadDatePicker::sizeHint() const
{
    Q_D(const ShadDatePicker);
    // 文字宽度取占位文字、当前文字与示例日期中最宽的一个，选择日期后宽度不跳动
    const QDate sample(ShadDatePickerPrivate::kSampleYear, ShadDatePickerPrivate::kSampleMonth, ShadDatePickerPrivate::kSampleDay);
    const QString sampleText = d->_mode == ShadDatePickerType::SingleDate ? d->formatDate(sample) : QStringLiteral("%1 - %2").arg(d->formatDate(sample), d->formatDate(sample));
    return ShadPickerTrigger::sizeHint(font(), {d->_placeholderText, sampleText, d->displayText()}, d->_isCalendarIconVisible, d->_isChevronVisible);
}

QSize ShadDatePicker::minimumSizeHint() const
{
    return ShadPickerTrigger::minimumSizeHint();
}

bool ShadDatePicker::event(QEvent* event)
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
            ShadStyleHelper::applyDefaultText(this, "placeholderText", getPlaceholderText(), tr("选择日期"), [this](const QString& text) { setPlaceholderText(text); });
            break;
        default:
            break;
    }
    return QWidget::event(event);
}

void ShadDatePicker::enterEvent(QEnterEvent* event)
{
    Q_D(ShadDatePicker);
    d->_hoverTransition->animateTo(1.0);
    QWidget::enterEvent(event);
}

void ShadDatePicker::leaveEvent(QEvent* event)
{
    Q_D(ShadDatePicker);
    d->_hoverTransition->animateTo(0.0);
    QWidget::leaveEvent(event);
}

void ShadDatePicker::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadDatePicker);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    update();
    QWidget::focusInEvent(event);
}

void ShadDatePicker::focusOutEvent(QFocusEvent* event)
{
    update();
    QWidget::focusOutEvent(event);
}

void ShadDatePicker::keyPressEvent(QKeyEvent* event)
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

void ShadDatePicker::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadDatePicker);
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    // 浮层打开时点击触发框由浮层自身关闭，并且这次按下不会传到这里
    d->_isFocusVisible = false;
    setFocus(Qt::MouseFocusReason);
    showPopup();
    event->accept();
}

void ShadDatePicker::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadDatePicker);
    ShadPickerTrigger::Options options;
    options.text = d->displayText();
    options.placeholderText = d->_placeholderText;
    options.leadingIcon = d->_isCalendarIconVisible ? ShadIconType::Calendar : ShadIconType::None;
    options.hasChevron = d->_isChevronVisible;
    options.hoverProgress = d->_hoverTransition->value();
    options.showFocusRing = hasFocus() && d->_isFocusVisible;
    QPainter painter(this);
    ShadPickerTrigger::paint(&painter, this, options);
}
