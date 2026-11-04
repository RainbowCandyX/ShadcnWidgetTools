#include "DatePickerPage.h"

#include <QDate>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QShortcut>
#include <QString>
#include <QTime>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCalendar.h"
#include "ShadCard.h"
#include "ShadDatePicker.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadInputGroup.h"
#include "ShadLabel.h"
#include "ShadPopover.h"
#include "ShadTimePicker.h"

namespace
{
    // 字段宽度均为可视宽度，加上左右焦点环边距：w-44 / w-[212px] / w-60 / w-48 / w-32
    constexpr int kBasicWidth = 176 + 2 * ExampleLayout::kFocusRingWidth;
    constexpr int kDemoWidth = 212 + 2 * ExampleLayout::kFocusRingWidth;
    constexpr int kRangeWidth = 240 + 2 * ExampleLayout::kFocusRingWidth;
    constexpr int kInputWidth = 192 + 2 * ExampleLayout::kFocusRingWidth;
    constexpr int kTimeDateWidth = 160 + 2 * ExampleLayout::kFocusRingWidth;
    constexpr int kTimeWidth = 128 + 2 * ExampleLayout::kFocusRingWidth;
    // Field gap-3，FieldGroup 横向排列时的间距 gap-4
    constexpr int kFieldGap = 12;
    constexpr int kFieldGroupGap = 16;
    // 范围示例：今年 1 月 20 日起 20 天
    constexpr int kRangeStartMonth = 1;
    constexpr int kRangeStartDay = 20;
    constexpr int kRangeLength = 20;

    QString isoText(const QDate& date)
    {
        return date.toString(QStringLiteral("yyyy-MM-dd"));
    }

    // Field：标签在上、控件在下；标签左右留焦点环边距，与控件可视边缘对齐
    QWidget* createField(ExampleI18n::TextFunction labelText, QWidget* control, QWidget* parent)
    {
        QWidget* field = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(field);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kFieldGap - ExampleLayout::kFocusRingWidth);
        ShadLabel* label = new ShadLabel(field);
        ExampleI18n::bind(label, [label, labelText]() { label->setText(labelText()); });
        label->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, ExampleLayout::kFocusRingWidth, 0);
        label->setBuddy(control);
        control->setParent(field);
        layout->addWidget(label);
        layout->addWidget(control, 0, Qt::AlignLeft);
        return field;
    }

    // 文字由调用方按当前状态绑定
    ShadLabel* createMutedText(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }
} // namespace

