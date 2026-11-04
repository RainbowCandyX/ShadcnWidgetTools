#include "InputGroupPage.h"

#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QPlainTextEdit>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDropdownMenu.h"
#include "ShadInput.h"
#include "ShadInputGroup.h"
#include "ShadKbd.h"
#include "ShadLabel.h"
#include "ShadTextarea.h"
#include "ShadToaster.h"

namespace
{
    // 演示列宽 max-w-sm，加上焦点环边距
    constexpr int kColumnWidth = 384 + 2 * ExampleLayout::kFocusRingWidth;
    // 多行输入示例的字数上限
    constexpr int kMessageLimit = 280;

    // 纵向排列多个输入组的容器，可视间距 gap-6
    QWidget* createColumn(QWidget* parent)
    {
        QWidget* column = new QWidget(parent);
        column->setFixedWidth(kColumnWidth);
        QVBoxLayout* layout = new QVBoxLayout(column);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(24 - 2 * ExampleLayout::kFocusRingWidth);
        return column;
    }

    // placeholder 为空函数时不设置占位文字；占位文字随语言刷新，输入内容不受影响
    ShadInputGroup* createInputGroup(ExampleI18n::TextFunction placeholder, QWidget* parent)
    {
        ShadInputGroup* group = new ShadInputGroup(parent);
        if (placeholder)
        {
            ExampleI18n::bind(group, [group, placeholder]() { group->getInput()->setPlaceholderText(placeholder()); });
        }
        parent->layout()->addWidget(group);
        return group;
    }
} // namespace

