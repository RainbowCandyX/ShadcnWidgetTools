#include "RadioGroupPage.h"

#include <QString>
#include <QStringList>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadLabel.h"
#include "ShadRadioButton.h"
#include "ShadRadioGroup.h"
#include "ShadTheme.h"

RadioGroupPage::RadioGroupPage(QWidget* parent)
    : ExampleBasePage([] { return tr("单选组"); }, [] { return tr("ShadRadioGroup：单选组，支持纵向与横向排列，方向键可在选项间切换。"); }, parent)
{
    ShadCard* verticalCard = addSection([] { return tr("纵向排列"); }, [] { return tr("默认纵向排列"); });
    ShadRadioGroup* densityGroup = new ShadRadioGroup(this);
    densityGroup->addItems(QStringList{QString(), QString(), QString()});
    // 按索引改写选项文字，不重建选项，选中状态保留
    ExampleI18n::bind(densityGroup, [densityGroup]()
    {
        densityGroup->itemAt(0)->setText(tr("默认"));
        densityGroup->itemAt(1)->setText(tr("舒适"));
        densityGroup->itemAt(2)->setText(tr("紧凑"));
    });
    densityGroup->setCurrentIndex(1);
    ShadLabel* selectedLabel = new ShadLabel(this);
    selectedLabel->setColorRole(ShadThemeType::MutedForeground);
    // 按当前选中项生成文字；选项文字的绑定先连接，切换语言时先于这里刷新
    const auto updateSelectedText = [densityGroup, selectedLabel]()
    {
        const int index = densityGroup->getCurrentIndex();
        const QString itemText = index >= 0 ? densityGroup->itemAt(index)->text() : tr("无");
        selectedLabel->setText(tr("当前选中: %1").arg(itemText));
    };
    connect(densityGroup, &ShadRadioGroup::currentIndexChanged, selectedLabel, updateSelectedText);
    ExampleI18n::bind(selectedLabel, updateSelectedText);
    addItem(verticalCard, densityGroup);
    addItem(verticalCard, selectedLabel);

    ShadCard* horizontalCard = addSection([] { return tr("横向排列"); }, [] { return tr("横向排列，第三项禁用"); });
    ShadRadioGroup* planGroup = new ShadRadioGroup(Qt::Horizontal, this);
    planGroup->addItems(QStringList{QString(), QString(), QString()});
    ExampleI18n::bind(planGroup, [planGroup]()
    {
        planGroup->itemAt(0)->setText(tr("入门版"));
        planGroup->itemAt(1)->setText(tr("专业版"));
        planGroup->itemAt(2)->setText(tr("企业版"));
    });
    planGroup->setCurrentIndex(0);
    planGroup->itemAt(2)->setEnabled(false);
    addItem(horizontalCard, planGroup);
}

RadioGroupPage::~RadioGroupPage()
{
}
