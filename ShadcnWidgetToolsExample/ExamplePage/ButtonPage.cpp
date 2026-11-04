#include "ButtonPage.h"

#include <QAction>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <initializer_list>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadButtonGroup.h"
#include "ShadCard.h"
#include "ShadComboBox.h"
#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadInputGroup.h"
#include "ShadLabel.h"
#include "ShadPopover.h"
#include "ShadTextarea.h"
#include "ShadToaster.h"

namespace
{
    ShadButton* createIconButton(ShadIconType::IconName icon, const ExampleI18n::TextFunction& toolTip, ShadButtonType::Size size, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, parent);
        button->setButtonSize(size);
        button->setShadIcon(icon);
        ExampleI18n::bind(button, [button, toolTip]()
        {
            const QString text = toolTip();
            button->setToolTip(text);
            button->setAccessibleName(text);
        });
        return button;
    }

    // 文字随语言刷新的按钮
    ShadButton* createTextButton(const ExampleI18n::TextFunction& text, ShadButtonType::Variant variant, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), variant, parent);
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
        return button;
    }

    // 菜单项文字随语言刷新，QAction::setText 会触发菜单重绘
    QAction* addMenuItem(ShadDropdownMenu* menu, const ExampleI18n::TextFunction& text, ShadIconType::IconName icon)
    {
        QAction* action = menu->addItem(text(), icon);
        ExampleI18n::bind(action, [action, text]() { action->setText(text()); });
        return action;
    }
} // namespace

