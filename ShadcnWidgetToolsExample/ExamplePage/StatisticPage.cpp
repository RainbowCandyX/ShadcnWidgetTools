#include "StatisticPage.h"

#include <QDateTime>
#include <QList>
#include <QRandomGenerator>
#include <QString>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadStatistic.h"

namespace
{
    constexpr int kStatisticWidth = 180;
    constexpr int kRestartSeconds = 10;
}

StatisticPage::StatisticPage(QWidget* parent)
    : ExampleBasePage([] { return tr("统计数值"); }, [] { return tr("ShadStatistic：展示关键数值，支持前后缀、趋势、数值滚动动画与倒计时。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("数值变化时从当前值滚动到新值"); });
    ShadStatistic* usersStatistic = new ShadStatistic(this);
    usersStatistic->setTitleIcon(ShadIconType::Users);
    usersStatistic->setValue(112893);
    ExampleI18n::bind(usersStatistic, [usersStatistic]() { usersStatistic->setTitle(tr("活跃用户")); });
    ShadStatistic* balanceStatistic = new ShadStatistic(this);
    balanceStatistic->setPrefix(QStringLiteral("¥"));
    balanceStatistic->setPrecision(2);
    balanceStatistic->setValue(93421.5);
    ExampleI18n::bind(balanceStatistic, [balanceStatistic]() { balanceStatistic->setTitle(tr("账户余额")); });
    ShadStatistic* rateStatistic = new ShadStatistic(this);
    rateStatistic->setSuffix(QStringLiteral("%"));
    rateStatistic->setPrecision(1);
    rateStatistic->setValue(68.4);
    ExampleI18n::bind(rateStatistic, [rateStatistic]() { rateStatistic->setTitle(tr("转化率")); });
    ShadButton* refreshButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    refreshButton->setButtonSize(ShadButtonType::Small);
    ExampleI18n::bind(refreshButton, [refreshButton]() { refreshButton->setText(tr("随机刷新")); });
    connect(refreshButton, &ShadButton::clicked, this, [usersStatistic, balanceStatistic, rateStatistic]()
    {
        usersStatistic->setValue(QRandomGenerator::global()->bounded(1000, 999999));
        balanceStatistic->setValue(QRandomGenerator::global()->bounded(1000, 9999999) / 100.0);
        rateStatistic->setValue(QRandomGenerator::global()->bounded(0, 1000) / 10.0);
    });
    usersStatistic->setMinimumWidth(kStatisticWidth);
    balanceStatistic->setMinimumWidth(kStatisticWidth);
    rateStatistic->setMinimumWidth(kStatisticWidth);
    addRow(basicCard, QList<QWidget*>{usersStatistic, balanceStatistic, rateStatistic});
    addItem(basicCard, refreshButton);

    ShadCard* trendCard = addSection([] { return tr("趋势"); }, [] { return tr("上涨显示绿色向上箭头，下跌显示红色向下箭头"); });
    ShadStatistic* upStatistic = new ShadStatistic(this);
    upStatistic->setTrend(ShadStatisticType::Up);
    upStatistic->setSuffix(QStringLiteral("%"));
    upStatistic->setPrecision(2);
    upStatistic->setValue(11.28);
    ExampleI18n::bind(upStatistic, [upStatistic]() { upStatistic->setTitle(tr("环比增长")); });
    ShadStatistic* downStatistic = new ShadStatistic(this);
    downStatistic->setTrend(ShadStatisticType::Down);
    downStatistic->setSuffix(QStringLiteral("%"));
    downStatistic->setPrecision(2);
    downStatistic->setValue(9.3);
    ExampleI18n::bind(downStatistic, [downStatistic]() { downStatistic->setTitle(tr("环比下降")); });
    upStatistic->setMinimumWidth(kStatisticWidth);
    downStatistic->setMinimumWidth(kStatisticWidth);
    addRow(trendCard, QList<QWidget*>{upStatistic, downStatistic});

    ShadCard* countdownCard = addSection([] { return tr("倒计时"); }, [] { return tr("setCountdown：每秒刷新，到点后显示全 0 并发出 finished 信号"); });
    const QDateTime hoursTarget = QDateTime::currentDateTime().addSecs(2 * 3600);
    ShadStatistic* hoursStatistic = new ShadStatistic(this);
    ExampleI18n::bind(hoursStatistic, [hoursStatistic, hoursTarget]()
    {
        hoursStatistic->setTitle(tr("距离活动开始"));
        hoursStatistic->setCountdown(hoursTarget, QStringLiteral("HH:mm:ss"));
    });
    const QDateTime daysTarget = QDateTime::currentDateTime().addSecs(3 * 86400 + 5 * 3600 + 20 * 60);
    ShadStatistic* daysStatistic = new ShadStatistic(this);
    ExampleI18n::bind(daysStatistic, [daysStatistic, daysTarget]()
    {
        daysStatistic->setTitle(tr("距离活动结束"));
        daysStatistic->setCountdown(daysTarget, tr("D 天 HH:mm:ss"));
    });
    hoursStatistic->setMinimumWidth(kStatisticWidth);
    daysStatistic->setMinimumWidth(kStatisticWidth);
    addRow(countdownCard, QList<QWidget*>{hoursStatistic, daysStatistic});

    ShadStatistic* shortStatistic = new ShadStatistic(this);
    shortStatistic->setMinimumWidth(kStatisticWidth);
    ExampleI18n::bind(shortStatistic, [shortStatistic]() { shortStatistic->setTitle(tr("验证码有效期")); });
    ShadLabel* finishedLabel = new ShadLabel(this);
    finishedLabel->setColorRole(ShadThemeType::MutedForeground);
    ExampleI18n::bind(finishedLabel, [finishedLabel]() { finishedLabel->setText(tr("已结束")); });
    finishedLabel->hide();
    ShadButton* restartButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    restartButton->setButtonSize(ShadButtonType::Small);
    ExampleI18n::bind(restartButton, [restartButton]() { restartButton->setText(tr("重新开始")); });
    const auto restart = [shortStatistic, finishedLabel]()
    {
        finishedLabel->hide();
        shortStatistic->setCountdown(QDateTime::currentDateTime().addSecs(kRestartSeconds), QStringLiteral("mm:ss"));
    };
    connect(restartButton, &ShadButton::clicked, this, restart);
    connect(shortStatistic, &ShadStatistic::finished, finishedLabel, &QWidget::show);
    restart();
    addRow(countdownCard, QList<QWidget*>{shortStatistic, restartButton, finishedLabel});
}

StatisticPage::~StatisticPage()
{
}
