#include "ShadCalendar.h"

#include <QColor>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QLocale>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVariant>
#include <QtGlobal>
#include <algorithm>
#include <initializer_list>
#include <utility>

#include "ShadButton.h"
#include "ShadCalendarPrivate.h"
#include "ShadComboBox.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadCalendar::ShadCalendar(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadCalendarPrivate())
{
    Q_D(ShadCalendar);
    d->q_ptr = this;
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, ShadCalendarPrivate::kTextFontSize, QFont::Normal);
    d->_firstDayOfWeek = locale().firstDayOfWeek();
    const QDate today = QDate::currentDate();
    d->_currentMonth = QDate(today.year(), today.month(), 1);

    // 上一月 / 下一月按钮：ghost 变体，size-(--cell-size)
    d->_previousButton = new ShadButton(this);
    d->_nextButton = new ShadButton(this);
    for (ShadButton* button : {d->_previousButton, d->_nextButton})
    {
        button->setVariant(ShadButtonType::Ghost);
        button->setButtonSize(ShadButtonType::IconSmall);
    }
    d->_previousButton->setAccessibleName(tr("上一月"));
    d->_nextButton->setAccessibleName(tr("下一月"));
    connect(d->_previousButton, &ShadButton::clicked, this, [this, d]() { setCurrentMonth(d->_currentMonth.addMonths(-1)); });
    connect(d->_nextButton, &ShadButton::clicked, this, [this, d]() { setCurrentMonth(d->_currentMonth.addMonths(1)); });

    // 下拉标题：月份与年份
    d->_monthCombo = new ShadComboBox(this);
    d->_yearCombo = new ShadComboBox(this);
    d->_monthCombo->setComboBoxSize(ShadComboBoxType::Small);
    d->_yearCombo->setComboBoxSize(ShadComboBoxType::Small);
    d->_monthCombo->setAccessibleName(tr("月份"));
    d->_yearCombo->setAccessibleName(tr("年份"));
    const auto onDropdownChanged = [this, d]()
    {
        if (d->_isSyncingDropdowns)
        {
            return;
        }
        const int month = d->_monthCombo->currentData().toInt();
        const int year = d->_yearCombo->currentData().toInt();
        if (month >= 1 && year > 0)
        {
            setCurrentMonth(QDate(year, month, 1));
        }
    };
    connect(d->_monthCombo, &ShadComboBox::currentIndexChanged, this, onDropdownChanged);
    connect(d->_yearCombo, &ShadComboBox::currentIndexChanged, this, onDropdownChanged);
    d->rebuildDropdowns();

    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    d->refresh();
}

ShadCalendar::~ShadCalendar()
{
    Q_D(ShadCalendar);
    // 私有数据先于子组件释放，断开捕获 d 的连接
    QObject::disconnect(d->_previousButton, nullptr, this, nullptr);
    QObject::disconnect(d->_nextButton, nullptr, this, nullptr);
    QObject::disconnect(d->_monthCombo, nullptr, this, nullptr);
    QObject::disconnect(d->_yearCombo, nullptr, this, nullptr);
}

void ShadCalendar::setSelectionMode(ShadCalendarType::SelectionMode mode)
{
    Q_D(ShadCalendar);
    if (d->_mode == mode)
    {
        return;
    }
    d->_mode = mode;
    clearSelection();
}

ShadCalendarType::SelectionMode ShadCalendar::getSelectionMode() const
{
    Q_D(const ShadCalendar);
    return d->_mode;
}

void ShadCalendar::setSelectedDate(const QDate& date)
{
    Q_D(ShadCalendar);
    const QDate value = date.isValid() ? date : QDate();
    switch (d->_mode)
    {
        case ShadCalendarType::Single:
            if (d->_selectedDate == value)
            {
                return;
            }
            d->_selectedDate = value;
            break;
        case ShadCalendarType::Multiple:
            d->_selectedDates.clear();
            if (value.isValid())
            {
                d->_selectedDates.append(value);
            }
            break;
        case ShadCalendarType::Range:
            d->_rangeStart = value;
            d->_rangeEnd = QDate();
            break;
    }
    update();
    Q_EMIT selectionChanged();
}

