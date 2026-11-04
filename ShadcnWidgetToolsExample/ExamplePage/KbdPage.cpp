#include "KbdPage.h"

#include <QAction>
#include <QKeyCombination>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadButtonGroup.h"
#include "ShadCard.h"
#include "ShadInput.h"
#include "ShadInputGroup.h"
#include "ShadKbd.h"
#include "ShadKeySequenceEdit.h"
#include "ShadLabel.h"
#include "ShadToaster.h"
#include "ShadTooltip.h"

namespace
{
    // 句子中的普通文字：text-sm text-muted-foreground；文字由调用方绑定
    ShadLabel* createMutedText(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    // 文字随语言刷新的普通文字
    ShadLabel* createMutedText(ExampleI18n::TextFunction text, QWidget* parent)
    {
        ShadLabel* label = createMutedText(parent);
        ExampleI18n::bind(label, [label, text]() { label->setText(text()); });
        return label;
    }

    // 文字随语言刷新的按钮
    ShadButton* createButton(ExampleI18n::TextFunction text, ShadButtonType::Variant variant, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), variant, parent);
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
        return button;
    }

    // 按 QKeySequence 生成的按键组合
    ShadKbdGroup* createSequenceGroup(const QKeySequence& sequence, QWidget* parent)
    {
        ShadKbdGroup* group = new ShadKbdGroup(parent);
        group->setKeySequence(sequence);
        return group;
    }
} // namespace

