#include "InputOtpPage.h"

#include <QHBoxLayout>
#include <QList>
#include <QSize>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadInputOtp.h"
#include "ShadLabel.h"
#include "ShadToaster.h"

namespace
{
    // 表单卡片 max-w-md
    constexpr int kFormCardWidth = 448;
    // 表单中的大号格子：h-16 w-12 text-xl
    constexpr int kLargeSlotWidth = 48;
    constexpr int kLargeSlotHeight = 64;
    constexpr int kLargeSlotTextSize = 20;
} // namespace

InputOtpPage::InputOtpPage(ShadToaster* toaster, QWidget* parent)
    : ExampleBasePage([] { return tr("验证码输入"); }, [] { return tr("ShadInputOtp：一排相连的格子，每格一个字符，用于输入一次性验证码。"); }, parent)
{
    // 基础用法：两组各 3 格
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("默认两组各 3 格，输入后自动跳到下一格"); });
    addRow(basicCard, {createField([] { return tr("验证码"); }, new ShadInputOtp(this))});

    // 仅数字：一组 6 格
    ShadCard* digitsCard = addSection([] { return tr("仅数字"); }, [] { return tr("只接受数字，一组 6 格"); });
    ShadInputOtp* digitsOtp = new ShadInputOtp(this);
    digitsOtp->setGroups({6});
    digitsOtp->setPattern(ShadInputOtp::onlyDigits());
    addRow(digitsCard, {createField([] { return tr("仅数字"); }, digitsOtp)});

    // 分隔符：三组各 2 格，受控值
    ShadCard* separatorCard = addSection([] { return tr("分隔符"); }, [] { return tr("分为三组，组之间显示分隔符，带初始值"); });
    ShadInputOtp* separatorOtp = new ShadInputOtp(this);
    separatorOtp->setGroups({2, 2, 2});
    separatorOtp->setValue(QStringLiteral("123456"));
    ShadLabel* valueLabel = new ShadLabel(this);
    valueLabel->setColorRole(ShadThemeType::MutedForeground);
    // 按验证码输入的当前值生成文字，值变化与切换语言时都重新计算
    const auto updateValueLabel = [valueLabel, separatorOtp]()
    {
        const QString value = separatorOtp->getValue();
        valueLabel->setText(value.isEmpty() ? tr("当前值：空") : tr("当前值：%1").arg(value));
    };
    connect(separatorOtp, &ShadInputOtp::valueChanged, valueLabel, updateValueLabel);
    ExampleI18n::bind(valueLabel, updateValueLabel);
    addRow(separatorCard, {createField([] { return tr("分隔符"); }, separatorOtp), valueLabel});

    // 字母与数字
    ShadCard* alphanumericCard = addSection([] { return tr("字母与数字"); }, [] { return tr("接受字母与数字，字母自动转为大写"); });
    ShadInputOtp* alphanumericOtp = new ShadInputOtp(this);
    alphanumericOtp->setPattern(ShadInputOtp::onlyDigitsAndChars());
    addRow(alphanumericCard, {createField([] { return tr("字母与数字"); }, alphanumericOtp, [] { return tr("可以输入字母和数字。"); })});

    // 禁用
    ShadCard* disabledCard = addSection([] { return tr("禁用"); }, [] { return tr("禁用时整体半透明，鼠标显示禁止图标"); });
    ShadInputOtp* disabledOtp = new ShadInputOtp(this);
    disabledOtp->setValue(QStringLiteral("123456"));
    disabledOtp->setEnabled(false);
    addRow(disabledCard, {createField([] { return tr("禁用"); }, disabledOtp)});

    // 4 位 PIN
    ShadCard* pinCard = addSection([] { return tr("4 位 PIN"); }, [] { return tr("4 格仅数字，填满后发出 completed 信号"); });
    ShadInputOtp* pinOtp = new ShadInputOtp(this);
    pinOtp->setGroups({4});
    pinOtp->setPattern(ShadInputOtp::onlyDigits());
    connect(pinOtp, &ShadInputOtp::completed, this, [toaster](const QString& value) { toaster->add(tr("PIN 已输入"), tr("输入的 PIN 为 %1").arg(value), ShadToastType::Success); });
    addRow(pinCard, {createField([] { return tr("4 位 PIN"); }, pinOtp, [] { return tr("常用于 PIN 码。"); })});

    // 错误态
    ShadCard* invalidCard = addSection([] { return tr("错误态"); }, [] { return tr("setInvalid(true)：显示错误色，修改内容后取消"); });
    ShadInputOtp* invalidOtp = new ShadInputOtp(this);
    invalidOtp->setGroups({2, 2, 2});
    invalidOtp->setValue(QStringLiteral("000000"));
    invalidOtp->setInvalid(true);
    connect(invalidOtp, &ShadInputOtp::valueChanged, invalidOtp, [invalidOtp]() { invalidOtp->setInvalid(false); });
    addRow(invalidCard, {createField([] { return tr("错误态"); }, invalidOtp, [] { return tr("验证码错误，请重试。"); }, true)});

    // 表单：验证登录
    ShadCard* formSection = addSection([] { return tr("表单"); }, [] { return tr("使用大号格子的登录验证卡片，验证时检查是否填满"); });
    ShadCard* formCard = new ShadCard(this);
    formCard->setFixedWidth(kFormCardWidth);
    QWidget* labelRow = new QWidget(formCard);
    QHBoxLayout* labelRowLayout = new QHBoxLayout(labelRow);
    labelRowLayout->setContentsMargins(0, 0, 0, 0);
    ShadLabel* codeLabel = new ShadLabel(labelRow);
    codeLabel->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, 0, 0);
    ShadButton* resendButton = new ShadButton(QString(), ShadButtonType::Outline, labelRow);
    resendButton->setButtonSize(ShadButtonType::ExtraSmall);
    resendButton->setShadIcon(ShadIconType::RefreshCw);
    labelRowLayout->addWidget(codeLabel);
    labelRowLayout->addStretch(1);
    labelRowLayout->addWidget(resendButton);
    ShadInputOtp* formOtp = new ShadInputOtp(formCard);
    formOtp->setSlotSize(QSize(kLargeSlotWidth, kLargeSlotHeight));
    formOtp->setSlotTextPixelSize(kLargeSlotTextSize);
    codeLabel->setBuddy(formOtp);
    ShadLabel* accessLabel = new ShadLabel(formCard);
    accessLabel->setColorRole(ShadThemeType::MutedForeground);
    accessLabel->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, 0, 0);
    QVBoxLayout* contentLayout = formCard->contentLayout();
    contentLayout->setSpacing(12 - ExampleLayout::kFocusRingWidth);
    contentLayout->addWidget(labelRow);
    contentLayout->addWidget(formOtp, 0, Qt::AlignLeft);
    contentLayout->addWidget(accessLabel);
    QHBoxLayout* footerLayout = formCard->footerLayout();
    QWidget* footerColumn = new QWidget(formCard);
    QVBoxLayout* footerColumnLayout = new QVBoxLayout(footerColumn);
    footerColumnLayout->setContentsMargins(0, 0, 0, 0);
    footerColumnLayout->setSpacing(8);
    ShadButton* verifyButton = new ShadButton(footerColumn);
    ShadLabel* supportLabel = new ShadLabel(footerColumn);
    supportLabel->setColorRole(ShadThemeType::MutedForeground);
    supportLabel->setAlignment(Qt::AlignCenter);
    footerColumnLayout->addWidget(verifyButton);
    footerColumnLayout->addWidget(supportLabel);
    footerLayout->addWidget(footerColumn);
    ExampleI18n::bind(formCard, [formCard, codeLabel, resendButton, accessLabel, verifyButton, supportLabel]()
    {
        formCard->setTitle(tr("验证登录"));
        formCard->setDescription(tr("请输入发送到 m@example.com 的验证码。"));
        codeLabel->setText(tr("验证码"));
        resendButton->setText(tr("重新发送"));
        accessLabel->setText(tr("无法访问这个邮箱？"));
        verifyButton->setText(tr("验证"));
        supportLabel->setText(tr("登录遇到问题？请联系客服。"));
    });
    connect(resendButton, &ShadButton::clicked, this, [toaster, formOtp]()
    {
        formOtp->clear();
        formOtp->setInvalid(false);
        toaster->add(tr("验证码已重新发送"), tr("请查看 m@example.com 的收件箱。"));
    });
    connect(verifyButton, &ShadButton::clicked, this, [toaster, formOtp]()
    {
        if (formOtp->getValue().size() < formOtp->getMaxLength())
        {
            formOtp->setInvalid(true);
            toaster->add(tr("验证码不完整"), tr("请输入 6 位验证码。"), ShadToastType::Error);
            return;
        }
        formOtp->setInvalid(false);
        toaster->add(tr("验证成功"), tr("验证码 %1 已通过验证。").arg(formOtp->getValue()), ShadToastType::Success);
    });
    connect(formOtp, &ShadInputOtp::valueChanged, formOtp, [formOtp]() { formOtp->setInvalid(false); });
    addItem(formSection, formCard);
}

