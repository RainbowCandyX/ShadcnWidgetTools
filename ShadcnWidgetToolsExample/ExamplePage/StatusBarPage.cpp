#include "StatusBarPage.h"

#include <QAbstractButton>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ExampleI18n.h"
#include "ShadBadge.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadProgress.h"
#include "ShadSpinner.h"
#include "ShadStatusBar.h"
#include "ShadSwitch.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kFrameHeight = 120;
    constexpr int kProgressWidth = 96;
    constexpr int kSpinnerSize = 12;
    constexpr int kMessageTimeout = 3000;

    class StatusBarFrame : public QWidget
    {
        public:
            StatusBarFrame(ShadStatusBar* statusBar, QWidget* parent)
                : QWidget(parent)
            {
                QVBoxLayout* layout = new QVBoxLayout(this);
                layout->setContentsMargins(1, 1, 1, 1);
                layout->setSpacing(0);
                layout->addStretch(1);
                layout->addWidget(statusBar);
                setMinimumHeight(kFrameHeight);
                connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
            }

        protected:
            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setPen(QPen(shadTheme->getColor(ShadThemeType::Border), 1.0));
                painter.setBrush(Qt::NoBrush);
                const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
                painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
            }
    };

    ShadBadge* createBadge(ShadBadgeType::Variant variant, QWidget* parent)
    {
        ShadBadge* badge = new ShadBadge(parent);
        badge->setVariant(variant);
        return badge;
    }

    ShadButton* createButton(QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Outline, parent);
        button->setButtonSize(ShadButtonType::Small);
        return button;
    }
} // namespace

StatusBarPage::StatusBarPage(QWidget* parent)
    : ExampleBasePage([] { return tr("状态栏"); }, [] { return tr("ShadStatusBar：窗口底部的状态信息。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("开始侧显示状态，结束侧显示常驻信息"); });
    ShadStatusBar* basicBar = new ShadStatusBar(this);
    ShadLabel* readyLabel = basicBar->addText(QString());
    ShadLabel* positionLabel = basicBar->addText(QString());
    ShadBadge* networkBadge = createBadge(ShadBadgeType::Secondary, basicBar);
    networkBadge->setShadIcon(ShadIconType::Wifi);
    basicBar->addPermanentWidget(networkBadge);
    ShadBadge* encodingBadge = createBadge(ShadBadgeType::Outline, basicBar);
    encodingBadge->setText(QStringLiteral("UTF-8"));
    basicBar->addPermanentWidget(encodingBadge);
    ExampleI18n::bind(basicBar, [readyLabel, positionLabel, networkBadge]()
    {
        readyLabel->setText(tr("就绪"));
        positionLabel->setText(tr("第 %1 行，第 %2 列").arg(12).arg(4));
        networkBadge->setText(tr("已连接"));
    });
    addStretchItem(basicCard, new StatusBarFrame(basicBar, this));
    ShadSwitch* separatorSwitch = new ShadSwitch(this);
    separatorSwitch->setChecked(true);
    ExampleI18n::bind(separatorSwitch, [separatorSwitch]() { separatorSwitch->setText(tr("显示分隔线")); });
    connect(separatorSwitch, &QAbstractButton::toggled, basicBar, &ShadStatusBar::setSeparatorVisible);
    addRow(basicCard, {separatorSwitch});

    ShadCard* messageCard = addSection([] { return tr("临时消息"); }, [] { return tr("消息显示期间隐藏开始侧内容"); });
    ShadStatusBar* messageBar = new ShadStatusBar(this);
    ShadLabel* fileCountLabel = messageBar->addText(QString());
    ShadBadge* lineEndingBadge = createBadge(ShadBadgeType::Outline, messageBar);
    lineEndingBadge->setText(QStringLiteral("LF"));
    messageBar->addPermanentWidget(lineEndingBadge);
    ExampleI18n::bind(messageBar, [fileCountLabel]() { fileCountLabel->setText(tr("共 %1 个文件").arg(3)); });
    addStretchItem(messageCard, new StatusBarFrame(messageBar, this));
    ShadButton* timedButton = createButton(this);
    ShadButton* stickyButton = createButton(this);
    ShadButton* clearButton = createButton(this);
    ExampleI18n::bind(timedButton, [timedButton, stickyButton, clearButton]()
    {
        timedButton->setText(tr("显示 3 秒"));
        stickyButton->setText(tr("一直显示"));
        clearButton->setText(tr("清除"));
    });
    connect(timedButton, &QAbstractButton::clicked, messageBar, [messageBar]() { messageBar->showMessage(tr("已保存"), kMessageTimeout); });
    connect(stickyButton, &QAbstractButton::clicked, messageBar, [messageBar]() { messageBar->showMessage(tr("正在同步…")); });
    connect(clearButton, &QAbstractButton::clicked, messageBar, &ShadStatusBar::clearMessage);
    addRow(messageCard, {timedButton, stickyButton, clearButton});

    ShadCard* progressCard = addSection([] { return tr("进度"); }, [] { return tr("结束侧放置进度与加载状态"); });
    ShadStatusBar* progressBar = new ShadStatusBar(this);
    ShadLabel* indexLabel = progressBar->addText(QString());
    ShadSpinner* spinner = new ShadSpinner(progressBar);
    spinner->setSpinnerSize(kSpinnerSize);
    spinner->setColorRole(ShadThemeType::MutedForeground);
    progressBar->addPermanentWidget(spinner);
    ShadProgress* progress = new ShadProgress(progressBar);
    progress->setFixedWidth(kProgressWidth);
    progress->setValue(60);
    progressBar->addPermanentWidget(progress);
    ExampleI18n::bind(progressBar, [indexLabel]() { indexLabel->setText(tr("正在索引…")); });
    addStretchItem(progressCard, new StatusBarFrame(progressBar, this));
}

StatusBarPage::~StatusBarPage()
{
}