QDate ShadCalendar::getSelectedDate() const
{
    Q_D(const ShadCalendar);
    switch (d->_mode)
    {
        case ShadCalendarType::Multiple:
            return d->_selectedDates.isEmpty() ? QDate() : d->_selectedDates.first();
        case ShadCalendarType::Range:
            return d->_rangeStart;
        case ShadCalendarType::Single:
        default:
            return d->_selectedDate;
    }
}

void ShadCalendar::setSelectedDates(const QList<QDate>& dates)
{
    Q_D(ShadCalendar);
    if (d->_mode != ShadCalendarType::Multiple)
    {
        setSelectedDate(dates.isEmpty() ? QDate() : *std::min_element(dates.cbegin(), dates.cend()));
        return;
    }
    QList<QDate> sorted;
    for (const QDate& date : dates)
    {
        if (date.isValid() && !sorted.contains(date))
        {
            sorted.append(date);
        }
    }
    std::sort(sorted.begin(), sorted.end());
    d->_selectedDates = sorted;
    update();
    Q_EMIT selectionChanged();
}

QList<QDate> ShadCalendar::getSelectedDates() const
{
    Q_D(const ShadCalendar);
    switch (d->_mode)
    {
        case ShadCalendarType::Multiple:
            return d->_selectedDates;
        case ShadCalendarType::Range:
            {
                // 范围模式返回起止之间的全部日期（含两端）
                QList<QDate> dates;
                if (!d->_rangeStart.isValid())
                {
                    return dates;
                }
                const QDate end = d->_rangeEnd.isValid() ? d->_rangeEnd : d->_rangeStart;
                for (QDate date = d->_rangeStart; date <= end; date = date.addDays(1))
                {
                    dates.append(date);
                }
                return dates;
            }
        case ShadCalendarType::Single:
        default:
            return d->_selectedDate.isValid() ? QList<QDate>{d->_selectedDate} : QList<QDate>();
    }
}

void ShadCalendar::setSelectedRange(const QDate& start, const QDate& end)
{
    Q_D(ShadCalendar);
    QDate rangeStart = start.isValid() ? start : QDate();
    QDate rangeEnd = rangeStart.isValid() && end.isValid() ? end : QDate();
    if (rangeEnd.isValid() && rangeEnd < rangeStart)
    {
        std::swap(rangeStart, rangeEnd);
    }
    d->_rangeStart = rangeStart;
    d->_rangeEnd = rangeEnd;
    update();
    Q_EMIT selectionChanged();
}

QDate ShadCalendar::getRangeStart() const
{
    Q_D(const ShadCalendar);
    return d->_rangeStart;
}

QDate ShadCalendar::getRangeEnd() const
{
    Q_D(const ShadCalendar);
    return d->_rangeEnd;
}

void ShadCalendar::clearSelection()
{
    Q_D(ShadCalendar);
    d->_selectedDate = QDate();
    d->_selectedDates.clear();
    d->_rangeStart = QDate();
    d->_rangeEnd = QDate();
    update();
    Q_EMIT selectionChanged();
}

void ShadCalendar::setCurrentMonth(const QDate& month)
{
    Q_D(ShadCalendar);
    if (!month.isValid())
    {
        return;
    }
    QDate value(month.year(), month.month(), 1);
    value = qBound(d->firstNavigableMonth(), value, d->lastNavigableMonth());
    if (d->_currentMonth == value)
    {
        return;
    }
    d->_currentMonth = value;
    d->_isSyncingDropdowns = true;
    d->syncDropdowns();
    d->_isSyncingDropdowns = false;
    d->refresh();
    Q_EMIT currentMonthChanged(value);
}

QDate ShadCalendar::getCurrentMonth() const
{
    Q_D(const ShadCalendar);
    return d->_currentMonth;
}

void ShadCalendar::setMonthCount(int count)
{
    Q_D(ShadCalendar);
    const int monthCount = qMax(1, count);
    if (d->_monthCount == monthCount)
    {
        return;
    }
    d->_monthCount = monthCount;
    // 月份数变化后可浏览的范围随之变化，重新限制当前月份
    const QDate month = qBound(d->firstNavigableMonth(), d->_currentMonth, d->lastNavigableMonth());
    if (month != d->_currentMonth)
    {
        setCurrentMonth(month);
    }
    d->refresh();
}

int ShadCalendar::getMonthCount() const
{
    Q_D(const ShadCalendar);
    return d->_monthCount;
}