DatePickerPage::DatePickerPage(QWidget* parent)
    : ExampleBasePage([] { return tr("日期选择"); }, [] { return tr("ShadDatePicker：触发框加浮层日历的日期选择器，ShadCalendar 也可单独使用。"); }, parent)
{
    // Calendar：单独使用日历
    ShadCard* calendarCard = addSection([] { return tr("日历"); }, [] { return tr("ShadCalendar 单独使用，支持键盘选择日期"); });
    ShadCalendar* calendar = new ShadCalendar(this);
    calendar->setSelectedDate(QDate::currentDate());
    ShadLabel* calendarInfo = createMutedText(this);
    const auto updateCalendarInfo = [calendar, calendarInfo]()
    {
        const QDate date = calendar->getSelectedDate();
        calendarInfo->setText(date.isValid() ? tr("已选择：%1").arg(isoText(date)) : tr("未选择"));
    };
    connect(calendar, &ShadCalendar::selectionChanged, calendarInfo, updateCalendarInfo);
    ExampleI18n::bind(calendarInfo, updateCalendarInfo);
    addItem(calendarCard, calendar);
    addItem(calendarCard, calendarInfo);

    // date-picker-basic：标签加触发框
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("点击触发框弹出日历，选择后关闭"); });
    ShadDatePicker* basicPicker = new ShadDatePicker(this);
    basicPicker->setFixedWidth(kBasicWidth);
    addItem(basicCard, createField([] { return tr("日期"); }, basicPicker, this));

    // date-picker-demo：结束侧显示下拉箭头，未选择时为 muted 色
    ShadCard* demoCard = addSection([] { return tr("带下拉箭头"); }, [] { return tr("触发框结束侧显示下拉箭头"); });
    ShadDatePicker* demoPicker = new ShadDatePicker(this);
    demoPicker->setFixedWidth(kDemoWidth);
    demoPicker->setChevronVisible(true);
    addItem(demoCard, demoPicker);

    // date-picker-dob：下拉切换年月，选择后关闭
    ShadCard* dobCard = addSection([] { return tr("出生日期"); }, [] { return tr("用下拉框切换月份与年份，并自定义显示格式"); });
    ShadDatePicker* dobPicker = new ShadDatePicker(this);
    dobPicker->setFixedWidth(kBasicWidth);
    dobPicker->setDisplayFormat(QStringLiteral("yyyy/M/d"));
    dobPicker->calendar()->setCaptionLayout(ShadCalendarType::Dropdown);
    dobPicker->calendar()->setMaximumDate(QDate::currentDate());
    addItem(dobCard, createField([] { return tr("出生日期"); }, dobPicker, this));

    // date-picker-range：两个月并排，起始侧显示日历图标
    ShadCard* rangeCard = addSection([] { return tr("日期范围"); }, [] { return tr("选择起止日期，并排显示两个月"); });
    ShadDatePicker* rangePicker = new ShadDatePicker(ShadDatePickerType::DateRange, this);
    rangePicker->setFixedWidth(kRangeWidth);
    rangePicker->setCalendarIconVisible(true);
    ExampleI18n::bind(rangePicker, [rangePicker]() { rangePicker->setDisplayFormat(tr("M月d日", "日期显示格式")); });
    rangePicker->calendar()->setMonthCount(2);
    const QDate rangeStart(QDate::currentDate().year(), kRangeStartMonth, kRangeStartDay);
    rangePicker->setDateRange(rangeStart, rangeStart.addDays(kRangeLength));
    addItem(rangeCard, createField([] { return tr("日期范围"); }, rangePicker, this));

    // date-picker-input：输入组中放日历按钮，输入框可直接输入日期
    ShadCard* inputCard = addSection([] { return tr("输入框"); }, [] { return tr("直接输入日期，或点击日历按钮选择"); });
    ShadInputGroup* inputGroup = new ShadInputGroup(this);
    inputGroup->setFixedWidth(kInputWidth);
    ShadInput* dateInput = inputGroup->getInput();
    dateInput->setPlaceholderText(QStringLiteral("2025-06-01"));
    dateInput->setText(QStringLiteral("2025-06-01"));
    ShadButton* calendarButton = inputGroup->addButton(ShadInputGroupType::InlineEnd, QString(), ShadIconType::Calendar, ShadButtonType::IconExtraSmall);
    ExampleI18n::bind(calendarButton, [calendarButton]() { calendarButton->setToolTip(tr("选择日期")); });
    ShadPopover* inputPopover = new ShadPopover(this);
    inputPopover->setContentPadding(0);
    ShadCalendar* inputCalendar = new ShadCalendar(inputPopover);
    inputPopover->contentLayout()->addWidget(inputCalendar);
    inputCalendar->setSelectedDate(QDate::fromString(dateInput->text(), QStringLiteral("yyyy-MM-dd")));
    const auto openInputCalendar = [inputPopover, inputCalendar, inputGroup]()
    {
        if (inputPopover->isVisible())
        {
            return;
        }
        const QDate selected = inputCalendar->getSelectedDate();
        inputCalendar->setCurrentMonth(selected.isValid() ? selected : QDate::currentDate());
        // 浮层宽度为日历宽度加左右 1px 边框
        inputPopover->setContentWidth(inputCalendar->sizeHint().width() + 2);
        inputPopover->popup(inputGroup, ShadPlacementType::Bottom, ShadPlacementType::End);
        inputCalendar->setFocus(Qt::PopupFocusReason);
    };
    connect(calendarButton, &ShadButton::clicked, inputPopover, openInputCalendar);
    QShortcut* openShortcut = new QShortcut(QKeySequence(Qt::Key_Down), dateInput);
    openShortcut->setContext(Qt::WidgetShortcut);
    connect(openShortcut, &QShortcut::activated, inputPopover, openInputCalendar);
    connect(inputCalendar, &ShadCalendar::dateClicked, inputPopover, [dateInput, inputPopover](const QDate& date)
    {
        dateInput->setText(isoText(date));
        inputPopover->hide();
    });
    connect(dateInput, &ShadInput::textChanged, inputCalendar, [inputCalendar](const QString& text)
    {
        const QDate date = QDate::fromString(text, QStringLiteral("yyyy-MM-dd"));
        if (date.isValid())
        {
            inputCalendar->setSelectedDate(date);
            inputCalendar->setCurrentMonth(date);
        }
    });
    addItem(inputCard, createField([] { return tr("订阅日期"); }, inputGroup, this));

    // date-picker-time：日期与时间并排
    ShadCard* timeCard = addSection([] { return tr("日期与时间"); }, [] { return tr("日期选择与时间选择并排使用"); });
    QWidget* timeRow = new QWidget(this);
    QHBoxLayout* timeLayout = new QHBoxLayout(timeRow);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->setSpacing(kFieldGroupGap - 2 * ExampleLayout::kFocusRingWidth);
    ShadDatePicker* timeDatePicker = new ShadDatePicker(timeRow);
    timeDatePicker->setFixedWidth(kTimeDateWidth);
    timeDatePicker->setChevronVisible(true);
    ShadTimePicker* timePicker = new ShadTimePicker(timeRow);
    timePicker->setFixedWidth(kTimeWidth);
    timePicker->setChevronVisible(true);
    timePicker->setTime(QTime(10, 30, 0));
    timeLayout->addWidget(createField([] { return tr("日期"); }, timeDatePicker, timeRow), 0, Qt::AlignTop);
    timeLayout->addWidget(createField([] { return tr("时间"); }, timePicker, timeRow), 0, Qt::AlignTop);
    timeLayout->addStretch(1);
    addItem(timeCard, timeRow);

    // 时间选择：时分秒与只选时分
    ShadCard* timePickerCard = addSection([] { return tr("时间选择"); }, [] { return tr("ShadTimePicker 滚动或点击选择时间，可只选时与分"); });
    QWidget* timePickerRow = new QWidget(this);
    QHBoxLayout* timePickerLayout = new QHBoxLayout(timePickerRow);
    timePickerLayout->setContentsMargins(0, 0, 0, 0);
    timePickerLayout->setSpacing(kFieldGroupGap - 2 * ExampleLayout::kFocusRingWidth);
    ShadTimePicker* secondsPicker = new ShadTimePicker(timePickerRow);
    secondsPicker->setFixedWidth(kTimeWidth);
    secondsPicker->setClockIconVisible(true);
    ShadTimePicker* minutesPicker = new ShadTimePicker(timePickerRow);
    minutesPicker->setFixedWidth(kTimeWidth);
    minutesPicker->setClockIconVisible(true);
    minutesPicker->setSecondsVisible(false);
    ShadLabel* timePickerInfo = createMutedText(timePickerRow);
    // 最近一次选择的时间文字，为空表示尚未选择
    const std::shared_ptr<QString> pickedTime = std::make_shared<QString>();
    const auto updateTimePickerInfo = [timePickerInfo, pickedTime]() { timePickerInfo->setText(pickedTime->isEmpty() ? tr("尚未选择") : tr("已选择时间：%1").arg(*pickedTime)); };
    connect(secondsPicker, &ShadTimePicker::timeChanged, timePickerInfo, [pickedTime, updateTimePickerInfo](const QTime& time)
    {
        *pickedTime = time.toString(QStringLiteral("HH:mm:ss"));
        updateTimePickerInfo();
    });
    connect(minutesPicker, &ShadTimePicker::timeChanged, timePickerInfo, [pickedTime, updateTimePickerInfo](const QTime& time)
    {
        *pickedTime = time.toString(QStringLiteral("HH:mm"));
        updateTimePickerInfo();
    });
    ExampleI18n::bind(timePickerInfo, updateTimePickerInfo);
    timePickerLayout->addWidget(createField([] { return tr("时分秒"); }, secondsPicker, timePickerRow), 0, Qt::AlignTop);
    timePickerLayout->addWidget(createField([] { return tr("时分"); }, minutesPicker, timePickerRow), 0, Qt::AlignTop);
    timePickerLayout->addWidget(timePickerInfo, 0, Qt::AlignBottom);
    timePickerLayout->addStretch(1);
    addItem(timePickerCard, timePickerRow);
}

DatePickerPage::~DatePickerPage()
{
}
