#include "DateTimePickerPage.h"

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QTime>
#include <QWidget>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadCalendar.h"
#include "ShadCard.h"
#include "ShadDateTimePicker.h"
#include "ShadDef.h"
#include "ShadLabel.h"

namespace
{
    constexpr int kPickerWidth = 260 + 2 * ExampleLayout::kFocusRingWidth;
    constexpr int kSelectableDays = 30;
} // namespace

DateTimePickerPage::DateTimePickerPage(QWidget* parent)
    : ExampleBasePage([] { return tr("日期时间选择"); }, [] { return tr("ShadDateTimePicker：在一个浮层中选择日期与时间，左侧为日历，右侧为时、分、秒三列，点击确定后写入。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("点击触发框打开浮层，选择日期与时间后点击确定；按 Esc 或点击外部关闭时不保存"); });
    ShadDateTimePicker* basicPicker = new ShadDateTimePicker(this);
    basicPicker->setFixedWidth(kPickerWidth);
    ShadLabel* valueLabel = new ShadLabel(this);
    valueLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updateValueLabel = [basicPicker, valueLabel]() { valueLabel->setText(basicPicker->getDateTime().isValid() ? tr("当前选择: %1").arg(basicPicker->getDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))) : tr("当前选择: 无")); };
    ExampleI18n::bind(valueLabel, updateValueLabel);
    connect(basicPicker, &ShadDateTimePicker::dateTimeChanged, valueLabel, updateValueLabel);
    addItem(basicCard, basicPicker);
    addItem(basicCard, valueLabel);

    ShadCard* formatCard = addSection([] { return tr("图标与格式"); }, [] { return tr("setCalendarIconVisible、setChevronVisible 显示图标与下拉箭头；setSecondsVisible(false) 隐藏秒，setDisplayFormat 自定义显示格式"); });
    ShadDateTimePicker* formatPicker = new ShadDateTimePicker(this);
    formatPicker->setFixedWidth(kPickerWidth);
    formatPicker->setCalendarIconVisible(true);
    formatPicker->setChevronVisible(true);
    formatPicker->setSecondsVisible(false);
    formatPicker->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
    formatPicker->setDateTime(QDateTime(QDate::currentDate(), QTime(9, 30)));
    addItem(formatCard, formatPicker);

    ShadCard* rangeCard = addSection([] { return tr("可选范围"); }, [] { return tr("calendar() 取得浮层中的日历，限制只能选择今天起 30 天内的日期；setNowButtonVisible(false) 隐藏此刻按钮"); });
    ShadDateTimePicker* rangePicker = new ShadDateTimePicker(this);
    rangePicker->setFixedWidth(kPickerWidth);
    rangePicker->setCalendarIconVisible(true);
    rangePicker->setNowButtonVisible(false);
    rangePicker->calendar()->setMinimumDate(QDate::currentDate());
    rangePicker->calendar()->setMaximumDate(QDate::currentDate().addDays(kSelectableDays));
    ExampleI18n::bind(rangePicker, [rangePicker]() { rangePicker->setPlaceholderText(tr("选择预约时间")); });
    addItem(rangeCard, rangePicker);

    ShadCard* disabledCard = addSection([] { return tr("禁用"); }, [] { return tr("setEnabled(false) 时不能打开浮层"); });
    ShadDateTimePicker* disabledPicker = new ShadDateTimePicker(this);
    disabledPicker->setFixedWidth(kPickerWidth);
    disabledPicker->setDateTime(QDateTime(QDate::currentDate(), QTime(18, 0)));
    disabledPicker->setEnabled(false);
    addItem(disabledCard, disabledPicker);
}

DateTimePickerPage::~DateTimePickerPage()
{
}
