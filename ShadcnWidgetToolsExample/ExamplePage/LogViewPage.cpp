#include "LogViewPage.h"

#include <QAbstractButton>
#include <QLineEdit>
#include <QString>
#include <QTimer>
#include <QWidget>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadLogView.h"
#include "ShadSegmented.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kLogHeight = 240;
    constexpr int kStreamIntervalMs = 200;
    constexpr int kStreamMaxEntryCount = 500;
    constexpr int kFilterEntryCount = 40;
    constexpr int kStreamMessageCount = 6;

    ShadLogView* createLogView(QWidget* parent)
    {
        ShadLogView* logView = new ShadLogView(parent);
        logView->setFixedHeight(kLogHeight);
        ExampleI18n::bind(logView, [logView]() { logView->setPlaceholderText(LogViewPage::tr("暂无日志")); });
        return logView;
    }

    void appendStreamEntry(ShadLogView* logView, int index)
    {
        switch (index % kStreamMessageCount)
        {
            case 0:
                logView->info(LogViewPage::tr("收到请求 GET /api/items/%1").arg(index));
                break;
            case 1:
                logView->debug(LogViewPage::tr("查询耗时 %1 ms").arg(12 + index % 30));
                break;
            case 2:
                logView->debug(LogViewPage::tr("缓存命中率 %1%").arg(60 + index % 35));
                break;
            case 3:
                logView->success(LogViewPage::tr("任务 %1 已完成").arg(index));
                break;
            case 4:
                logView->warning(LogViewPage::tr("响应较慢：%1 ms，超过阈值").arg(800 + index % 400));
                break;
            default:
                logView->error(LogViewPage::tr("请求 %1 失败：连接超时").arg(index));
                break;
        }
    }

    ShadButton* createOutlineButton(QWidget* parent)
    {
        ShadButton* button = new ShadButton(parent);
        button->setVariant(ShadButtonType::Outline);
        return button;
    }
} // namespace