KbdPage::KbdPage(ShadToaster* toaster, QWidget* parent)
    : ExampleBasePage([] { return tr("按键"); }, [] { return tr("ShadKbd / ShadKbdGroup：显示键盘按键或快捷键，并可绑定为真正的快捷键。"); }, parent)
{
    const auto notify = [toaster](const QString& title, const QString& description)
    {
        if (toaster != nullptr)
        {
            toaster->add(title, description);
        }
    };

    // kbd-demo
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("按键名称按平台显示，按键之间可插入普通文字"); });
    // 四个修饰键按平台显示：macOS 为 ⌃ ⌥ ⇧ ⌘，其他平台为 Ctrl Alt Shift Win / Meta
    ShadKbdGroup* modifierGroup = new ShadKbdGroup(ShadKbdGroup::keyTexts(QKeyCombination(Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier, Qt::Key(0))), this);
    // 按键之间插入 "+"：macOS 的习惯写法不带加号，按键直接相连
    ShadKbdGroup* comboGroup = new ShadKbdGroup(this);
    const QStringList comboKeys = ShadKbdGroup::keyTexts(QKeySequence(Qt::CTRL | Qt::Key_B));
    for (int index = 0; index < comboKeys.size(); ++index)
    {
#ifndef Q_OS_MACOS
        if (index > 0)
        {
            comboGroup->addText(QStringLiteral("+"));
        }
#endif
        comboGroup->addKey(comboKeys.at(index));
    }
    addRow(basicCard, {modifierGroup, comboGroup});

    ShadCard* sequenceCard = addSection([] { return tr("按键序列"); }, [] { return tr("setKeySequence：按平台显示组合键"); });
    addRow(sequenceCard, {createSequenceGroup(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), this), createSequenceGroup(QKeySequence(Qt::ALT | Qt::Key_Return), this), createSequenceGroup(QKeySequence(QKeySequence::Find), this), createSequenceGroup(QKeySequence(Qt::CTRL | Qt::Key_K, Qt::CTRL | Qt::Key_S), this)});

    // kbd-group：句子中的快捷键
    ShadCard* sentenceCard = addSection([] { return tr("句子中"); }, [] { return tr("与文字一起排列"); });
    addRow(sentenceCard, {createMutedText([] { return tr("使用"); }, this), createSequenceGroup(QKeySequence(Qt::CTRL | Qt::Key_B), this), createMutedText([] { return tr("或"); }, this), createSequenceGroup(QKeySequence(Qt::CTRL | Qt::Key_J), this), createMutedText([] { return tr("打开命令面板"); }, this)});

    // kbd-tooltip：快捷键由 QAction 绑定到按钮，提示中以反色显示
    ShadCard* tooltipCard = addSection([] { return tr("提示中"); }, [] { return tr("按钮绑定快捷键，提示中显示同一组合键"); });
    ShadButtonGroup* fileGroup = new ShadButtonGroup(this);
    struct ShortcutDemo
    {
        ExampleI18n::TextFunction text;
        ExampleI18n::TextFunction toolTip;
        QKeySequence sequence;
    };
    const QList<ShortcutDemo> shortcutDemos{
        {[] { return tr("保存"); }, [] { return tr("保存更改"); }, QKeySequence(Qt::Key_S)},
        {[] { return tr("打印"); }, [] { return tr("打印文档"); }, QKeySequence(Qt::CTRL | Qt::Key_P)},
    };
    for (const ShortcutDemo& demo : shortcutDemos)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, fileGroup);
        // 快捷键挂在按钮上：按钮所在页面显示时生效，输入框编辑文字时不会抢占普通按键
        QAction* action = new QAction(button);
        const ExampleI18n::TextFunction text = demo.text;
        const ExampleI18n::TextFunction toolTip = demo.toolTip;
        ExampleI18n::bind(button, [button, action, text, toolTip]()
        {
            button->setText(text());
            button->setToolTip(toolTip());
            action->setText(toolTip());
        });
        action->setShortcut(demo.sequence);
        action->setShortcutContext(Qt::WindowShortcut);
        button->addAction(action);
        connect(action, &QAction::triggered, button, &ShadButton::click);
        ShadTooltip::setShortcut(button, demo.sequence);
        connect(button, &ShadButton::clicked, this, [notify, toolTip]() { notify(toolTip(), tr("已执行")); });
        fileGroup->addWidget(button);
    }
    addItem(tooltipCard, fileGroup);

    // 跟随 QAction：快捷键变化时按键组自动更新，触发由 QAction 负责
    ShadCard* actionCard = addSection([] { return tr("跟随 QAction"); }, [] { return tr("bindAction：显示 QAction 的快捷键并随之更新"); });
    ShadButton* exportButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    QAction* exportAction = new QAction(exportButton);
    ExampleI18n::bind(exportButton, [exportButton, exportAction]()
    {
        exportButton->setText(tr("导出"));
        exportAction->setText(tr("导出"));
    });
    exportAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    exportAction->setShortcutContext(Qt::WindowShortcut);
    exportButton->addAction(exportAction);
    connect(exportAction, &QAction::triggered, exportButton, &ShadButton::click);
    connect(exportButton, &ShadButton::clicked, this, [notify, exportAction]() { notify(tr("导出"), exportAction->shortcut().toString(QKeySequence::NativeText)); });
    ShadKbdGroup* exportKeys = new ShadKbdGroup(this);
    exportKeys->bindAction(exportAction);
    ShadButton* switchButton = createButton([] { return tr("切换快捷键"); }, ShadButtonType::Ghost, this);
    connect(switchButton, &ShadButton::clicked, exportAction, [exportAction]()
    {
        const QKeySequence plain(Qt::CTRL | Qt::Key_E);
        exportAction->setShortcut(exportAction->shortcut() == plain ? QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E) : plain);
    });
    addRow(actionCard, {exportButton, exportKeys, switchButton});

    // 录制快捷键：只记录与显示，不注册快捷键
    ShadCard* recordCard = addSection([] { return tr("录制快捷键"); }, [] { return tr("ShadKeySequenceEdit：点击后按下组合键录制"); });
    ShadKeySequenceEdit* recorder = new ShadKeySequenceEdit(this);
    recorder->setFixedWidth(ExampleLayout::kFieldWidth / 4 * 3 + 2 * ExampleLayout::kFocusRingWidth);
    ShadLabel* recordedLabel = createMutedText(this);
    const auto updateRecordedLabel = [recorder, recordedLabel]()
    {
        // 显示用 NativeText（macOS 为 ⌘Y，其他平台为 Ctrl+Y）；保存到配置时应使用 PortableText，跨平台读回后含义不变
        const QKeySequence sequence = recorder->getKeySequence();
        recordedLabel->setText(sequence.isEmpty() ? tr("当前值：未设置") : tr("当前值：%1").arg(sequence.toString(QKeySequence::NativeText)));
    };
    connect(recorder, &ShadKeySequenceEdit::keySequenceChanged, recordedLabel, updateRecordedLabel);
    ExampleI18n::bind(recordedLabel, updateRecordedLabel);
    addRow(recordCard, {recorder, recordedLabel});

    // kbd-input-group：Ctrl+K（macOS 为 ⌘K）聚焦搜索框
    ShadCard* inputCard = addSection([] { return tr("输入组中"); }, [] { return tr("绑定快捷键，按下后聚焦搜索框"); });
    ShadInputGroup* searchGroup = new ShadInputGroup(this);
    searchGroup->setFixedWidth(ExampleLayout::kFieldWidth + 2 * ExampleLayout::kFocusRingWidth);
    ExampleI18n::bind(searchGroup, [searchGroup]() { searchGroup->getInput()->setPlaceholderText(tr("搜索...")); });
    searchGroup->addIcon(ShadInputGroupType::InlineStart, ShadIconType::Search);
    ShadKbdGroup* searchKeys = new ShadKbdGroup(searchGroup);
    searchKeys->setKeySequence(QKeySequence(Qt::CTRL | Qt::Key_K));
    searchKeys->setShortcutEnabled(true);
    connect(searchKeys, &ShadKbdGroup::activated, searchGroup, [searchGroup]()
    {
        searchGroup->getInput()->setFocus(Qt::ShortcutFocusReason);
        searchGroup->getInput()->selectAll();
    });
    searchGroup->addWidget(ShadInputGroupType::InlineEnd, searchKeys);
    addItem(inputCard, searchGroup);
}

KbdPage::~KbdPage()
{
}