void ShadCalendar::setCaptionLayout(ShadCalendarType::CaptionLayout layout)
{
    Q_D(ShadCalendar);
    if (d->_captionLayout == layout)
    {
        return;
    }
    d->_captionLayout = layout;
    d->refresh();
}

ShadCalendarType::CaptionLayout ShadCalendar::getCaptionLayout() const
{
    Q_D(const ShadCalendar);
    return d->_captionLayout;
}

void ShadCalendar::setMinimumDate(const QDate& date)
{
    Q_D(ShadCalendar);
    if (!date.isValid() || d->_minimumDate == date)
    {
        return;
    }
    d->_minimumDate = date;
    if (d->_maximumDate < date)
    {
        d->_maximumDate = date;
    }
    d->rebuildDropdowns();
    setCurrentMonth(d->_currentMonth);
    d->refresh();
}

QDate ShadCalendar::getMinimumDate() const
{
    Q_D(const ShadCalendar);
    return d->_minimumDate;
}

void ShadCalendar::setMaximumDate(const QDate& date)
{
    Q_D(ShadCalendar);
    if (!date.isValid() || d->_maximumDate == date)
    {
        return;
    }
    d->_maximumDate = date;
    if (d->_minimumDate > date)
    {
        d->_minimumDate = date;
    }
    d->rebuildDropdowns();
    setCurrentMonth(d->_currentMonth);
    d->refresh();
}

QDate ShadCalendar::getMaximumDate() const
{
    Q_D(const ShadCalendar);
    return d->_maximumDate;
}

void ShadCalendar::setFirstDayOfWeek(Qt::DayOfWeek day)
{
    Q_D(ShadCalendar);
    d->_isFirstDayExplicit = true;
    if (d->_firstDayOfWeek == day)
    {
        return;
    }
    d->_firstDayOfWeek = day;
    d->refresh();
}

Qt::DayOfWeek ShadCalendar::getFirstDayOfWeek() const
{
    Q_D(const ShadCalendar);
    return d->_firstDayOfWeek;
}

QSize ShadCalendar::sizeHint() const
{
    Q_D(const ShadCalendar);
    using Private = ShadCalendarPrivate;
    const int width = 2 * Private::kPadding + d->_monthCount * d->monthWidth() + (d->_monthCount - 1) * Private::kMonthGap;
    const int height = d->weekTop(d->maximumWeekCount()) - Private::kWeekGap + Private::kPadding;
    return QSize(width, height);
}

QSize ShadCalendar::minimumSizeHint() const
{
    return sizeHint();
}

bool ShadCalendar::event(QEvent* event)
{
    Q_D(ShadCalendar);
    switch (event->type())
    {
        case QEvent::LocaleChange:
            {
                if (!d->_isFirstDayExplicit)
                {
                    d->_firstDayOfWeek = locale().firstDayOfWeek();
                }
                d->rebuildDropdowns();
                d->refresh();
                break;
            }
        case QEvent::LayoutDirectionChange:
            {
                d->refresh();
                break;
            }
        case QEvent::LanguageChange:
            {
                d->_previousButton->setAccessibleName(tr("上一月"));
                d->_nextButton->setAccessibleName(tr("下一月"));
                d->_monthCombo->setAccessibleName(tr("月份"));
                d->_yearCombo->setAccessibleName(tr("年份"));
                break;
            }
        default:
            {
                break;
            }
    }
    return QWidget::event(event);
}