InputGroupPage::InputGroupPage(ShadToaster* toaster, QWidget* parent)
    : ExampleBasePage([] { return tr("输入组"); }, [] { return tr("ShadInputGroup：在输入框两侧或上下加入文字、图标、按钮等附加区的输入组。"); }, parent)
{
    using Position = ShadInputGroupType::AddonPosition;
    const auto notify = [toaster](const QString& title, const QString& description)
    {
        if (toaster != nullptr)
        {
            toaster->add(title, description);
        }
    };

    ShadCard* iconCard = addSection([] { return tr("图标"); }, [] { return tr("在输入框起始侧或结束侧放置图标"); });
    QWidget* iconColumn = createColumn(this);
    createInputGroup([] { return tr("搜索..."); }, iconColumn)->addIcon(Position::InlineStart, ShadIconType::Search);
    ShadInputGroup* mailGroup = createInputGroup([] { return tr("输入邮箱"); }, iconColumn);
    mailGroup->addIcon(Position::InlineStart, ShadIconType::Mail);
    QWidget* mailInfoIcon = mailGroup->addIcon(Position::InlineEnd, ShadIconType::Info);
    addItem(iconCard, iconColumn);

    ShadCard* textCard = addSection([] { return tr("文字"); }, [] { return tr("用作前缀、后缀与单位的文字"); });
    QWidget* textColumn = createColumn(this);
    ShadInputGroup* priceGroup = createInputGroup([] { return QStringLiteral("0.00"); }, textColumn);
    priceGroup->addText(Position::InlineStart, QStringLiteral("$"));
    priceGroup->addText(Position::InlineEnd, QStringLiteral("USD"));
    ShadInputGroup* siteGroup = createInputGroup([] { return QStringLiteral("example"); }, textColumn);
    siteGroup->addText(Position::InlineStart, QStringLiteral("https://"));
    siteGroup->addText(Position::InlineEnd, QStringLiteral(".com"));
    ShadInputGroup* userGroup = createInputGroup([] { return tr("用户名"); }, textColumn);
    userGroup->addText(Position::InlineEnd, QStringLiteral("@company.com"));
    addItem(textCard, textColumn);

    ShadCard* buttonCard = addSection([] { return tr("按钮"); }, [] { return tr("在输入框内放置不同尺寸的按钮"); });
    QWidget* buttonColumn = createColumn(this);
    ShadInputGroup* linkGroup = createInputGroup(ExampleI18n::TextFunction(), buttonColumn);
    linkGroup->getInput()->setText(QStringLiteral("https://github.com/RainbowCandyX/ShadcnWidgetTools"));
    linkGroup->getInput()->setReadOnly(true);
    ShadButton* copyButton = linkGroup->addButton(Position::InlineEnd, QString(), ShadIconType::Copy, ShadButtonType::IconExtraSmall);
    connect(copyButton, &ShadButton::clicked, this, [notify]() { notify(tr("已复制"), tr("链接已复制到剪贴板")); });
    ShadInputGroup* searchGroup = createInputGroup([] { return tr("输入关键词..."); }, buttonColumn);
    ShadButton* searchButton = searchGroup->addButton(Position::InlineEnd, QString());
    connect(searchButton, &ShadButton::clicked, this, [notify, searchGroup]() { notify(tr("搜索"), searchGroup->getInput()->text()); });
    ShadInputGroup* secureGroup = createInputGroup([] { return QStringLiteral("example.com"); }, buttonColumn);
    secureGroup->addText(Position::InlineStart, QStringLiteral("https://"));
    ShadButton* infoButton = secureGroup->addButton(Position::InlineStart, QString(), ShadIconType::Info, ShadButtonType::IconExtraSmall);
    ShadButton* favoriteButton = secureGroup->addButton(Position::InlineEnd, QString(), ShadIconType::Star, ShadButtonType::IconExtraSmall);
    addItem(buttonCard, buttonColumn);

    ShadCard* kbdCard = addSection([] { return tr("快捷键"); }, [] { return tr("放入按键组，按快捷键聚焦搜索框"); });
    QWidget* kbdColumn = createColumn(this);
    ShadInputGroup* commandGroup = createInputGroup([] { return tr("搜索..."); }, kbdColumn);
    commandGroup->addIcon(Position::InlineStart, ShadIconType::Search);
    ShadKbdGroup* commandKeys = new ShadKbdGroup(commandGroup);
    commandKeys->setKeySequence(QKeySequence(Qt::CTRL | Qt::Key_K));
    commandKeys->setShortcutEnabled(true);
    connect(commandKeys, &ShadKbdGroup::activated, commandGroup, [commandGroup]()
    {
        commandGroup->getInput()->setFocus(Qt::ShortcutFocusReason);
        commandGroup->getInput()->selectAll();
    });
    commandGroup->addWidget(Position::InlineEnd, commandKeys);
    addItem(kbdCard, kbdColumn);

    ShadCard* blockCard = addSection([] { return tr("上下附加区"); }, [] { return tr("附加区在输入框上方或下方单独占一行"); });
    QWidget* blockColumn = createColumn(this);
    ShadInputGroup* nameGroup = createInputGroup([] { return tr("张三"); }, blockColumn);
    ShadLabel* nameLabel = nameGroup->addText(Position::BlockStart, QString());
    nameGroup->addStretch(Position::BlockStart);
    QWidget* nameInfoIcon = nameGroup->addIcon(Position::BlockStart, ShadIconType::Info);
    ShadInputGroup* fileGroup = createInputGroup([] { return QStringLiteral("console.log(\"Hello\");"); }, blockColumn);
    fileGroup->addIcon(Position::BlockEnd, ShadIconType::FileCode);
    fileGroup->addText(Position::BlockEnd, QStringLiteral("script.js"));
    fileGroup->addStretch(Position::BlockEnd);
    ShadButton* runButton = fileGroup->addButton(Position::BlockEnd, QString(), ShadIconType::Play);
    connect(runButton, &ShadButton::clicked, this, [notify]() { notify(tr("运行"), QStringLiteral("script.js")); });
    addItem(blockCard, blockColumn);

    ShadCard* textareaCard = addSection([] { return tr("多行输入"); }, [] { return tr("高度随内容增长，底部显示字数与发送按钮"); });
    QWidget* textareaColumn = createColumn(this);
    ShadInputGroup* messageGroup = new ShadInputGroup(ShadInputGroupType::PlainText, textareaColumn);
    messageGroup->getTextarea()->setMaximumAutoHeight(160);
    ShadLabel* counterLabel = messageGroup->addText(Position::BlockEnd, QStringLiteral("0/%1").arg(kMessageLimit));
    messageGroup->addStretch(Position::BlockEnd);
    ShadButton* sendButton = messageGroup->addButton(Position::BlockEnd, QString(), ShadIconType::ArrowUp, ShadButtonType::IconExtraSmall);
    sendButton->setVariant(ShadButtonType::Default);
    ShadTextarea* messageTextarea = messageGroup->getTextarea();
    connect(messageTextarea, &QPlainTextEdit::textChanged, counterLabel, [counterLabel, messageTextarea, messageGroup]()
    {
        const int length = int(messageTextarea->toPlainText().size());
        counterLabel->setText(QStringLiteral("%1/%2").arg(length).arg(kMessageLimit));
        messageGroup->setInvalid(length > kMessageLimit);
    });
    connect(sendButton, &ShadButton::clicked, this, [notify, messageTextarea]()
    {
        notify(tr("已发送"), messageTextarea->toPlainText().isEmpty() ? tr("（空消息）") : messageTextarea->toPlainText());
        messageTextarea->clear();
    });
    textareaColumn->layout()->addWidget(messageGroup);
    addItem(textareaCard, textareaColumn);

    ShadCard* loadingCard = addSection([] { return tr("加载状态"); }, [] { return tr("自动旋转的图标作为加载指示"); });
    QWidget* loadingColumn = createColumn(this);
    ShadInputGroup* searchingGroup = createInputGroup([] { return tr("正在搜索..."); }, loadingColumn);
    searchingGroup->addIcon(Position::InlineEnd, ShadIconType::LoaderCircle);
    searchingGroup->setEnabled(false);
    ShadInputGroup* savingGroup = createInputGroup([] { return tr("保存中..."); }, loadingColumn);
    savingGroup->addIcon(Position::InlineStart, ShadIconType::LoaderCircle);
    ShadLabel* autoSaveLabel = savingGroup->addText(Position::InlineEnd, QString());
    addItem(loadingCard, loadingColumn);

    ShadCard* dropdownCard = addSection([] { return tr("下拉菜单"); }, [] { return tr("结束侧的按钮弹出下拉菜单"); });
    QWidget* dropdownColumn = createColumn(this);
    ShadInputGroup* queryGroup = createInputGroup([] { return tr("输入搜索内容..."); }, dropdownColumn);
    ShadButton* scopeButton = queryGroup->addButton(Position::InlineEnd, QString(), ShadIconType::ChevronDown);
    ShadDropdownMenu* scopeMenu = new ShadDropdownMenu(this);
    const QList<QAction*> scopeActions{scopeMenu->addItem(QString()), scopeMenu->addItem(QString()), scopeMenu->addItem(QString())};
    // 当前选中的范围序号，-1 表示尚未选择；按钮文字按序号与当前语言生成
    std::shared_ptr<int> scopeIndex = std::make_shared<int>(-1);
    const auto updateScopeButton = [scopeButton, scopeActions, scopeIndex]() { scopeButton->setText(*scopeIndex < 0 ? tr("范围") : scopeActions.at(*scopeIndex)->text()); };
    connect(scopeButton, &ShadButton::clicked, scopeMenu, [scopeMenu, queryGroup]() { scopeMenu->popup(queryGroup, Qt::AlignRight); });
    connect(scopeMenu, &ShadDropdownMenu::itemTriggered, scopeButton, [scopeActions, scopeIndex, updateScopeButton](QAction* action)
    {
        *scopeIndex = int(scopeActions.indexOf(action));
        updateScopeButton();
    });
    // 先刷新菜单项，再按刷新后的菜单项文字刷新按钮
    ExampleI18n::bind(scopeMenu, [scopeActions, updateScopeButton]()
    {
        scopeActions.at(0)->setText(tr("全部"));
        scopeActions.at(1)->setText(tr("文档"));
        scopeActions.at(2)->setText(tr("代码"));
        updateScopeButton();
    });
    addItem(dropdownCard, dropdownColumn);

    ShadCard* invalidCard = addSection([] { return tr("错误态"); }, [] { return tr("setInvalid(true)：边框与焦点环显示错误色"); });
    QWidget* invalidColumn = createColumn(this);
    ShadInputGroup* invalidGroup = createInputGroup([] { return tr("请输入有效的邮箱"); }, invalidColumn);
    invalidGroup->addIcon(Position::InlineStart, ShadIconType::Mail);
    invalidGroup->getInput()->setText(QStringLiteral("not-an-email"));
    invalidGroup->setInvalid(true);
    addItem(invalidCard, invalidColumn);

    ExampleI18n::bind(this, [mailInfoIcon, copyButton, searchButton, infoButton, favoriteButton, nameLabel, nameInfoIcon, runButton, messageTextarea, sendButton, autoSaveLabel]()
    {
        mailInfoIcon->setToolTip(tr("我们不会公开你的邮箱"));
        copyButton->setToolTip(tr("复制"));
        searchButton->setText(tr("搜索"));
        infoButton->setToolTip(tr("该连接是安全的"));
        favoriteButton->setToolTip(tr("收藏"));
        nameLabel->setText(tr("全名"));
        nameInfoIcon->setToolTip(tr("将显示在你的个人资料中"));
        runButton->setText(tr("运行"));
        messageTextarea->setPlaceholderText(tr("向 AI 助手提问、搜索或生成内容..."));
        sendButton->setToolTip(tr("发送"));
        autoSaveLabel->setText(tr("自动保存"));
    });
}

InputGroupPage::~InputGroupPage()
{
}