LogViewPage::LogViewPage(QWidget* parent)
    : ExampleBasePage([] { return tr("日志视图"); }, [] { return tr("ShadLogView：按级别着色的日志输出，可按级别与关键字过滤，停在末尾时自动跟随新日志。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("debug、info、success、warning、error 追加一条日志；右键可复制、全选与清空"); });
    ShadLogView* basicView = createLogView(this);
    basicView->info(tr("应用已启动"));
    basicView->debug(tr("读取配置文件 config.json"));
    basicView->success(tr("已连接到服务器"));
    basicView->warning(tr("磁盘剩余空间不足 10%"));
    basicView->error(tr("无法打开文件：权限不足"));
    ShadButton* debugButton = createOutlineButton(this);
    debugButton->setText(QStringLiteral("Debug"));
    ShadButton* infoButton = createOutlineButton(this);
    infoButton->setText(QStringLiteral("Info"));
    ShadButton* successButton = createOutlineButton(this);
    successButton->setText(QStringLiteral("Success"));
    ShadButton* warningButton = createOutlineButton(this);
    warningButton->setText(QStringLiteral("Warning"));
    ShadButton* errorButton = createOutlineButton(this);
    errorButton->setText(QStringLiteral("Error"));
    connect(debugButton, &QAbstractButton::clicked, basicView, [basicView]() { basicView->debug(tr("这是一条 %1 日志").arg(QStringLiteral("Debug"))); });
    connect(infoButton, &QAbstractButton::clicked, basicView, [basicView]() { basicView->info(tr("这是一条 %1 日志").arg(QStringLiteral("Info"))); });
    connect(successButton, &QAbstractButton::clicked, basicView, [basicView]() { basicView->success(tr("这是一条 %1 日志").arg(QStringLiteral("Success"))); });
    connect(warningButton, &QAbstractButton::clicked, basicView, [basicView]() { basicView->warning(tr("这是一条 %1 日志").arg(QStringLiteral("Warning"))); });
    connect(errorButton, &QAbstractButton::clicked, basicView, [basicView]() { basicView->error(tr("这是一条 %1 日志").arg(QStringLiteral("Error"))); });
    addRow(basicCard, {debugButton, infoButton, successButton, warningButton, errorButton});
    addStretchItem(basicCard, basicView);

    ShadCard* filterCard = addSection([] { return tr("过滤"); }, [] { return tr("setMinimumLevel 按级别过滤，setFilterText 按关键字过滤，忽略大小写"); });
    ShadLogView* filterView = createLogView(this);
    for (int index = 1; index <= kFilterEntryCount; ++index)
    {
        appendStreamEntry(filterView, index);
    }
    ShadSegmented* levelSegmented = new ShadSegmented(this);
    levelSegmented->addItem(QString());
    levelSegmented->addItem(QStringLiteral("Info"));
    levelSegmented->addItem(QStringLiteral("Warning"));
    levelSegmented->addItem(QStringLiteral("Error"));
    ExampleI18n::bind(levelSegmented, [levelSegmented]() { levelSegmented->setItemText(0, tr("全部")); });
    ShadInput* filterInput = new ShadInput(this);
    filterInput->setFixedWidth(ExampleLayout::kFieldWidth);
    filterInput->setLeadingIcon(ShadIconType::Search);
    filterInput->setTrailingAction(ShadInputType::ClearAction);
    ExampleI18n::bind(filterInput, [filterInput]() { filterInput->setPlaceholderText(tr("按关键字过滤")); });
    ShadLabel* filterLabel = new ShadLabel(this);
    filterLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updateFilterLabel = [filterView, filterLabel]() { filterLabel->setText(tr("显示 %1 / %2 条").arg(filterView->visibleCount()).arg(filterView->count())); };
    ExampleI18n::bind(filterLabel, updateFilterLabel);
    connect(levelSegmented, &ShadSegmented::currentIndexChanged, filterView, [filterView, updateFilterLabel](int index)
    {
        const ShadLogViewType::Level levels[] = {ShadLogViewType::Debug, ShadLogViewType::Info, ShadLogViewType::Warning, ShadLogViewType::Error};
        filterView->setMinimumLevel(levels[index]);
        updateFilterLabel();
    });
    connect(filterInput, &QLineEdit::textChanged, filterView, [filterView, updateFilterLabel](const QString& text)
    {
        filterView->setFilterText(text);
        updateFilterLabel();
    });
    addRow(filterCard, {levelSegmented, filterInput});
    addStretchItem(filterCard, filterView);
    addItem(filterCard, filterLabel);

    ShadCard* streamCard = addSection([] { return tr("实时输出"); }, [] { return tr("每 200ms 追加一条日志；setMaxEntryCount(500) 超出后丢弃最早的日志，向上滚动后暂停跟随"); });
    ShadLogView* streamView = createLogView(this);
    streamView->setMaxEntryCount(kStreamMaxEntryCount);
    QTimer* streamTimer = new QTimer(streamView);
    streamTimer->setInterval(kStreamIntervalMs);
    connect(streamTimer, &QTimer::timeout, streamView, [streamView, index = 0]() mutable { appendStreamEntry(streamView, ++index); });
    ShadButton* toggleButton = new ShadButton(this);
    const auto updateToggleButton = [streamTimer, toggleButton]() { toggleButton->setText(streamTimer->isActive() ? tr("停止") : tr("开始")); };
    ExampleI18n::bind(toggleButton, updateToggleButton);
    connect(toggleButton, &QAbstractButton::clicked, streamTimer, [streamTimer, updateToggleButton]()
    {
        if (streamTimer->isActive())
        {
            streamTimer->stop();
        }
        else
        {
            streamTimer->start();
        }
        updateToggleButton();
    });
    ShadButton* clearButton = createOutlineButton(this);
    ExampleI18n::bind(clearButton, [clearButton]() { clearButton->setText(tr("清空")); });
    connect(clearButton, &QAbstractButton::clicked, streamView, &ShadLogView::clear);
    ShadSwitch* autoScrollSwitch = new ShadSwitch(this);
    autoScrollSwitch->setChecked(true);
    ExampleI18n::bind(autoScrollSwitch, [autoScrollSwitch]() { autoScrollSwitch->setText(tr("自动滚动")); });
    connect(autoScrollSwitch, &ShadSwitch::toggled, streamView, &ShadLogView::setAutoScroll);
    ShadSwitch* wordWrapSwitch = new ShadSwitch(this);
    ExampleI18n::bind(wordWrapSwitch, [wordWrapSwitch]() { wordWrapSwitch->setText(tr("自动换行")); });
    connect(wordWrapSwitch, &ShadSwitch::toggled, streamView, &ShadLogView::setWordWrap);
    ShadLabel* countLabel = new ShadLabel(this);
    countLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updateCountLabel = [streamView, countLabel]() { countLabel->setText(tr("共 %1 条").arg(streamView->count())); };
    ExampleI18n::bind(countLabel, updateCountLabel);
    connect(streamView, &ShadLogView::countChanged, countLabel, updateCountLabel);
    addRow(streamCard, {toggleButton, clearButton, autoScrollSwitch, wordWrapSwitch});
    addStretchItem(streamCard, streamView);
    addItem(streamCard, countLabel);
}

LogViewPage::~LogViewPage()
{
}