void ShadCalendar::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadCalendar);
    if (!d->_focusDate.isValid() || !d->isVisible(d->_focusDate))
    {
        d->_focusDate = d->initialFocusDate();
    }
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    const bool hasShift = event->modifiers().testFlag(Qt::ShiftModifier);
    const int weekOffset = (d->_focusDate.dayOfWeek() - static_cast<int>(d->_firstDayOfWeek) + ShadCalendarPrivate::kDaysPerWeek) % ShadCalendarPrivate::kDaysPerWeek;
    switch (event->key())
    {
        case Qt::Key_Left:
            d->moveFocus(d->_focusDate.addDays(isRightToLeft ? 1 : -1));
            break;
        case Qt::Key_Right:
            d->moveFocus(d->_focusDate.addDays(isRightToLeft ? -1 : 1));
            break;
        case Qt::Key_Up:
            d->moveFocus(d->_focusDate.addDays(-ShadCalendarPrivate::kDaysPerWeek));
            break;
        case Qt::Key_Down:
            d->moveFocus(d->_focusDate.addDays(ShadCalendarPrivate::kDaysPerWeek));
            break;
        case Qt::Key_PageUp:
            d->moveFocus(hasShift ? d->_focusDate.addYears(-1) : d->_focusDate.addMonths(-1));
            break;
        case Qt::Key_PageDown:
            d->moveFocus(hasShift ? d->_focusDate.addYears(1) : d->_focusDate.addMonths(1));
            break;
        case Qt::Key_Home:
            d->moveFocus(d->_focusDate.addDays(-weekOffset));
            break;
        case Qt::Key_End:
            d->moveFocus(d->_focusDate.addDays(ShadCalendarPrivate::kDaysPerWeek - 1 - weekOffset));
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            if (d->isSelectable(d->_focusDate))
            {
                d->selectDate(d->_focusDate);
            }
            break;
        default:
            // 其余按键（包括 Esc）交给父组件，浮层可以按 Esc 关闭
            QWidget::keyPressEvent(event);
            return;
    }
    d->_isFocusVisible = true;
    event->accept();
    update();
}

void ShadCalendar::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadCalendar);
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    const QDate date = d->dateAt(event->position().toPoint(), nullptr);
    if (!d->isSelectable(date))
    {
        QWidget::mousePressEvent(event);
        return;
    }
    // 点击不在显示范围内的日期（首尾月份网格中的上月 / 下月日期）时切换月份，使其作为本月日期显示
    d->ensureVisible(date);
    d->_focusDate = date;
    d->_isFocusVisible = false;
    setFocus(Qt::MouseFocusReason);
    d->selectDate(date);
    event->accept();
}

void ShadCalendar::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadCalendar);
    const QDate date = d->dateAt(event->position().toPoint(), nullptr);
    const QDate hoverDate = d->isSelectable(date) ? date : QDate();
    if (d->_hoverDate != hoverDate)
    {
        d->_hoverDate = hoverDate;
        setCursor(hoverDate.isValid() ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ShadCalendar::leaveEvent(QEvent* event)
{
    Q_D(ShadCalendar);
    if (d->_hoverDate.isValid())
    {
        d->_hoverDate = QDate();
        unsetCursor();
        update();
    }
    QWidget::leaveEvent(event);
}

void ShadCalendar::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadCalendar);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    if (!d->_focusDate.isValid() || !d->isVisible(d->_focusDate))
    {
        d->_focusDate = d->initialFocusDate();
    }
    update();
    QWidget::focusInEvent(event);
}

void ShadCalendar::focusOutEvent(QFocusEvent* event)
{
    update();
    QWidget::focusOutEvent(event);
}

void ShadCalendar::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadCalendar);
    QWidget::resizeEvent(event);
    d->layoutChildren();
}

