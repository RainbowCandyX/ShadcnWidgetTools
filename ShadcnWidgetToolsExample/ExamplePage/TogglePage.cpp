#include "TogglePage.h"

#include <QList>
#include <QString>
#include <QWidget>

#include <memory>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadTheme.h"
#include "ShadToggle.h"
#include "ShadToggleGroup.h"

TogglePage::TogglePage(QWidget* parent)
    : ExampleBasePage([] { return tr("切换按钮"); }, [] { return tr("ShadToggle / ShadToggleGroup：可选中的双态按钮，以及支持单选与多选的按钮组。"); }, parent)
{
    ShadCard* variantCard = addSection([] { return tr("变体"); }, [] { return tr("Default 与 Outline 两种外观"); });
    ShadToggle* boldToggle = new ShadToggle(ShadIconType::Bold, this);
    ShadToggle* italicToggle = new ShadToggle(ShadIconType::Italic, this);
    italicToggle->setVariant(ShadToggleType::Outline);
    ShadToggle* underlineToggle = new ShadToggle(this);
    underlineToggle->setShadIcon(ShadIconType::Underline);
    underlineToggle->setChecked(true);
    ExampleI18n::bind(this, [boldToggle, italicToggle, underlineToggle]()
    {
        boldToggle->setToolTip(tr("加粗"));
        italicToggle->setToolTip(tr("斜体"));
        underlineToggle->setText(tr("下划线"));
    });
    addRow(variantCard, {boldToggle, italicToggle, underlineToggle});

    ShadCard* sizeCard = addSection([] { return tr("尺寸"); }, [] { return tr("Small、Medium、Large 三种尺寸"); });
    const QList<ShadToggleType::Size> sizes{ShadToggleType::Small, ShadToggleType::Medium, ShadToggleType::Large};
    QList<QWidget*> sizeWidgets;
    for (ShadToggleType::Size size : sizes)
    {
        ShadToggle* toggle = new ShadToggle(ShadIconType::Italic, this);
        toggle->setVariant(ShadToggleType::Outline);
        toggle->setToggleSize(size);
        ExampleI18n::bind(toggle, [toggle]() { toggle->setToolTip(tr("斜体")); });
        sizeWidgets.append(toggle);
    }
    addRow(sizeCard, sizeWidgets);

    ShadCard* disabledCard = addSection([] { return tr("禁用"); }, [] { return tr("禁用后整体半透明"); });
    ShadToggle* disabledToggle = new ShadToggle(ShadIconType::Underline, this);
    disabledToggle->setEnabled(false);
    ShadToggle* disabledCheckedToggle = new ShadToggle(ShadIconType::Bold, this);
    disabledCheckedToggle->setChecked(true);
    disabledCheckedToggle->setEnabled(false);
    addRow(disabledCard, {disabledToggle, disabledCheckedToggle});

    ShadCard* singleGroupCard = addSection([] { return tr("按钮组：单选"); }, [] { return tr("最多选中一项，再次点击可取消选中"); });
    ShadToggleGroup* alignGroup = new ShadToggleGroup(ShadToggleGroupType::Single, this);
    alignGroup->setVariant(ShadToggleType::Outline);
    alignGroup->addItem(ShadIconType::TextAlignStart);
    alignGroup->addItem(ShadIconType::TextAlignCenter);
    alignGroup->addItem(ShadIconType::TextAlignEnd);
    alignGroup->setCurrentIndex(0);
    ExampleI18n::bind(alignGroup, [alignGroup]()
    {
        alignGroup->itemAt(0)->setToolTip(tr("左对齐"));
        alignGroup->itemAt(1)->setToolTip(tr("居中"));
        alignGroup->itemAt(2)->setToolTip(tr("右对齐"));
    });
    ShadToggleGroup* periodGroup = new ShadToggleGroup(ShadToggleGroupType::Single, this);
    periodGroup->addItem(QString());
    periodGroup->addItem(QString());
    periodGroup->addItem(QString());
    periodGroup->setCurrentIndex(1);
    ShadLabel* periodLabel = new ShadLabel(this);
    periodLabel->setColorRole(ShadThemeType::MutedForeground);
    // 用户还没有改变选择时显示初始文字，之后按当前选中项生成；选项文字先于此处刷新（连接顺序在前）
    const std::shared_ptr<bool> isPeriodChanged = std::make_shared<bool>(false);
    const auto updatePeriodLabel = [periodGroup, periodLabel, isPeriodChanged]()
    {
        if (!*isPeriodChanged)
        {
            periodLabel->setText(tr("当前选中: 周"));
            return;
        }
        const int index = periodGroup->getCurrentIndex();
        periodLabel->setText(index >= 0 ? tr("当前选中: %1").arg(periodGroup->itemAt(index)->text()) : tr("当前选中: 无"));
    };
    ExampleI18n::bind(periodGroup, [periodGroup]()
    {
        periodGroup->itemAt(0)->setText(tr("日"));
        periodGroup->itemAt(1)->setText(tr("周"));
        periodGroup->itemAt(2)->setText(tr("月"));
    });
    ExampleI18n::bind(periodLabel, updatePeriodLabel);
    connect(periodGroup, &ShadToggleGroup::currentIndexChanged, periodLabel, [isPeriodChanged, updatePeriodLabel]()
    {
        *isPeriodChanged = true;
        updatePeriodLabel();
    });
    addRow(singleGroupCard, {alignGroup, periodGroup, periodLabel});

    ShadCard* multipleGroupCard = addSection([] { return tr("按钮组：多选"); }, [] { return tr("可以同时选中多项"); });
    ShadToggleGroup* formatGroup = new ShadToggleGroup(ShadToggleGroupType::Multiple, this);
    formatGroup->setVariant(ShadToggleType::Outline);
    formatGroup->addItem(ShadIconType::Bold);
    formatGroup->addItem(ShadIconType::Italic);
    formatGroup->addItem(ShadIconType::Underline);
    formatGroup->setItemChecked(0, true);
    formatGroup->setItemChecked(2, true);
    ExampleI18n::bind(formatGroup, [formatGroup]()
    {
        formatGroup->itemAt(0)->setText(tr("加粗"));
        formatGroup->itemAt(1)->setText(tr("斜体"));
        formatGroup->itemAt(2)->setText(tr("下划线"));
    });
    ShadToggleGroup* smallGroup = new ShadToggleGroup(ShadToggleGroupType::Multiple, this);
    smallGroup->setToggleSize(ShadToggleType::Small);
    smallGroup->addItem(ShadIconType::Bold);
    smallGroup->addItem(ShadIconType::Italic);
    smallGroup->addItem(ShadIconType::Underline);
    ExampleI18n::bind(smallGroup, [smallGroup]()
    {
        smallGroup->itemAt(0)->setToolTip(tr("加粗"));
        smallGroup->itemAt(1)->setToolTip(tr("斜体"));
        smallGroup->itemAt(2)->setToolTip(tr("下划线"));
    });
    addRow(multipleGroupCard, {formatGroup, smallGroup});
}

TogglePage::~TogglePage()
{
}
