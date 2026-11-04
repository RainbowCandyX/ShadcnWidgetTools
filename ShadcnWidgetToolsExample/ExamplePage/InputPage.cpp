#include "InputPage.h"

#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadTextarea.h"

InputPage::InputPage(QWidget* parent)
    : ExampleBasePage([] { return tr("输入框"); }, [] { return tr("ShadInput / ShadTextarea / ShadLabel：单行、多行输入框与标签，获得焦点时显示焦点环，支持错误态。"); }, parent)
{
    ShadCard* inputCard = addSection([] { return tr("基础用法"); }, [] { return tr("单行输入框"); });
    ShadInput* emailInput = new ShadInput(this);
    ExampleI18n::bind(emailInput, [emailInput]() { emailInput->setPlaceholderText(tr("邮箱")); });
    emailInput->setFixedWidth(ExampleLayout::kFieldWidth);
    addRow(inputCard, {emailInput});

    ShadCard* labelCard = addSection([] { return tr("搭配标签"); }, [] { return tr("点击标签会把焦点交给关联的输入框"); });
    ShadInput* labeledInput = new ShadInput(this);
    labeledInput->setPlaceholderText(QStringLiteral("m@example.com"));
    ShadInput* passwordInput = new ShadInput(this);
    // 尾部按钮切换密码显示，同时把 echoMode 设为 Password
    passwordInput->setTrailingAction(ShadInputType::PasswordToggleAction);
    ExampleI18n::bind(passwordInput, [passwordInput]() { passwordInput->setPlaceholderText(tr("请输入密码")); });
    addRow(labelCard, {createField([] { return tr("邮箱"); }, labeledInput), createField([] { return tr("密码"); }, passwordInput)});

    ShadCard* decorationCard = addSection([] { return tr("图标与尾部按钮"); }, [] { return tr("前置图标，以及清空或切换密码显示的尾部按钮"); });
    ShadInput* searchInput = new ShadInput(this);
    ExampleI18n::bind(searchInput, [searchInput]() { searchInput->setPlaceholderText(tr("搜索组件")); });
    searchInput->setLeadingIcon(ShadIconType::Search);
    searchInput->setTrailingAction(ShadInputType::ClearAction);
    ShadInput* secretInput = new ShadInput(this);
    ExampleI18n::bind(secretInput, [secretInput]() { secretInput->setPlaceholderText(tr("请输入密码")); });
    secretInput->setLeadingIcon(ShadIconType::Lock);
    secretInput->setTrailingAction(ShadInputType::PasswordToggleAction);
    addRow(decorationCard, {createField([] { return tr("搜索"); }, searchInput), createField([] { return tr("密码"); }, secretInput)});

    ShadCard* stateCard = addSection([] { return tr("状态"); }, [] { return tr("禁用与错误态，禁用时关联的标签也变为半透明"); });
    ShadInput* disabledInput = new ShadInput(this);
    ExampleI18n::bind(disabledInput, [disabledInput]() { disabledInput->setPlaceholderText(tr("已禁用")); });
    disabledInput->setEnabled(false);
    ShadInput* invalidInput = new ShadInput(this);
    // 输入内容可由用户修改，只在创建时设置一次，切换语言时不覆盖
    invalidInput->setText(tr("不是有效的邮箱地址"));
    invalidInput->setInvalid(true);
    addRow(stateCard, {createField([] { return tr("禁用"); }, disabledInput), createField([] { return tr("错误"); }, invalidInput)});

    ShadCard* textareaCard = addSection([] { return tr("多行输入"); }, [] { return tr("高度随内容增长，超出最大高度后出现滚动条"); });
    ShadTextarea* messageTextarea = new ShadTextarea(this);
    ExampleI18n::bind(messageTextarea, [messageTextarea]() { messageTextarea->setPlaceholderText(tr("在这里输入你的留言，换行或输入长文本时高度会自动增长。")); });
    messageTextarea->setFixedWidth(ExampleLayout::kWideFieldWidth);
    messageTextarea->setMaximumAutoHeight(160);
    ShadTextarea* disabledTextarea = new ShadTextarea(this);
    ExampleI18n::bind(disabledTextarea, [disabledTextarea]() { disabledTextarea->setPlaceholderText(tr("已禁用")); });
    disabledTextarea->setEnabled(false);
    disabledTextarea->setFixedWidth(ExampleLayout::kFieldWidth);
    // 自动增高的文本框纵向排列，避免同一行内其他组件跟着上下移动
    addItem(textareaCard, createField([] { return tr("留言"); }, messageTextarea));
    addItem(textareaCard, createField([] { return tr("禁用"); }, disabledTextarea));
}

InputPage::~InputPage()
{
}

QWidget* InputPage::createField(ExampleI18n::TextFunction labelText, QWidget* field)
{
    QWidget* container = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    // gap-3 扣除输入框上方 3px 焦点环边距
    layout->setSpacing(12 - ExampleLayout::kFocusRingWidth);
    ShadLabel* label = new ShadLabel(container);
    ExampleI18n::bind(label, [label, labelText]() { label->setText(labelText()); });
    label->setBuddy(field);
    // 标签与输入框可视边缘对齐；contentsMargins 不随布局方向镜像，左右对称设置
    label->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, ExampleLayout::kFocusRingWidth, 0);
    layout->addWidget(label);
    layout->addWidget(field);
    if (field->minimumWidth() == 0)
    {
        field->setFixedWidth(ExampleLayout::kFieldWidth);
    }
    return container;
}
