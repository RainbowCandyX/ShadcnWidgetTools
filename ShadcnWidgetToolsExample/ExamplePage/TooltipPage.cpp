#include "TooltipPage.h"

#include <QKeySequence>
#include <QList>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadButtonGroup.h"
#include "ShadCard.h"
#include "ShadTooltip.h"

TooltipPage::TooltipPage(QWidget* parent)
    : ExampleBasePage([] { return tr("悬停提示"); }, [] { return tr("ShadTooltip：统一所有组件 toolTip 样式的悬停提示，悬停或聚焦时出现。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("直接使用 setToolTip，默认立即显示"); });
    ShadButton* hoverButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadButton* starButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    starButton->setButtonSize(ShadButtonType::Icon);
    starButton->setShadIcon(ShadIconType::Star);
    ShadButton* longButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(this, [hoverButton, starButton, longButton]()
    {
        hoverButton->setText(tr("悬停"));
        hoverButton->setToolTip(tr("添加到收藏"));
        starButton->setToolTip(tr("收藏"));
        longButton->setText(tr("长文字"));
        longButton->setToolTip(tr("提示文字超过 320px 时自动换行，适合放一两句简短的说明，不要放可以交互的内容。"));
    });
    addRow(basicCard, {hoverButton, starButton, longButton});

    ShadCard* sideCard = addSection([] { return tr("方向"); }, [] { return tr("setSide：指定提示方向，空间不足时翻到对侧"); });
    struct SideDemo
    {
        ShadPlacementType::Side side;
        ExampleI18n::TextFunction name;
        ExampleI18n::TextFunction toolTip;
    };
    const QList<SideDemo> sideDemos{
        {ShadPlacementType::Top, [] { return tr("上方"); }, [] { return tr("显示在上方"); }},
        {ShadPlacementType::Bottom, [] { return tr("下方"); }, [] { return tr("显示在下方"); }},
        {ShadPlacementType::Left, [] { return tr("左侧"); }, [] { return tr("显示在左侧"); }},
        {ShadPlacementType::Right, [] { return tr("右侧"); }, [] { return tr("显示在右侧"); }},
    };
    QList<QWidget*> sideButtons;
    for (const SideDemo& demo : sideDemos)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, this);
        ExampleI18n::bind(button, [button, demo]()
        {
            button->setText(demo.name());
            button->setToolTip(demo.toolTip());
        });
        ShadTooltip::setSide(button, demo.side);
        sideButtons.append(button);
    }
    addRow(sideCard, sideButtons);

    // kbd-tooltip：提示中带快捷键
    ShadCard* shortcutCard = addSection([] { return tr("快捷键"); }, [] { return tr("setShortcut：在提示文字后显示快捷键"); });
    ShadButtonGroup* fileGroup = new ShadButtonGroup(this);
    ShadButton* saveButton = new ShadButton(QString(), ShadButtonType::Outline, fileGroup);
    ShadTooltip::setShortcut(saveButton, QKeySequence(Qt::Key_S));
    ShadButton* printButton = new ShadButton(QString(), ShadButtonType::Outline, fileGroup);
    ShadTooltip::setShortcut(printButton, QKeySequence(Qt::CTRL | Qt::Key_P));
    ExampleI18n::bind(fileGroup, [saveButton, printButton]()
    {
        saveButton->setText(tr("保存"));
        saveButton->setToolTip(tr("保存更改"));
        printButton->setText(tr("打印"));
        printButton->setToolTip(tr("打印文档"));
    });
    fileGroup->addWidget(saveButton);
    fileGroup->addWidget(printButton);
    addItem(shortcutCard, fileGroup);

    ShadCard* keyboardCard = addSection([] { return tr("键盘聚焦"); }, [] { return tr("键盘聚焦到按钮时同样显示提示"); });
    QList<QWidget*> keyboardButtons;
    const QList<ExampleI18n::TextFunction> keyboardNames{[] { return tr("第一个"); }, [] { return tr("第二个"); }, [] { return tr("第三个"); }};
    for (const ExampleI18n::TextFunction& name : keyboardNames)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, this);
        ExampleI18n::bind(button, [button, name]()
        {
            const QString currentName = name();
            button->setText(currentName);
            button->setToolTip(tr("通过键盘聚焦到%1按钮").arg(currentName));
        });
        keyboardButtons.append(button);
    }
    addRow(keyboardCard, keyboardButtons);

    // tooltip-disabled：禁用组件不响应悬停，在外层包一个容器并把提示设在容器上
    ShadCard* disabledCard = addSection([] { return tr("禁用组件"); }, [] { return tr("禁用组件外包一层容器，把提示设在容器上"); });
    QWidget* wrapper = new QWidget(this);
    QVBoxLayout* wrapperLayout = new QVBoxLayout(wrapper);
    wrapperLayout->setContentsMargins(0, 0, 0, 0);
    ShadButton* disabledButton = new ShadButton(QString(), ShadButtonType::Outline, wrapper);
    disabledButton->setEnabled(false);
    wrapperLayout->addWidget(disabledButton);
    ExampleI18n::bind(wrapper, [wrapper, disabledButton]()
    {
        disabledButton->setText(tr("已禁用"));
        wrapper->setToolTip(tr("该操作当前不可用"));
    });
    addItem(disabledCard, wrapper);
}

TooltipPage::~TooltipPage()
{
}