ButtonPage::ButtonPage(ShadToaster* toaster, QWidget* parent)
    : ExampleBasePage([] { return tr("按钮"); }, [] { return tr("ShadButton / ShadButtonGroup：多种变体与尺寸的按钮，以及把相关控件拼接成组的按钮组。"); }, parent)
{
    ShadCard* variantCard = addSection([] { return tr("变体"); }, [] { return tr("默认、危险、描边、次要、幽灵与链接六种变体"); });
    addRow(variantCard, {createTextButton([] { return tr("默认"); }, ShadButtonType::Default, this), createTextButton([] { return tr("删除"); }, ShadButtonType::Destructive, this), createTextButton([] { return tr("描边"); }, ShadButtonType::Outline, this), createTextButton([] { return tr("次要"); }, ShadButtonType::Secondary, this), createTextButton([] { return tr("幽灵"); }, ShadButtonType::Ghost, this), createTextButton([] { return tr("链接"); }, ShadButtonType::Link, this)});

    ShadCard* sizeCard = addSection([] { return tr("尺寸"); }, [] { return tr("四种文字按钮尺寸与四种方形图标按钮尺寸"); });
    QList<QWidget*> sizeWidgets;
    const QList<ShadButtonType::Size> textSizes{
        ShadButtonType::ExtraSmall, ShadButtonType::Small, ShadButtonType::Medium, ShadButtonType::Large
    };
    const QList<ExampleI18n::TextFunction> textSizeNames{
        [] { return tr("超小"); }, [] { return tr("小"); }, [] { return tr("默认"); }, [] { return tr("大"); }
    };
    for (int i = 0; i < textSizes.size(); ++i)
    {
        ShadButton* button = createTextButton(textSizeNames.at(i), ShadButtonType::Outline, this);
        button->setButtonSize(textSizes.at(i));
        sizeWidgets.append(button);
    }
    const QList<ShadButtonType::Size> iconSizes{
        ShadButtonType::IconExtraSmall, ShadButtonType::IconSmall, ShadButtonType::Icon, ShadButtonType::IconLarge
    };
    for (ShadButtonType::Size size : iconSizes)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, this);
        button->setButtonSize(size);
        button->setShadIcon(ShadIconType::ArrowUpRight);
        ExampleI18n::bind(button, [button]() { button->setToolTip(tr("图标按钮")); });
        sizeWidgets.append(button);
    }
    addRow(sizeCard, sizeWidgets);

    ShadCard* iconCard = addSection([] { return tr("带图标"); }, [] { return tr("图标位于文字左侧的按钮"); });
    ShadButton* branchButton = createTextButton([] { return tr("新建分支"); }, ShadButtonType::Outline, this);
    branchButton->setButtonSize(ShadButtonType::Small);
    branchButton->setShadIcon(ShadIconType::GitBranch);
    ShadButton* mailButton = createTextButton([] { return tr("邮箱登录"); }, ShadButtonType::Default, this);
    mailButton->setShadIcon(ShadIconType::Mail);
    ShadButton* deleteButton = createTextButton([] { return tr("删除"); }, ShadButtonType::Destructive, this);
    deleteButton->setShadIcon(ShadIconType::Trash2);
    ShadButton* sendButton = createTextButton([] { return tr("发送"); }, ShadButtonType::Secondary, this);
    sendButton->setShadIcon(ShadIconType::Send);
    addRow(iconCard, {branchButton, mailButton, deleteButton, sendButton});

    ShadCard* disabledCard = addSection([] { return tr("禁用"); }, [] { return tr("禁用后半透明，不响应悬停与点击"); });
    ShadButton* loadingButton = createTextButton([] { return tr("请稍候"); }, ShadButtonType::Default, this);
    loadingButton->setShadIcon(ShadIconType::LoaderCircle);
    loadingButton->setEnabled(false);
    ShadButton* disabledOutline = createTextButton([] { return tr("描边"); }, ShadButtonType::Outline, this);
    disabledOutline->setEnabled(false);
    ShadButton* disabledSecondary = createTextButton([] { return tr("次要"); }, ShadButtonType::Secondary, this);
    disabledSecondary->setEnabled(false);
    addRow(disabledCard, {loadingButton, disabledOutline, disabledSecondary});

    ShadCard* stateCard = addSection([] { return tr("加载状态"); }, [] { return tr("setLoading(true)：显示加载图标，期间不响应点击"); });
    ShadButton* submitButton = createTextButton([] { return tr("提交"); }, ShadButtonType::Default, this);
    ShadButton* saveButton = createTextButton([] { return tr("保存草稿"); }, ShadButtonType::Outline, this);
    saveButton->setShadIcon(ShadIconType::Save);
    ShadButton* refreshButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    refreshButton->setButtonSize(ShadButtonType::Icon);
    refreshButton->setShadIcon(ShadIconType::RefreshCw);
    ExampleI18n::bind(refreshButton, [refreshButton]() { refreshButton->setToolTip(tr("刷新")); });
    for (ShadButton* button : {submitButton, saveButton, refreshButton})
    {
        connect(button, &ShadButton::clicked, button, [button]()
        {
            button->setLoading(true);
            QTimer::singleShot(2000, button, [button]() { button->setLoading(false); });
        });
    }
    addRow(stateCard, {submitButton, saveButton, refreshButton});

    addButtonGroupSections(toaster);
}

ButtonPage::~ButtonPage()
{
}

