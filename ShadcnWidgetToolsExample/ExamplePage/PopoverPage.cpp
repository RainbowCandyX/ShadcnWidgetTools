#include "PopoverPage.h"

#include <QHBoxLayout>
#include <QList>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadPopover.h"

namespace
{
    // popover-demo 的 w-80
    constexpr int kFormPopoverWidth = 320;
    // 表单标签列宽，对应 grid-cols-3 的一列
    constexpr int kFormLabelWidth = 80;

    // 演示方向与对齐时浮层里的说明文字
    ShadPopover* createHintPopover(ExampleI18n::TextFunction title, ExampleI18n::TextFunction description, QWidget* parent)
    {
        ShadPopover* popover = new ShadPopover(parent);
        ExampleI18n::bind(popover, [popover, title, description]()
        {
            popover->setTitle(title());
            popover->setDescription(description());
        });
        popover->setContentWidth(240);
        return popover;
    }

    // 带文字的 outline 按钮，文字随语言刷新
    ShadButton* createOutlineButton(ExampleI18n::TextFunction text, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, parent);
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
        return button;
    }
} // namespace

PopoverPage::PopoverPage(QWidget* parent)
    : ExampleBasePage([] { return tr("浮层"); }, [] { return tr("ShadPopover：在锚点旁弹出的浮动卡片，可以放任意组件。"); }, parent)
{
    // popover-demo：尺寸表单
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("带标题与描述的表单浮层"); });
    ShadButton* openButton = createOutlineButton([] { return tr("打开浮层"); }, this);
    ShadPopover* formPopover = new ShadPopover(this);
    ExampleI18n::bind(formPopover, [formPopover]()
    {
        formPopover->setTitle(tr("尺寸"));
        formPopover->setDescription(tr("设置图层的尺寸。"));
    });
    formPopover->setContentWidth(kFormPopoverWidth);
    struct Field
    {
        ExampleI18n::TextFunction label;
        QString value;
    };
    const QList<Field> fields{
        {[] { return tr("宽度"); }, QStringLiteral("100%")},
        {[] { return tr("最大宽度"); }, QStringLiteral("300px")},
        {[] { return tr("高度"); }, QStringLiteral("25px")},
        {[] { return tr("最大高度"); }, QStringLiteral("none")},
    };
    // 字段之间 gap-2，扣除输入框上下的焦点环边距
    formPopover->contentLayout()->setSpacing(8 - 2 * ExampleLayout::kFocusRingWidth);
    for (const Field& field : fields)
    {
        QHBoxLayout* rowLayout = new QHBoxLayout();
        rowLayout->setSpacing(16 - ExampleLayout::kFocusRingWidth);
        ShadLabel* label = new ShadLabel(formPopover);
        const ExampleI18n::TextFunction labelText = field.label;
        ExampleI18n::bind(label, [label, labelText]() { label->setText(labelText()); });
        label->setFixedWidth(kFormLabelWidth);
        ShadInput* input = new ShadInput(formPopover);
        input->setText(field.value);
        label->setBuddy(input);
        rowLayout->addWidget(label);
        rowLayout->addWidget(input, 1);
        formPopover->contentLayout()->addLayout(rowLayout);
    }
    connect(openButton, &ShadButton::clicked, formPopover, [formPopover, openButton]() { formPopover->toggle(openButton); });
    addItem(basicCard, openButton);

    // popover-alignments：四个方向与三种对齐
    ShadCard* sideCard = addSection([] { return tr("方向"); }, [] { return tr("从四个方向弹出，空间不足时翻到对侧"); });
    struct SideDemo
    {
        ShadPlacementType::Side side;
        ExampleI18n::TextFunction name;
    };
    const QList<SideDemo> sideDemos{
        {ShadPlacementType::Top, [] { return tr("上方"); }},
        {ShadPlacementType::Bottom, [] { return tr("下方"); }},
        {ShadPlacementType::Left, [] { return tr("左侧"); }},
        {ShadPlacementType::Right, [] { return tr("右侧"); }},
    };
    QList<QWidget*> sideButtons;
    for (const SideDemo& demo : sideDemos)
    {
        const ExampleI18n::TextFunction name = demo.name;
        ShadButton* button = createOutlineButton(name, this);
        ShadPopover* popover = createHintPopover([name] { return tr("显示在%1").arg(name()); }, [] { return tr("与按钮间距 4px，居中对齐。"); }, this);
        const ShadPlacementType::Side side = demo.side;
        connect(button, &ShadButton::clicked, popover, [popover, button, side]() { popover->toggle(button, side, ShadPlacementType::Center); });
        sideButtons.append(button);
    }
    addRow(sideCard, sideButtons);

    ShadCard* alignCard = addSection([] { return tr("对齐"); }, [] { return tr("起始、居中、结束三种对齐方式"); });
    struct AlignDemo
    {
        ShadPlacementType::Align align;
        ExampleI18n::TextFunction name;
    };
    const QList<AlignDemo> alignDemos{
        {ShadPlacementType::Start, [] { return tr("起始对齐"); }},
        {ShadPlacementType::Center, [] { return tr("居中对齐"); }},
        {ShadPlacementType::End, [] { return tr("结束对齐"); }},
    };
    QList<QWidget*> alignButtons;
    for (const AlignDemo& demo : alignDemos)
    {
        ShadButton* button = createOutlineButton(demo.name, this);
        ShadPopover* popover = createHintPopover(demo.name, [] { return tr("浮层宽 240，按钮较窄时可以看出对齐方式。"); }, this);
        const ShadPlacementType::Align align = demo.align;
        connect(button, &ShadButton::clicked, popover, [popover, button, align]() { popover->toggle(button, ShadPlacementType::Bottom, align); });
        alignButtons.append(button);
    }
    addRow(alignCard, alignButtons);
}

PopoverPage::~PopoverPage()
{
}