void ShadCalendar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadCalendar);
    using namespace ShadStyleHelper;
    using Private = ShadCalendarPrivate;
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QDate today = QDate::currentDate();
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor mutedForeground = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor muted = shadTheme->getColor(ShadThemeType::Muted);
    const QColor primary = shadTheme->getColor(ShadThemeType::Primary);
    const QColor primaryForeground = shadTheme->getColor(ShadThemeType::PrimaryForeground);
    const QColor accent = shadTheme->getColor(ShadThemeType::Accent);
    const QColor accentForeground = shadTheme->getColor(ShadThemeType::AccentForeground);
    const bool hasCompleteRange = d->_mode == ShadCalendarType::Range && d->_rangeStart.isValid() && d->_rangeEnd.isValid();
    QFont captionFont = font();
    captionFont.setWeight(QFont::Medium);
    QFont weekdayFont = font();
    weekdayFont.setPixelSize(Private::kWeekdayFontSize);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    const qreal baseOpacity = painter.opacity();

    for (int index = 0; index < d->_monthCount; ++index)
    {
        const QDate month = d->monthAt(index);
        const int left = d->monthLeft(index);
        // 标题：年月文字，下拉模式时由下拉框显示
        if (!d->isDropdownActive())
        {
            painter.setFont(captionFont);
            painter.setPen(foreground);
            painter.drawText(d->visualRect(QRect(left, Private::kPadding, d->monthWidth(), Private::kCaptionHeight)), Qt::AlignCenter, d->captionText(month));
        }
        // 星期名
        painter.setFont(weekdayFont);
        painter.setPen(mutedForeground);
        for (int column = 0; column < Private::kDaysPerWeek; ++column)
        {
            const QRect weekdayRect(left + column * Private::kCellSize, d->weekdayTop(), Private::kCellSize, Private::kWeekdayHeight);
            painter.drawText(d->visualRect(weekdayRect), Qt::AlignCenter, d->weekdayText(d->dayOfWeekAt(column)));
        }

        painter.setFont(font());
        const QDate start = d->gridStart(month);
        const int weeks = d->weekCount(month);
        for (int week = 0; week < weeks; ++week)
        {
            // 范围中间的背景：同一行内连续的本月日期连成一条 bg-muted 圆角条，两端为行首 / 行尾或范围端点
            if (hasCompleteRange)
            {
                int runStart = -1;
                for (int column = 0; column <= Private::kDaysPerWeek; ++column)
                {
                    bool isInRange = false;
                    if (column < Private::kDaysPerWeek)
                    {
                        const QDate date = start.addDays(week * Private::kDaysPerWeek + column);
                        isInRange = date.month() == month.month() && date >= d->_rangeStart && date <= d->_rangeEnd;
                    }
                    if (isInRange && runStart < 0)
                    {
                        runStart = column;
                    }
                    else if (!isInRange && runStart >= 0)
                    {
                        const QRect runRect = d->cellRect(index, week, runStart).united(d->cellRect(index, week, column - 1));
                        fillRoundedRect(&painter, QRectF(d->visualRect(runRect)), radius, muted);
                        runStart = -1;
                    }
                }
            }
            for (int column = 0; column < Private::kDaysPerWeek; ++column)
            {
                const QDate date = start.addDays(week * Private::kDaysPerWeek + column);
                const QRectF cell(d->visualRect(d->cellRect(index, week, column)));
                const bool isInMonth = date.month() == month.month();
                const bool isSelectable = d->isSelectable(date);
                bool isPrimary = false;
                bool isRangeMiddle = false;
                // 不属于当月的日期不显示选中状态，避免并排多个月时同一天出现两次选中
                if (isInMonth)
                {
                    switch (d->_mode)
                    {
                        case ShadCalendarType::Single:
                            isPrimary = date == d->_selectedDate;
                            break;
                        case ShadCalendarType::Multiple:
                            isPrimary = d->_selectedDates.contains(date);
                            break;
                        case ShadCalendarType::Range:
                            isPrimary = date == d->_rangeStart || (d->_rangeEnd.isValid() && date == d->_rangeEnd);
                            isRangeMiddle = hasCompleteRange && date > d->_rangeStart && date < d->_rangeEnd;
                            break;
                    }
                }
                QColor textColor = isInMonth ? foreground : mutedForeground;
                if (isPrimary)
                {
                    fillRoundedRect(&painter, cell, radius, primary);
                    textColor = primaryForeground;
                }
                else if (!isRangeMiddle)
                {
                    if (date == d->_hoverDate && isSelectable)
                    {
                        // 对应 ghost 按钮的 hover:bg-accent
                        fillRoundedRect(&painter, cell, radius, accent);
                        textColor = isInMonth ? accentForeground : mutedForeground;
                    }
                    else if (date == today && isInMonth)
                    {
                        fillRoundedRect(&painter, cell, radius, muted);
                    }
                }
                painter.setOpacity(isSelectable ? baseOpacity : baseOpacity * kDisabledOpacity);
                painter.setPen(isSelectable ? textColor : mutedForeground);
                painter.drawText(cell, Qt::AlignCenter, d->dayText(date));
                painter.setOpacity(baseOpacity);
            }
        }
    }

    // 键盘焦点：border-ring + ring-[3px] ring-ring/50
    int focusMonth = 0;
    int focusWeek = 0;
    int focusColumn = 0;
    if (hasFocus() && d->_isFocusVisible && d->locateDate(d->_focusDate, &focusMonth, &focusWeek, &focusColumn))
    {
        const QRectF focusRect(d->visualRect(d->cellRect(focusMonth, focusWeek, focusColumn)));
        strokeRoundedRect(&painter, focusRect, radius, shadTheme->getColor(ShadThemeType::Ring));
        drawFocusRing(&painter, focusRect, radius, focusRingColor(false));
    }
}
