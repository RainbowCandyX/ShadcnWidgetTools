#include "AlertPage.h"

#include <QColor>
#include <QSizePolicy>
#include <QString>
#include <QWidget>

#include "ShadAlert.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"

namespace
{
    ShadAlert* createAlert(ShadIconType::IconName icon, const QString& title, const QString& description, ShadAlertType::Variant variant, QWidget* parent)
    {
        ShadAlert* alert = new ShadAlert(icon, title, description, variant, parent);
        alert->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        return alert;
    }
} // namespace

AlertPage::AlertPage(QWidget* parent)
    : ExampleBasePage([] { return tr("提示"); }, [] { return tr("ShadAlert：带图标、标题与描述的提示，右上角可放操作组件。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("图标可选，图标与标题同色"); });
    addStretchItem(basicCard, createAlert(ShadIconType::CircleCheck, QStringLiteral("Payment successful"), QStringLiteral("Your payment of $29.99 has been processed. A receipt has been sent to your email address."), ShadAlertType::Default, this));
    addStretchItem(basicCard, createAlert(ShadIconType::Info, QStringLiteral("New feature available"), QStringLiteral("We've added dark mode support. You can enable it in your account settings."), ShadAlertType::Default, this));

    ShadCard* destructiveCard = addSection([] { return tr("危险样式"); }, [] { return tr("标题、图标与描述使用危险色"); });
    addStretchItem(destructiveCard, createAlert(ShadIconType::CircleAlert, QStringLiteral("Payment failed"), QStringLiteral("Your payment could not be processed. Please check your payment method and try again."), ShadAlertType::Destructive, this));

    ShadCard* actionCard = addSection([] { return tr("操作"); }, [] { return tr("setAction：在右上角放置操作组件"); });
    ShadAlert* actionAlert = createAlert(ShadIconType::None, QStringLiteral("Dark mode is now available"), QStringLiteral("Enable it under your profile settings to get started."), ShadAlertType::Default, this);
    ShadButton* enableButton = new ShadButton(QStringLiteral("Enable"), ShadButtonType::Default);
    enableButton->setButtonSize(ShadButtonType::Small);
    actionAlert->setAction(enableButton);
    addStretchItem(actionCard, actionAlert);

    ShadCard* colorCard = addSection([] { return tr("自定义颜色"); }, [] { return tr("setCustomColors：按亮色与暗色主题分别设置颜色"); });
    ShadAlert* colorAlert = createAlert(ShadIconType::TriangleAlert, QStringLiteral("Your subscription will expire in 3 days."), QStringLiteral("Renew now to avoid service interruption or upgrade to a paid plan to continue using the service."), ShadAlertType::Default, this);
    // border-amber-200 bg-amber-50 text-amber-900 dark:border-amber-900 dark:bg-amber-950 dark:text-amber-50
    colorAlert->setCustomColors(ShadThemeType::Light, QColor(0xFF, 0xFB, 0xEB), QColor(0xFE, 0xE6, 0x85), QColor(0x7B, 0x33, 0x06));
    colorAlert->setCustomColors(ShadThemeType::Dark, QColor(0x46, 0x19, 0x01), QColor(0x7B, 0x33, 0x06), QColor(0xFF, 0xFB, 0xEB));
    addStretchItem(colorCard, colorAlert);
}

AlertPage::~AlertPage()
{
}
