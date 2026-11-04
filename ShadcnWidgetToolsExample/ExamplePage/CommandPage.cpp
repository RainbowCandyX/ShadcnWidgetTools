#include "CommandPage.h"

#include <QAction>
#include <QCoreApplication>
#include <QKeySequence>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <memory>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadCommand.h"
#include "ShadCommandDialog.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadKbd.h"
#include "ShadKeySequenceEdit.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"

namespace
{
    // md:min-w-[450px]
    constexpr int kCommandWidth = 450;

    QString shortcut(const QKeySequence& sequence)
    {
        return ShadKbdGroup::shortcutText(sequence);
    }

    // 分组标题通过 setGroupHeading、命令项文字通过 QAction::setText 随语言刷新
    void fillCommand(ShadCommand* command, ShadLabel* status)
    {
        const int suggestionGroup = command->addGroup(QString());
        QAction* calendarAction = command->addItem(QString(), ShadIconType::CalendarDays);
        command->setItemKeywords(calendarAction, {QStringLiteral("calendar"), QStringLiteral("rili")});
        QAction* emojiAction = command->addItem(QString(), ShadIconType::Smile);
        command->setItemKeywords(emojiAction, {QStringLiteral("emoji")});
        QAction* calculatorAction = command->addItem(QString(), ShadIconType::Calculator);
        calculatorAction->setEnabled(false);
        command->addSeparator();
        const int settingsGroup = command->addGroup(QString());
        QAction* profileAction = command->addItem(QString(), ShadIconType::User, shortcut(QKeySequence(Qt::CTRL | Qt::Key_P)));
        QAction* billingAction = command->addItem(QString(), ShadIconType::CreditCard, shortcut(QKeySequence(Qt::CTRL | Qt::Key_B)));
        QAction* settingsAction = command->addItem(QString(), ShadIconType::Settings, shortcut(QKeySequence(Qt::CTRL | Qt::Key_S)));
        ExampleI18n::bind(command, [command, suggestionGroup, settingsGroup, calendarAction, emojiAction, calculatorAction, profileAction, billingAction, settingsAction]()
        {
            command->setGroupHeading(suggestionGroup, QCoreApplication::translate("CommandPage", "建议"));
            command->setGroupHeading(settingsGroup, QCoreApplication::translate("CommandPage", "设置"));
            calendarAction->setText(QCoreApplication::translate("CommandPage", "日历"));
            emojiAction->setText(QCoreApplication::translate("CommandPage", "搜索表情"));
            calculatorAction->setText(QCoreApplication::translate("CommandPage", "计算器"));
            profileAction->setText(QCoreApplication::translate("CommandPage", "个人资料"));
            billingAction->setText(QCoreApplication::translate("CommandPage", "账单"));
            settingsAction->setText(QCoreApplication::translate("CommandPage", "设置"));
        });
        // 记录最近点击的命令项，切换语言时按该项的当前文字重新生成；在命令项之后绑定，保证读到的是新语言的文字
        std::shared_ptr<QPointer<QAction>> lastAction = std::make_shared<QPointer<QAction>>();
        const auto updateStatus = [status, lastAction]()
        {
            const QAction* action = lastAction->data();
            status->setText(action != nullptr ? QCoreApplication::translate("CommandPage", "最近点击: %1").arg(action->text()) : QCoreApplication::translate("CommandPage", "最近点击: 无"));
        };
        ExampleI18n::bind(status, updateStatus);
        QObject::connect(command, &ShadCommand::itemTriggered, status, [lastAction, updateStatus](QAction* action)
        {
            *lastAction = action;
            updateStatus();
        });
    }

    ShadLabel* createStatusLabel(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }
} // namespace

CommandPage::CommandPage(QWidget* parent)
    : ExampleBasePage([] { return tr("命令面板"); }, [] { return tr("ShadCommand：搜索并执行命令。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("分组、图标与快捷键"); });
    ShadLabel* basicStatus = createStatusLabel(this);
    ShadCommand* command = new ShadCommand(this);
    command->setMinimumWidth(kCommandWidth);
    fillCommand(command, basicStatus);
    addItem(basicCard, command);
    addItem(basicCard, basicStatus);

    ShadCard* dialogCard = addSection([] { return tr("对话框"); }, [] { return tr("全局快捷键打开，点击外部关闭"); });
    ShadLabel* dialogStatus = createStatusLabel(this);
    // 创建前设置默认快捷键
    ShadCommandDialog::setDefaultShortcut(QKeySequence(Qt::CTRL | Qt::Key_K));
    ShadCommandDialog* dialog = new ShadCommandDialog(this);
    fillCommand(dialog->command(), dialogStatus);
    ShadButton* openButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    connect(openButton, &ShadButton::clicked, dialog, &ShadCommandDialog::open);
    ShadSwitch* resizableSwitch = new ShadSwitch(this);
    connect(resizableSwitch, &ShadSwitch::toggled, dialog, &ShadCommandDialog::setResizable);
    addRow(dialogCard, {openButton, resizableSwitch, dialogStatus});
    // 创建后修改快捷键
    ShadKeySequenceEdit* shortcutEdit = new ShadKeySequenceEdit(dialog->getShortcut(), this);
    connect(shortcutEdit, &ShadKeySequenceEdit::keySequenceChanged, dialog, &ShadCommandDialog::setShortcut);
    ShadLabel* shortcutLabel = new ShadLabel(this);
    addRow(dialogCard, {shortcutLabel, shortcutEdit});
    ExampleI18n::bind(this, [openButton, resizableSwitch, shortcutLabel]()
    {
        openButton->setText(tr("打开命令面板"));
        resizableSwitch->setText(tr("可调整尺寸"));
        shortcutLabel->setText(tr("快捷键"));
    });
}

CommandPage::~CommandPage()
{
}
