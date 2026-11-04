#include "DropdownMenuPage.h"

#include <QAction>
#include <QKeySequence>
#include <QPointer>
#include <QString>

#include <memory>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadKbd.h"
#include "ShadLabel.h"

DropdownMenuPage::DropdownMenuPage(QWidget* parent)
    : ExampleBasePage([] { return tr("下拉菜单"); }, [] { return tr("ShadDropdownMenu：菜单项用 QAction 表示的下拉菜单，支持复选项、分组与分隔线。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("点击按钮打开菜单，支持键盘操作"); });
    ShadButton* accountButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadDropdownMenu* accountMenu = new ShadDropdownMenu(this);
    const int accountMenuLabel = accountMenu->addLabel(QString());
    ExampleI18n::bind(accountMenu, [accountMenu, accountMenuLabel]() { accountMenu->setLabelText(accountMenuLabel, tr("我的账户")); });
    accountMenu->addSeparator();
    // 快捷键提示按平台生成：macOS 为 ⇧⌘P，其他平台为 Ctrl+Shift+P
    QAction* profileAction = accountMenu->addItem(QString(), ShadIconType::User, ShadKbdGroup::shortcutText(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P)));
    QAction* billingAction = accountMenu->addItem(QString(), ShadIconType::CreditCard, ShadKbdGroup::shortcutText(QKeySequence(Qt::CTRL | Qt::Key_B)));
    QAction* settingsAction = accountMenu->addItem(QString(), ShadIconType::Settings, ShadKbdGroup::shortcutText(QKeySequence(Qt::CTRL | Qt::Key_S)));
    QAction* shortcutsAction = accountMenu->addItem(QString(), ShadIconType::Keyboard, ShadKbdGroup::shortcutText(QKeySequence(Qt::CTRL | Qt::Key_K)));
    accountMenu->addSeparator();
    QAction* teamAction = accountMenu->addItem(QString(), ShadIconType::Users);
    QAction* inviteAction = accountMenu->addItem(QString(), ShadIconType::UserPlus);
    accountMenu->addSeparator();
    accountMenu->addItem(QStringLiteral("GitHub"), ShadIconType::Github);
    QAction* supportAction = accountMenu->addItem(QString(), ShadIconType::LifeBuoy);
    QAction* apiAction = accountMenu->addItem(QStringLiteral("API"), ShadIconType::Cloud);
    apiAction->setEnabled(false);
    accountMenu->addSeparator();
    QAction* logoutAction = accountMenu->addItem(QString(), ShadIconType::LogOut, ShadKbdGroup::shortcutText(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Q)));
    ExampleI18n::bind(accountMenu, [accountButton, profileAction, billingAction, settingsAction, shortcutsAction, teamAction, inviteAction, supportAction, logoutAction]()
    {
        accountButton->setText(tr("打开菜单"));
        profileAction->setText(tr("个人资料"));
        billingAction->setText(tr("账单"));
        settingsAction->setText(tr("设置"));
        shortcutsAction->setText(tr("键盘快捷键"));
        teamAction->setText(tr("团队"));
        inviteAction->setText(tr("邀请成员"));
        supportAction->setText(tr("帮助与支持"));
        logoutAction->setText(tr("退出登录"));
    });
    ShadLabel* accountLabel = new ShadLabel(this);
    accountLabel->setColorRole(ShadThemeType::MutedForeground);
    // 最近点击的菜单项，菜单项文字先于此处刷新（连接顺序在前）
    const std::shared_ptr<QPointer<QAction>> lastAction = std::make_shared<QPointer<QAction>>();
    const auto updateAccountLabel = [accountLabel, lastAction]()
    {
        const QAction* action = lastAction->data();
        accountLabel->setText(action != nullptr ? tr("最近点击: %1").arg(action->text()) : tr("最近点击: 无"));
    };
    connect(accountButton, &ShadButton::clicked, accountMenu, [accountMenu, accountButton]() { accountMenu->popup(accountButton); });
    connect(accountMenu, &ShadDropdownMenu::itemTriggered, accountLabel, [lastAction, updateAccountLabel](QAction* action)
    {
        *lastAction = action;
        updateAccountLabel();
    });
    ExampleI18n::bind(accountLabel, updateAccountLabel);
    addRow(basicCard, {accountButton, accountLabel});

    ShadCard* checkableCard = addSection([] { return tr("复选项"); }, [] { return tr("点击复选项切换勾选，菜单保持打开"); });
    ShadButton* viewButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    viewButton->setShadIcon(ShadIconType::PanelTop);
    ShadDropdownMenu* viewMenu = new ShadDropdownMenu(this);
    const int viewMenuLabel = viewMenu->addLabel(QString());
    ExampleI18n::bind(viewMenu, [viewMenu, viewMenuLabel]() { viewMenu->setLabelText(viewMenuLabel, tr("外观")); });
    viewMenu->addSeparator();
    QAction* statusBarAction = viewMenu->addCheckableItem(QString(), true);
    QAction* activityBarAction = viewMenu->addCheckableItem(QString(), false);
    activityBarAction->setEnabled(false);
    QAction* panelAction = viewMenu->addCheckableItem(QString(), false);
    ShadLabel* viewLabel = new ShadLabel(this);
    viewLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updateViewLabel = [viewLabel, statusBarAction, panelAction]() { viewLabel->setText(tr("状态栏: %1    面板: %2").arg(statusBarAction->isChecked() ? tr("显示") : tr("隐藏"), panelAction->isChecked() ? tr("显示") : tr("隐藏"))); };
    ExampleI18n::bind(viewMenu, [viewButton, statusBarAction, activityBarAction, panelAction, updateViewLabel]()
    {
        viewButton->setText(tr("外观"));
        statusBarAction->setText(tr("状态栏"));
        activityBarAction->setText(tr("活动栏"));
        panelAction->setText(tr("面板"));
        updateViewLabel();
    });
    connect(statusBarAction, &QAction::toggled, viewLabel, updateViewLabel);
    connect(panelAction, &QAction::toggled, viewLabel, updateViewLabel);
    connect(viewButton, &ShadButton::clicked, viewMenu, [viewMenu, viewButton]() { viewMenu->popup(viewButton); });
    addRow(checkableCard, {viewButton, viewLabel});

    ShadCard* destructiveCard = addSection([] { return tr("危险操作与右对齐"); }, [] { return tr("危险操作样式，菜单与按钮右对齐弹出"); });
    ShadButton* moreButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    moreButton->setButtonSize(ShadButtonType::Icon);
    moreButton->setShadIcon(ShadIconType::Ellipsis);
    ShadDropdownMenu* moreMenu = new ShadDropdownMenu(this);
    QAction* editAction = moreMenu->addItem(QString(), ShadIconType::Pencil);
    QAction* duplicateAction = moreMenu->addItem(QString(), ShadIconType::Copy, ShadKbdGroup::shortcutText(QKeySequence(Qt::CTRL | Qt::Key_D)));
    moreMenu->addSeparator();
    QAction* deleteAction = moreMenu->addItem(QString(), ShadIconType::Trash2);
    moreMenu->setItemDestructive(deleteAction, true);
    ExampleI18n::bind(moreMenu, [editAction, duplicateAction, deleteAction]()
    {
        editAction->setText(tr("编辑"));
        duplicateAction->setText(tr("复制"));
        deleteAction->setText(tr("删除"));
    });
    connect(moreButton, &ShadButton::clicked, moreMenu, [moreMenu, moreButton]() { moreMenu->popup(moreButton, Qt::AlignRight); });
    addRow(destructiveCard, {moreButton});
}

DropdownMenuPage::~DropdownMenuPage()
{
}
