#include "RadialProgressPage.h"

#include <QList>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadRadialProgress.h"
#include "ShadSlider.h"

RadialProgressPage::RadialProgressPage(QWidget* parent)
    : ExampleBasePage([] { return tr("环形进度"); }, [] { return tr("ShadRadialProgress：环形进度，支持整圆与仪表盘两种形态，数值变化带过渡动画。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("中心文字显示当前百分比，数字随动画变化"); });
    ShadRadialProgress* basicProgress = new ShadRadialProgress(this);
    basicProgress->setValue(30);
    ShadButton* increaseButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(increaseButton, [increaseButton]() { increaseButton->setText(tr("增加 10%")); });
    increaseButton->setButtonSize(ShadButtonType::Small);
    connect(increaseButton, &ShadButton::clicked, basicProgress, [basicProgress]() { basicProgress->setValue(basicProgress->getValue() + 10); });
    ShadButton* decreaseButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(decreaseButton, [decreaseButton]() { decreaseButton->setText(tr("减少 10%")); });
    decreaseButton->setButtonSize(ShadButtonType::Small);
    connect(decreaseButton, &ShadButton::clicked, basicProgress, [basicProgress]() { basicProgress->setValue(basicProgress->getValue() - 10); });
    addItem(basicCard, basicProgress);
    addRow(basicCard, QList<QWidget*>{increaseButton, decreaseButton});

    ShadCard* gaugeCard = addSection([] { return tr("仪表盘"); }, [] { return tr("Gauge 变体为 270° 圆弧，缺口朝下"); });
    ShadRadialProgress* gaugeProgress = new ShadRadialProgress(this);
    gaugeProgress->setVariant(ShadRadialProgressType::Gauge);
    gaugeProgress->setValue(60);
    ShadSlider* gaugeSlider = new ShadSlider(Qt::Horizontal, this);
    gaugeSlider->setRange(0, 100);
    gaugeSlider->setValue(60);
    gaugeSlider->setFixedWidth(ExampleLayout::kFieldWidth);
    connect(gaugeSlider, &ShadSlider::valueChanged, gaugeProgress, &ShadRadialProgress::setValue);
    addItem(gaugeCard, gaugeProgress);
    addItem(gaugeCard, gaugeSlider);

    ShadCard* statusCard = addSection([] { return tr("状态"); }, [] { return tr("状态决定指示色，轨道为指示色的 20% 透明度"); });
    const QList<ShadRadialProgressType::Status> statuses{ShadRadialProgressType::Default, ShadRadialProgressType::Success, ShadRadialProgressType::Warning, ShadRadialProgressType::Destructive};
    const QList<int> statusValues{25, 100, 60, 15};
    QList<QWidget*> statusWidgets;
    for (int index = 0; index < statuses.size(); ++index)
    {
        ShadRadialProgress* progress = new ShadRadialProgress(this);
        progress->setStatus(statuses.at(index));
        progress->setValue(statusValues.at(index));
        statusWidgets.append(progress);
    }
    addRow(statusCard, statusWidgets);

    ShadCard* sizeCard = addSection([] { return tr("尺寸与文字"); }, [] { return tr("可设置直径与线宽，支持自定义文字或隐藏文字"); });
    const QList<int> diameters{64, 120, 160};
    QList<QWidget*> sizeWidgets;
    for (const int diameter : diameters)
    {
        ShadRadialProgress* progress = new ShadRadialProgress(this);
        progress->setDiameter(diameter);
        progress->setThickness(diameter <= 64 ? 6 : 10);
        progress->setValue(75);
        sizeWidgets.append(progress);
    }
    ShadRadialProgress* customTextProgress = new ShadRadialProgress(this);
    customTextProgress->setValue(100);
    customTextProgress->setStatus(ShadRadialProgressType::Success);
    ExampleI18n::bind(customTextProgress, [customTextProgress]() { customTextProgress->setText(tr("已完成")); });
    sizeWidgets.append(customTextProgress);
    ShadRadialProgress* hiddenTextProgress = new ShadRadialProgress(this);
    hiddenTextProgress->setValue(45);
    hiddenTextProgress->setTextVisible(false);
    sizeWidgets.append(hiddenTextProgress);
    addRow(sizeCard, sizeWidgets);
}

RadialProgressPage::~RadialProgressPage()
{
}
