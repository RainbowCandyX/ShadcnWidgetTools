#include "CardPage.h"

#include <QHBoxLayout>
#include <QSizePolicy>
#include <QString>
#include <QVBoxLayout>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadInput.h"
#include "ShadLabel.h"

CardPage::CardPage(QWidget* parent)
    : ExampleBasePage([] { return tr("卡片"); }, [] { return tr("ShadCard：由标题区、内容区与底部区组成的卡片容器。"); }, parent)
{
    ShadCard* demoCard = addSection([] { return tr("登录卡片"); }, [] { return tr("包含标题、操作、内容与底部四个部分"); });

    ShadCard* loginCard = new ShadCard(this);
    loginCard->setFixedWidth(400);
    ShadButton* signUpButton = new ShadButton(QString(), ShadButtonType::Link, loginCard);
    loginCard->setHeaderAction(signUpButton);

    QVBoxLayout* contentLayout = loginCard->contentLayout();
    // 字段之间 gap-6，标签与输入框之间 gap-2，均扣除输入框 3px 焦点环边距
    contentLayout->setSpacing(24 - 2 * ExampleLayout::kFocusRingWidth);
    ShadLabel* emailLabel = new ShadLabel(loginCard);
    emailLabel->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, ExampleLayout::kFocusRingWidth, 0);
    ShadInput* emailInput = new ShadInput(loginCard);
    emailInput->setPlaceholderText(QStringLiteral("m@example.com"));
    emailLabel->setBuddy(emailInput);
    ShadLabel* passwordLabel = new ShadLabel(loginCard);
    passwordLabel->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, ExampleLayout::kFocusRingWidth, 0);
    ShadInput* passwordInput = new ShadInput(loginCard);
    // 尾部按钮切换密码显示，同时把 echoMode 设为 Password
    passwordInput->setTrailingAction(ShadInputType::PasswordToggleAction);
    passwordLabel->setBuddy(passwordInput);
    QVBoxLayout* emailLayout = new QVBoxLayout();
    emailLayout->setSpacing(8 - ExampleLayout::kFocusRingWidth);
    emailLayout->addWidget(emailLabel);
    emailLayout->addWidget(emailInput);
    QVBoxLayout* passwordLayout = new QVBoxLayout();
    passwordLayout->setSpacing(8 - ExampleLayout::kFocusRingWidth);
    passwordLayout->addWidget(passwordLabel);
    passwordLayout->addWidget(passwordInput);
    contentLayout->addLayout(emailLayout);
    contentLayout->addLayout(passwordLayout);

    QHBoxLayout* footerLayout = loginCard->footerLayout();
    footerLayout->setSpacing(8 - 2 * ExampleLayout::kFocusRingWidth);
    ShadButton* loginButton = new ShadButton(QString(), ShadButtonType::Default, loginCard);
    loginButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ShadButton* googleButton = new ShadButton(QString(), ShadButtonType::Outline, loginCard);
    googleButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    footerLayout->addWidget(loginButton);
    footerLayout->addWidget(googleButton);

    ExampleI18n::bind(loginCard, [loginCard, signUpButton, emailLabel, passwordLabel, loginButton, googleButton]()
    {
        loginCard->setTitle(tr("登录你的账号"));
        loginCard->setDescription(tr("在下方输入邮箱以登录账号"));
        signUpButton->setText(tr("注册"));
        emailLabel->setText(tr("邮箱"));
        passwordLabel->setText(tr("密码"));
        loginButton->setText(tr("登录"));
        googleButton->setText(tr("使用 Google 登录"));
    });

    addItem(demoCard, loginCard);
}

CardPage::~CardPage()
{
}