InputOtpPage::~InputOtpPage()
{
}

QWidget* InputOtpPage::createField(const ExampleI18n::TextFunction& labelText, QWidget* field, const ExampleI18n::TextFunction& description, bool isError)
{
    QWidget* container = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    // gap-3 扣除验证码输入上下 3px 焦点环边距
    layout->setSpacing(12 - ExampleLayout::kFocusRingWidth);
    ShadLabel* label = new ShadLabel(container);
    label->setBuddy(field);
    label->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, ExampleLayout::kFocusRingWidth, 0);
    ExampleI18n::bind(label, [label, labelText]() { label->setText(labelText()); });
    layout->addWidget(label);
    layout->addWidget(field, 0, Qt::AlignLeft);
    if (description)
    {
        ShadLabel* descriptionLabel = new ShadLabel(container);
        descriptionLabel->setColorRole(isError ? ShadThemeType::Destructive : ShadThemeType::MutedForeground);
        descriptionLabel->setContentsMargins(ExampleLayout::kFocusRingWidth, 0, ExampleLayout::kFocusRingWidth, 0);
        ExampleI18n::bind(descriptionLabel, [descriptionLabel, description]() { descriptionLabel->setText(description()); });
        layout->addWidget(descriptionLabel);
    }
    return container;
}