void ButtonPage::addButtonGroupSections(ShadToaster* toaster)
{
    const auto notify = [toaster](const QString& title, const QString& description)
    {
        if (toaster != nullptr)
        {
            toaster->add(title, description);
        }
    };
    const auto notifyAction = [notify](QAction* action) { notify(tr("菜单项"), action->text()); };

    ShadCard* basicCard = addSection([] { return tr("按钮组：基础用法"); }, [] { return tr("三个子分组嵌套在一个分组中，末尾带下拉菜单"); });
    ShadButtonGroup* toolbar = new ShadButtonGroup(this);
    ShadButtonGroup* backGroup = new ShadButtonGroup(toolbar);
    backGroup->addWidget(createIconButton(ShadIconType::ArrowLeft, [] { return tr("返回"); }, ShadButtonType::Icon, backGroup));
    ShadButtonGroup* actionGroup = new ShadButtonGroup(toolbar);
    actionGroup->addWidget(createTextButton([] { return tr("归档"); }, ShadButtonType::Outline, actionGroup));
    actionGroup->addWidget(createTextButton([] { return tr("举报"); }, ShadButtonType::Outline, actionGroup));
    ShadButtonGroup* snoozeGroup = new ShadButtonGroup(toolbar);
    snoozeGroup->addWidget(createTextButton([] { return tr("稍后提醒"); }, ShadButtonType::Outline, snoozeGroup));
    ShadButton* moreButton = createIconButton(ShadIconType::Ellipsis, [] { return tr("更多选项"); }, ShadButtonType::Icon, snoozeGroup);
    snoozeGroup->addWidget(moreButton);
    ShadDropdownMenu* moreMenu = new ShadDropdownMenu(this);
    addMenuItem(moreMenu, [] { return tr("标为已读"); }, ShadIconType::MailCheck);
    addMenuItem(moreMenu, [] { return tr("归档"); }, ShadIconType::Archive);
    moreMenu->addSeparator();
    addMenuItem(moreMenu, [] { return tr("稍后提醒"); }, ShadIconType::Clock);
    addMenuItem(moreMenu, [] { return tr("添加到日历"); }, ShadIconType::CalendarPlus);
    addMenuItem(moreMenu, [] { return tr("添加到列表"); }, ShadIconType::ListFilter);
    moreMenu->addSeparator();
    moreMenu->setItemDestructive(addMenuItem(moreMenu, [] { return tr("删除"); }, ShadIconType::Trash2), true);
    connect(moreButton, &ShadButton::clicked, moreMenu, [moreMenu, moreButton]()
    {
        moreMenu->popup(moreButton, Qt::AlignRight);
    });
    connect(moreMenu, &ShadDropdownMenu::itemTriggered, this, notifyAction);
    toolbar->addWidget(backGroup);
    toolbar->addWidget(actionGroup);
    toolbar->addWidget(snoozeGroup);
    addItem(basicCard, toolbar);

    ShadCard* orientationCard = addSection([] { return tr("按钮组：方向"); }, []
    {
        return tr("setOrientation(Qt::Vertical)：纵向排列，上下拼接");
    });
    ShadButtonGroup* verticalGroup = new ShadButtonGroup(Qt::Vertical, this);
    verticalGroup->addWidget(createIconButton(ShadIconType::Plus, [] { return tr("增加"); }, ShadButtonType::Icon, verticalGroup));
    verticalGroup->addWidget(createIconButton(ShadIconType::Minus, [] { return tr("减少"); }, ShadButtonType::Icon, verticalGroup));
    addItem(orientationCard, verticalGroup);

    ShadCard* sizeCard = addSection([] { return tr("按钮组：尺寸"); }, [] { return tr("按钮组不改变子组件的尺寸，组内使用相同尺寸的按钮"); });
    struct SizeDemo
    {
        ShadButtonType::Size textSize;
        ShadButtonType::Size iconSize;
        ExampleI18n::TextFunction name;
    };
    const QList<SizeDemo> sizeDemos{
        {ShadButtonType::Small, ShadButtonType::IconSmall, [] { return tr("小号"); }},
        {ShadButtonType::Medium, ShadButtonType::Icon, [] { return tr("默认"); }},
        {ShadButtonType::Large, ShadButtonType::IconLarge, [] { return tr("大号"); }},
    };
    for (const SizeDemo& demo : sizeDemos)
    {
        ShadButtonGroup* group = new ShadButtonGroup(this);
        const QList<ExampleI18n::TextFunction> texts{demo.name, [] { return tr("按钮"); }, [] { return tr("组"); }};
        for (const ExampleI18n::TextFunction& text : texts)
        {
            ShadButton* button = createTextButton(text, ShadButtonType::Outline, group);
            button->setButtonSize(demo.textSize);
            group->addWidget(button);
        }
        group->addWidget(createIconButton(ShadIconType::Plus, [] { return tr("添加"); }, demo.iconSize, group));
        addItem(sizeCard, group);
    }

    ShadCard* separatorCard = addSection([] { return tr("按钮组：分隔线"); }, [] { return tr("addSeparator：在无边框按钮之间加分隔线"); });
    ShadButtonGroup* clipboardGroup = new ShadButtonGroup(this);
    const QList<ExampleI18n::TextFunction> clipboardTexts{[] { return tr("复制"); }, [] { return tr("粘贴"); }};
    for (const ExampleI18n::TextFunction& text : clipboardTexts)
    {
        if (clipboardGroup->count() > 0)
        {
            clipboardGroup->addSeparator();
        }
        ShadButton* button = createTextButton(text, ShadButtonType::Secondary, clipboardGroup);
        button->setButtonSize(ShadButtonType::Small);
        clipboardGroup->addWidget(button);
    }
    addItem(separatorCard, clipboardGroup);

    ShadCard* splitCard = addSection([] { return tr("按钮组：拆分按钮"); }, [] { return tr("主操作按钮与附加操作的图标按钮，中间用分隔线隔开"); });
    ShadButtonGroup* splitGroup = new ShadButtonGroup(this);
    splitGroup->addWidget(createTextButton([] { return tr("按钮"); }, ShadButtonType::Secondary, splitGroup));
    splitGroup->addSeparator();
    ShadButton* splitIconButton = createIconButton(ShadIconType::Plus, [] { return tr("添加"); }, ShadButtonType::Icon, splitGroup);
    splitIconButton->setVariant(ShadButtonType::Secondary);
    splitGroup->addWidget(splitIconButton);
    addItem(splitCard, splitGroup);

    ShadCard* inputCard = addSection([] { return tr("按钮组：输入框"); }, [] { return tr("输入框与按钮拼接，输入框占据多余宽度"); });
    ShadButtonGroup* searchGroup = new ShadButtonGroup(this);
    ShadInput* searchInput = new ShadInput(searchGroup);
    ExampleI18n::bind(searchInput, [searchInput]() { searchInput->setPlaceholderText(tr("搜索...")); });
    ShadButton* searchButton = createIconButton(ShadIconType::Search, [] { return tr("搜索"); }, ShadButtonType::Icon, searchGroup);
    connect(searchButton, &ShadButton::clicked, this, [notify, searchInput]()
    {
        notify(tr("搜索"), searchInput->text().isEmpty() ? tr("（空）") : searchInput->text());
    });
    searchGroup->addWidget(searchInput);
    searchGroup->addWidget(searchButton);
    searchGroup->setMinimumWidth(320);
    addItem(inputCard, searchGroup);

    ShadCard* dropdownCard = addSection([] { return tr("按钮组：下拉菜单"); }, [] { return tr("主按钮加下拉箭头按钮，点击弹出菜单"); });
    ShadButtonGroup* followGroup = new ShadButtonGroup(this);
    ShadButton* followButton = createTextButton([] { return tr("关注"); }, ShadButtonType::Outline, followGroup);
    connect(followButton, &ShadButton::clicked, this, [notify]() { notify(tr("已关注"), tr("将收到该会话的新消息通知")); });
    ShadButton* followMenuButton = createIconButton(ShadIconType::ChevronDown, [] { return tr("更多操作"); }, ShadButtonType::Icon, followGroup);
    followGroup->addWidget(followButton);
    followGroup->addWidget(followMenuButton);
    ShadDropdownMenu* followMenu = new ShadDropdownMenu(this);
    addMenuItem(followMenu, [] { return tr("静音会话"); }, ShadIconType::VolumeOff);
    addMenuItem(followMenu, [] { return tr("标为已读"); }, ShadIconType::Check);
    addMenuItem(followMenu, [] { return tr("举报会话"); }, ShadIconType::TriangleAlert);
    addMenuItem(followMenu, [] { return tr("屏蔽用户"); }, ShadIconType::UserRoundX);
    addMenuItem(followMenu, [] { return tr("分享会话"); }, ShadIconType::Share);
    addMenuItem(followMenu, [] { return tr("复制会话"); }, ShadIconType::Copy);
    followMenu->addSeparator();
    followMenu->setItemDestructive(addMenuItem(followMenu, [] { return tr("删除会话"); }, ShadIconType::Trash2), true);
    // 菜单锚定整个按钮组，与其结束侧对齐（对应 align="end"）
    connect(followMenuButton, &ShadButton::clicked, followMenu, [followMenu, followGroup]()
    {
        followMenu->popup(followGroup, Qt::AlignRight);
    });
    connect(followMenu, &ShadDropdownMenu::itemTriggered, this, notifyAction);
    addItem(dropdownCard, followGroup);

    ShadCard* selectCard = addSection([] { return tr("按钮组：下拉选择"); }, [] { return tr("币种下拉框与金额输入框拼接，后跟发送按钮"); });
    ShadButtonGroup* paymentGroup = new ShadButtonGroup(this);
    ShadButtonGroup* amountGroup = new ShadButtonGroup(paymentGroup);
    ShadComboBox* currencyComboBox = new ShadComboBox(amountGroup);
    currencyComboBox->addItems({QStringLiteral("$"), QStringLiteral("€"), QStringLiteral("£")});
    ExampleI18n::bind(currencyComboBox, [currencyComboBox]() { currencyComboBox->setToolTip(tr("币种")); });
    ShadInput* amountInput = new ShadInput(amountGroup);
    amountInput->setPlaceholderText(QStringLiteral("10.00"));
    amountGroup->addWidget(currencyComboBox);
    amountGroup->addWidget(amountInput);
    ShadButtonGroup* sendGroup = new ShadButtonGroup(paymentGroup);
    ShadButton* sendButton = createIconButton(ShadIconType::ArrowRight, [] { return tr("发送"); }, ShadButtonType::Icon, sendGroup);
    connect(sendButton, &ShadButton::clicked, this, [notify, currencyComboBox, amountInput]()
    {
        notify(tr("发送"), currencyComboBox->currentText() + (amountInput->text().isEmpty() ? QStringLiteral("0.00") : amountInput->text()));
    });
    sendGroup->addWidget(sendButton);
    paymentGroup->addWidget(amountGroup);
    paymentGroup->addWidget(sendGroup);
    addItem(selectCard, paymentGroup);

    ShadCard* textCard = addSection([] { return tr("按钮组：文本"); }, [] { return tr("addText：在按钮组中添加文字块，可带图标"); });
    ShadButtonGroup* urlGroup = new ShadButtonGroup(this);
    urlGroup->addText(QStringLiteral("https://"), ShadIconType::Globe);
    ShadInput* urlInput = new ShadInput(urlGroup);
    urlInput->setPlaceholderText(QStringLiteral("example.com"));
    urlGroup->addWidget(urlInput);
    urlGroup->setMinimumWidth(360);
    addItem(textCard, urlGroup);

    // 对应 button-group-popover：箭头按钮弹出与按钮组结束侧对齐的浮层
    ShadCard* popoverCard = addSection([] { return tr("按钮组：浮层"); }, [] { return tr("下拉箭头按钮弹出带多行输入框的浮层"); });
    ShadButtonGroup* copilotGroup = new ShadButtonGroup(this);
    ShadButton* copilotButton = new ShadButton(QStringLiteral("Copilot"), ShadButtonType::Outline, copilotGroup);
    copilotButton->setShadIcon(ShadIconType::Bot);
    ShadButton* copilotMenuButton = createIconButton(ShadIconType::ChevronDown, [] { return tr("打开浮层"); }, ShadButtonType::Icon, copilotGroup);
    copilotGroup->addWidget(copilotButton);
    copilotGroup->addWidget(copilotMenuButton);
    ShadPopover* copilotPopover = new ShadPopover(this);
    ShadTextarea* taskTextarea = new ShadTextarea(copilotPopover);
    ShadLabel* taskHint = new ShadLabel(copilotPopover);
    ExampleI18n::bind(copilotPopover, [copilotPopover, taskTextarea, taskHint]()
    {
        copilotPopover->setTitle(tr("使用 Copilot 开始新任务"));
        copilotPopover->setDescription(tr("用自然语言描述你的任务。"));
        taskTextarea->setPlaceholderText(tr("我需要……"));
        taskHint->setText(tr("Copilot 会创建一个拉取请求供你审阅。"));
    });
    taskHint->setColorRole(ShadThemeType::MutedForeground);
    taskHint->setWordWrap(true);
    taskHint->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, ExampleLayout::kFocusRingWidth, 0);
    copilotPopover->contentLayout()->setSpacing(8 - ExampleLayout::kFocusRingWidth);
    copilotPopover->contentLayout()->addWidget(taskTextarea);
    copilotPopover->contentLayout()->addWidget(taskHint);
    connect(copilotMenuButton, &ShadButton::clicked, copilotPopover, [copilotPopover, copilotGroup]()
    {
        copilotPopover->toggle(copilotGroup, ShadPlacementType::Bottom, ShadPlacementType::End);
    });
    connect(copilotButton, &ShadButton::clicked, this, [notify]()
    {
        notify(QStringLiteral("Copilot"), tr("已打开 Copilot 面板"));
    });
    addItem(popoverCard, copilotGroup);

    // 对应 button-group-input-group：输入组放进按钮组，语音按钮带提示并可切换
    ShadCard* inputGroupCard = addSection([] { return tr("按钮组：输入组与提示"); }, [] { return tr("输入组放进按钮组，语音按钮带悬停提示"); });
    ShadButtonGroup* messageToolbar = new ShadButtonGroup(this);
    ShadButtonGroup* plusGroup = new ShadButtonGroup(messageToolbar);
    plusGroup->addWidget(createIconButton(ShadIconType::Plus, [] { return tr("添加附件"); }, ShadButtonType::Icon, plusGroup));
    ShadButtonGroup* messageGroup = new ShadButtonGroup(messageToolbar);
    ShadInputGroup* messageInputGroup = new ShadInputGroup(messageGroup);
    messageInputGroup->setMinimumWidth(320);
    ShadButton* voiceButton = messageInputGroup->addButton(ShadInputGroupType::InlineEnd, QString(), ShadIconType::AudioLines, ShadButtonType::IconExtraSmall);
    // 占位文字按当前是否处于语音模式（按钮为 secondary 背景）生成
    const auto updateMessagePlaceholder = [voiceButton, messageInputGroup]()
    {
        const bool isVoiceEnabled = voiceButton->getVariant() == ShadButtonType::Secondary;
        messageInputGroup->getInput()->setPlaceholderText(isVoiceEnabled ? tr("录制并发送语音...") : tr("发送消息..."));
    };
    ExampleI18n::bind(messageInputGroup, [voiceButton, updateMessagePlaceholder]()
    {
        voiceButton->setToolTip(tr("语音模式"));
        updateMessagePlaceholder();
    });
    connect(voiceButton, &ShadButton::clicked, this, [voiceButton, messageInputGroup, updateMessagePlaceholder]()
    {
        // 语音模式下禁用文字输入，按钮以 secondary 背景表示已开启（对应 data-[active=true]）
        const bool isVoiceEnabled = voiceButton->getVariant() != ShadButtonType::Secondary;
        voiceButton->setVariant(isVoiceEnabled ? ShadButtonType::Secondary : ShadButtonType::Ghost);
        messageInputGroup->getInput()->setEnabled(!isVoiceEnabled);
        updateMessagePlaceholder();
    });
    messageGroup->addWidget(messageInputGroup);
    messageToolbar->addWidget(plusGroup);
    messageToolbar->addWidget(messageGroup);
    addItem(inputGroupCard, messageToolbar);
}
