#include "SwitchPage.h"

#include <QString>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"
#include "ShadTheme.h"

SwitchPage::SwitchPage(QWidget* parent)
    : ExampleBasePage([] { return tr("开关"); }, [] { return tr("ShadSwitch：在打开与关闭之间切换的开关，滑块带过渡动画。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("右侧文字可选，点击文字同样可以切换"); });
    ShadSwitch* airplaneSwitch = new ShadSwitch(this);
    ShadLabel* statusLabel = new ShadLabel(this);
    statusLabel->setColorRole(ShadThemeType::MutedForeground);
    // 状态文字按开关当前状态计算，切换开关与切换语言时都重新设置
    const auto updateStatus = [airplaneSwitch, statusLabel]() { statusLabel->setText(airplaneSwitch->isChecked() ? tr("当前状态: 打开") : tr("当前状态: 关闭")); };
    connect(airplaneSwitch, &ShadSwitch::toggled, statusLabel, updateStatus);
    ShadSwitch* checkedSwitch = new ShadSwitch(this);
    checkedSwitch->setChecked(true);
    addItem(basicCard, airplaneSwitch);
    addItem(basicCard, statusLabel);
    addItem(basicCard, checkedSwitch);

    ShadCard* disabledCard = addSection([] { return tr("禁用"); }, [] { return tr("禁用后整体半透明"); });
    ShadSwitch* disabledSwitch = new ShadSwitch(this);
    disabledSwitch->setEnabled(false);
    ShadSwitch* disabledCheckedSwitch = new ShadSwitch(this);
    disabledCheckedSwitch->setChecked(true);
    disabledCheckedSwitch->setEnabled(false);
    addItem(disabledCard, disabledSwitch);
    addItem(disabledCard, disabledCheckedSwitch);

    ExampleI18n::bind(this, [airplaneSwitch, checkedSwitch, disabledSwitch, disabledCheckedSwitch, updateStatus]()
    {
        airplaneSwitch->setText(tr("飞行模式"));
        checkedSwitch->setText(tr("无线网络"));
        disabledSwitch->setText(tr("禁用"));
        disabledCheckedSwitch->setText(tr("禁用且打开"));
        updateStatus();
    });
}

SwitchPage::~SwitchPage()
{
}
