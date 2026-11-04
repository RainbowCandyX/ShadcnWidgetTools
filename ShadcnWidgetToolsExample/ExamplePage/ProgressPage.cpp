#include "ProgressPage.h"

#include <QString>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadLabel.h"
#include "ShadProgress.h"
#include "ShadSlider.h"
#include "ShadTheme.h"

ProgressPage::ProgressPage(QWidget* parent)
    : ExampleBasePage([] { return tr("进度条与滑块"); }, [] { return tr("ShadProgress / ShadSlider：进度条与滑块，进度变化和滑块外圈都带过渡动画。"); }, parent)
{
    ShadCard* progressCard = addSection([] { return tr("进度条"); }, [] { return tr("每次点击增加 10%，到 100% 后归零"); });
    ShadProgress* progress = new ShadProgress(this);
    progress->setValue(30);
    ShadButton* increaseButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(increaseButton, [increaseButton]() { increaseButton->setText(tr("增加 10%")); });
    increaseButton->setButtonSize(ShadButtonType::Small);
    connect(increaseButton, &ShadButton::clicked, progress, [progress]() { progress->setValue(progress->getValue() >= 100 ? 0 : progress->getValue() + 10); });
    addStretchItem(progressCard, progress);
    addItem(progressCard, increaseButton);

    ShadCard* sliderCard = addSection([] { return tr("滑块"); }, [] { return tr("拖动或点击轨道调整数值"); });
    ShadSlider* slider = new ShadSlider(Qt::Horizontal, this);
    slider->setRange(0, 100);
    slider->setValue(50);
    ShadLabel* valueLabel = new ShadLabel(this);
    valueLabel->setColorRole(ShadThemeType::MutedForeground);
    // 按滑块当前值生成文字，数值变化与切换语言时都调用
    const auto updateValueLabel = [slider, valueLabel]() { valueLabel->setText(tr("当前值: %1").arg(slider->value())); };
    ExampleI18n::bind(valueLabel, updateValueLabel);
    connect(slider, &ShadSlider::valueChanged, valueLabel, updateValueLabel);
    addStretchItem(sliderCard, slider);
    addItem(sliderCard, valueLabel);

    ShadCard* rangeCard = addSection([] { return tr("范围选择"); }, [] { return tr("setRangeMode(true)：用两个滑块选择范围"); });
    ShadSlider* rangeSlider = new ShadSlider(Qt::Horizontal, this);
    rangeSlider->setRange(0, 100);
    rangeSlider->setRangeMode(true);
    rangeSlider->setRangeValues(25, 75);
    ShadLabel* rangeLabel = new ShadLabel(this);
    rangeLabel->setColorRole(ShadThemeType::MutedForeground);
    // 切换语言时按当前范围重新生成：范围模式下 value() 为上限，getLowerValue() 为下限
    ExampleI18n::bind(rangeLabel, [rangeSlider, rangeLabel]() { rangeLabel->setText(tr("价格区间: %1 - %2").arg(rangeSlider->getLowerValue()).arg(rangeSlider->value())); });
    connect(rangeSlider, &ShadSlider::rangeValuesChanged, rangeLabel, [rangeLabel](int lower, int upper) { rangeLabel->setText(tr("价格区间: %1 - %2").arg(lower).arg(upper)); });
    addStretchItem(rangeCard, rangeSlider);
    addItem(rangeCard, rangeLabel);

    ShadCard* stepCard = addSection([] { return tr("步长 / 禁用 / 纵向"); }, [] { return tr("singleStep 为 10 时吸附到 10 的倍数"); });
    ShadSlider* stepSlider = new ShadSlider(Qt::Horizontal, this);
    stepSlider->setRange(0, 100);
    stepSlider->setSingleStep(10);
    stepSlider->setPageStep(10);
    stepSlider->setValue(30);
    stepSlider->setFixedWidth(ExampleLayout::kFieldWidth);
    ShadSlider* disabledSlider = new ShadSlider(Qt::Horizontal, this);
    disabledSlider->setRange(0, 100);
    disabledSlider->setValue(70);
    disabledSlider->setFixedWidth(ExampleLayout::kFieldWidth);
    disabledSlider->setEnabled(false);
    ShadSlider* verticalSlider = new ShadSlider(Qt::Vertical, this);
    verticalSlider->setRange(0, 100);
    verticalSlider->setValue(40);
    verticalSlider->setFixedHeight(160);
    addItem(stepCard, stepSlider);
    addItem(stepCard, disabledSlider);
    addItem(stepCard, verticalSlider);
}

ProgressPage::~ProgressPage()
{
}
