#include "SegmentedPage.h"

#include <QList>
#include <QString>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadSegmented.h"
#include "ShadTheme.h"

SegmentedPage::SegmentedPage(QWidget* parent)
    : ExampleBasePage([] { return tr("分段控制器"); }, [] { return tr("ShadSegmented：在多个互斥选项之间切换的分段控制器，选中块带滑动动画。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("点击或使用左右方向键切换选项"); });
    ShadSegmented* periodSegmented = new ShadSegmented(this);
    periodSegmented->addItem(QString());
    periodSegmented->addItem(QString());
    periodSegmented->addItem(QString());
    periodSegmented->addItem(QString());
    ExampleI18n::bind(periodSegmented, [periodSegmented]()
    {
        periodSegmented->setItemText(0, tr("日"));
        periodSegmented->setItemText(1, tr("周"));
        periodSegmented->setItemText(2, tr("月"));
        periodSegmented->setItemText(3, tr("年"));
    });
    ShadLabel* periodLabel = new ShadLabel(this);
    periodLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updatePeriodLabel = [periodSegmented, periodLabel]() { periodLabel->setText(tr("当前选择: %1").arg(periodSegmented->getItemText(periodSegmented->getCurrentIndex()))); };
    ExampleI18n::bind(periodLabel, updatePeriodLabel);
    connect(periodSegmented, &ShadSegmented::currentIndexChanged, periodLabel, updatePeriodLabel);
    addItem(basicCard, periodSegmented);
    addItem(basicCard, periodLabel);

    ShadCard* iconCard = addSection([] { return tr("图标"); }, [] { return tr("图标加文字，或仅图标并设置提示"); });
    ShadSegmented* viewSegmented = new ShadSegmented(this);
    viewSegmented->addItem(ShadIconType::List);
    viewSegmented->addItem(ShadIconType::LayoutGrid);
    viewSegmented->addItem(ShadIconType::Kanban);
    ExampleI18n::bind(viewSegmented, [viewSegmented]()
    {
        viewSegmented->setItemText(0, tr("列表"));
        viewSegmented->setItemText(1, tr("网格"));
        viewSegmented->setItemText(2, tr("看板"));
    });
    ShadSegmented* iconOnlySegmented = new ShadSegmented(this);
    iconOnlySegmented->addItem(ShadIconType::List);
    iconOnlySegmented->addItem(ShadIconType::LayoutGrid);
    iconOnlySegmented->addItem(ShadIconType::Kanban);
    ExampleI18n::bind(iconOnlySegmented, [iconOnlySegmented]()
    {
        iconOnlySegmented->setItemToolTip(0, tr("列表"));
        iconOnlySegmented->setItemToolTip(1, tr("网格"));
        iconOnlySegmented->setItemToolTip(2, tr("看板"));
    });
    addRow(iconCard, {viewSegmented, iconOnlySegmented});

    ShadCard* sizeCard = addSection([] { return tr("尺寸"); }, [] { return tr("Small、Default、Large 三种尺寸"); });
    const QList<ShadSegmentedType::Size> sizes{ShadSegmentedType::Small, ShadSegmentedType::Default, ShadSegmentedType::Large};
    QList<QWidget*> sizeWidgets;
    for (ShadSegmentedType::Size size : sizes)
    {
        ShadSegmented* segmented = new ShadSegmented(this);
        segmented->setSegmentedSize(size);
        segmented->addItem(QString());
        segmented->addItem(QString());
        segmented->addItem(QString());
        ExampleI18n::bind(segmented, [segmented]()
        {
            segmented->setItemText(0, tr("日"));
            segmented->setItemText(1, tr("周"));
            segmented->setItemText(2, tr("月"));
        });
        sizeWidgets.append(segmented);
    }
    addRow(sizeCard, sizeWidgets);

    ShadCard* blockCard = addSection([] { return tr("撑满宽度与禁用项"); }, [] { return tr("setBlock(true) 各项等宽撑满；可禁用单项或整体"); });
    ShadSegmented* blockSegmented = new ShadSegmented(this);
    blockSegmented->setBlock(true);
    blockSegmented->addItem(QString());
    blockSegmented->addItem(QString());
    blockSegmented->addItem(QString());
    ExampleI18n::bind(blockSegmented, [blockSegmented]()
    {
        blockSegmented->setItemText(0, tr("概览"));
        blockSegmented->setItemText(1, tr("分析"));
        blockSegmented->setItemText(2, tr("报告"));
    });
    ShadSegmented* partDisabledSegmented = new ShadSegmented(this);
    partDisabledSegmented->addItem(QString());
    partDisabledSegmented->addItem(QString());
    partDisabledSegmented->addItem(QString());
    partDisabledSegmented->setItemEnabled(1, false);
    ExampleI18n::bind(partDisabledSegmented, [partDisabledSegmented]()
    {
        partDisabledSegmented->setItemText(0, tr("日"));
        partDisabledSegmented->setItemText(1, tr("周"));
        partDisabledSegmented->setItemText(2, tr("月"));
    });
    ShadSegmented* disabledSegmented = new ShadSegmented(this);
    disabledSegmented->addItem(QString());
    disabledSegmented->addItem(QString());
    disabledSegmented->addItem(QString());
    disabledSegmented->setEnabled(false);
    ExampleI18n::bind(disabledSegmented, [disabledSegmented]()
    {
        disabledSegmented->setItemText(0, tr("日"));
        disabledSegmented->setItemText(1, tr("周"));
        disabledSegmented->setItemText(2, tr("月"));
    });
    addStretchItem(blockCard, blockSegmented);
    addRow(blockCard, {partDisabledSegmented, disabledSegmented});
}

SegmentedPage::~SegmentedPage()
{
}
