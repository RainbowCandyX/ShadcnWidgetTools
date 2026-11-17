#include "StepsPage.h"

#include <QAbstractButton>
#include <QList>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadSteps.h"

namespace
{
    constexpr int kStepsWidth = 560;
    constexpr int kVerticalStepsWidth = 360;

    struct StepText
    {
        ExampleI18n::TextFunction title;
        ExampleI18n::TextFunction description;
    };

    ShadSteps* createSteps(const QList<StepText>& texts, QWidget* parent)
    {
        ShadSteps* steps = new ShadSteps(parent);
        for (int index = 0; index < int(texts.size()); ++index)
        {
            steps->addStep(QString());
        }
        ExampleI18n::bind(steps, [steps, texts]()
        {
            for (int index = 0; index < int(texts.size()); ++index)
            {
                steps->setStepTitle(index, texts.at(index).title());
                steps->setStepDescription(index, texts.at(index).description ? texts.at(index).description() : QString());
            }
        });
        return steps;
    }

    QList<StepText> signUpSteps(bool hasDescription)
    {
        QList<StepText> texts = {
            {[] { return StepsPage::tr("账号信息"); }, [] { return StepsPage::tr("填写邮箱与密码"); }},
            {[] { return StepsPage::tr("个人资料"); }, [] { return StepsPage::tr("完善姓名与头像"); }},
            {[] { return StepsPage::tr("完成"); }, [] { return StepsPage::tr("开始使用"); }},
        };
        if (!hasDescription)
        {
            for (StepText& text : texts)
            {
                text.description = nullptr;
            }
        }
        return texts;
    }
} // namespace

StepsPage::StepsPage(QWidget* parent)
    : ExampleBasePage([] { return tr("步骤条"); }, [] { return tr("ShadSteps：显示多步骤流程的进度，用于向导与分步表单。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("addStep 添加步骤，next / previous 前进或后退；当前之前的步骤显示为已完成"); });
    ShadSteps* basicSteps = createSteps(signUpSteps(false), this);
    basicSteps->setMaximumWidth(kStepsWidth);
    ShadButton* previousButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadButton* nextButton = new ShadButton(QString(), ShadButtonType::Default, this);
    const auto updateButtons = [basicSteps, previousButton, nextButton]()
    {
        const bool isLast = basicSteps->getCurrentIndex() == basicSteps->count() - 1;
        const bool isFinished = isLast && basicSteps->getStatus() == ShadStepsType::Finish;
        previousButton->setEnabled(basicSteps->getCurrentIndex() > 0);
        nextButton->setEnabled(!isFinished);
        nextButton->setText(isLast ? tr("完成") : tr("下一步"));
        previousButton->setText(tr("上一步"));
    };
    connect(previousButton, &QAbstractButton::clicked, basicSteps, [basicSteps]()
    {
        basicSteps->setStatus(ShadStepsType::Process);
        basicSteps->previous();
    });
    connect(nextButton, &QAbstractButton::clicked, basicSteps, [basicSteps, updateButtons]()
    {
        if (basicSteps->getCurrentIndex() == basicSteps->count() - 1)
        {
            basicSteps->setStatus(ShadStepsType::Finish);
            updateButtons();
            return;
        }
        basicSteps->next();
    });
    connect(basicSteps, &ShadSteps::currentIndexChanged, nextButton, updateButtons);
    ExampleI18n::bind(nextButton, updateButtons);
    addStretchItem(basicCard, basicSteps);
    addRow(basicCard, {previousButton, nextButton});

    ShadCard* verticalCard = addSection([] { return tr("纵向与描述"); }, [] { return tr("setOrientation(Qt::Vertical)：纵向排列，标题与描述自动换行"); });
    ShadSteps* verticalSteps = createSteps(signUpSteps(true), this);
    verticalSteps->setOrientation(Qt::Vertical);
    verticalSteps->setMaximumWidth(kVerticalStepsWidth);
    verticalSteps->setCurrentIndex(1);
    addStretchItem(verticalCard, verticalSteps);

    ShadCard* errorCard = addSection([] { return tr("出错状态"); }, [] { return tr("setStatus(Error)：当前步骤显示为出错，指示器为 X"); });
    const QList<StepText> paymentTexts = {
        {[] { return tr("提交订单"); }, [] { return tr("已确认商品与地址"); }},
        {[] { return tr("支付"); }, [] { return tr("支付失败，请重试"); }},
        {[] { return tr("发货"); }, nullptr},
    };
    ShadSteps* errorSteps = createSteps(paymentTexts, this);
    errorSteps->setMaximumWidth(kStepsWidth);
    errorSteps->setCurrentIndex(1);
    errorSteps->setStatus(ShadStepsType::Error);
    addStretchItem(errorCard, errorSteps);

    ShadCard* clickableCard = addSection([] { return tr("可点击"); }, [] { return tr("setClickable(true)：点击步骤或用方向键与回车切换；setStepDisabled 禁用的步骤不能切换"); });
    const QList<StepText> settingTexts = {
        {[] { return tr("基本设置"); }, nullptr},
        {[] { return tr("权限"); }, nullptr},
        {[] { return tr("通知"); }, nullptr},
        {[] { return tr("高级"); }, nullptr},
    };
    ShadSteps* clickableSteps = createSteps(settingTexts, this);
    clickableSteps->setMaximumWidth(kStepsWidth);
    clickableSteps->setClickable(true);
    clickableSteps->setStepDisabled(3, true);
    ShadLabel* clickableLabel = new ShadLabel(this);
    clickableLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updateClickableLabel = [clickableLabel, clickableSteps]() { clickableLabel->setText(tr("当前步骤: %1").arg(clickableSteps->getStepTitle(clickableSteps->getCurrentIndex()))); };
    connect(clickableSteps, &ShadSteps::currentIndexChanged, clickableLabel, updateClickableLabel);
    ExampleI18n::bind(clickableLabel, updateClickableLabel);
    addStretchItem(clickableCard, clickableSteps);
    addItem(clickableCard, clickableLabel);

    ShadCard* iconCard = addSection([] { return tr("自定义图标"); }, [] { return tr("setStepIcon 用图标代替序号"); });
    const QList<StepText> orderTexts = {
        {[] { return tr("购物车"); }, nullptr},
        {[] { return tr("支付"); }, nullptr},
        {[] { return tr("配送"); }, nullptr},
        {[] { return tr("签收"); }, nullptr},
    };
    ShadSteps* iconSteps = createSteps(orderTexts, this);
    iconSteps->setMaximumWidth(kStepsWidth);
    iconSteps->setStepIcon(0, ShadIconType::ShoppingCart);
    iconSteps->setStepIcon(1, ShadIconType::CreditCard);
    iconSteps->setStepIcon(2, ShadIconType::Truck);
    iconSteps->setStepIcon(3, ShadIconType::PackageCheck);
    iconSteps->setCurrentIndex(2);
    addStretchItem(iconCard, iconSteps);
}

StepsPage::~StepsPage()
{
}
