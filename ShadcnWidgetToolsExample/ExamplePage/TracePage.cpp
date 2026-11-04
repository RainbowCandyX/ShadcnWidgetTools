#include "TracePage.h"

#include <QChar>
#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadCodeBlock.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadPopover.h"
#include "ShadTrace.h"

namespace
{
    // 浮层中显示 diff 与搜索结果的宽度
    constexpr int kPopoverWidth = 420;
    // 输出代码块 max-h-64
    constexpr int kOutputMaxHeight = 256;
    // 模拟运行时每行输出与每个思考步骤的间隔
    constexpr int kBuildIntervalMs = 700;
    constexpr int kThoughtIntervalMs = 1200;

    ShadTrace* createTrace(QWidget* parent)
    {
        return new ShadTrace(parent);
    }

    // 标题随语言刷新的行
    ShadTraceLine* addTraceLine(ShadTrace* trace, ShadIconType::IconName icon, const ExampleI18n::TextFunction& title)
    {
        ShadTraceLine* line = trace->addLine(icon, title());
        ExampleI18n::bind(line, [line, title]() { line->setTitle(title()); });
        return line;
    }

    // 触发行标题随语言刷新的可展开行
    ShadTraceCollapsible* addTraceCollapsible(ShadTrace* trace, ShadIconType::IconName icon, const ExampleI18n::TextFunction& title)
    {
        ShadTraceCollapsible* row = trace->addCollapsible(icon, title());
        ShadTraceLine* line = row->getTriggerLine();
        ExampleI18n::bind(line, [line, title]() { line->setTitle(title()); });
        return row;
    }

    // 文字随语言刷新的说明
    void addTraceText(ShadTraceCollapsible* row, const ExampleI18n::TextFunction& text)
    {
        ShadLabel* label = row->addText(text());
        ExampleI18n::bind(label, [label, text]() { label->setText(text()); });
    }

    // 详情区中的命令输出：size="sm"、自动换行、max-h-64
    ShadCodeBlock* createOutput(const QString& code, ShadCodeBlockType::Variant variant, QWidget* parent)
    {
        ShadCodeBlock* block = new ShadCodeBlock(code, parent);
        block->setCodeBlockSize(ShadCodeBlockType::Small);
        block->setVariant(variant);
        block->setWordWrap(true);
        block->setMaximumContentHeight(kOutputMaxHeight);
        return block;
    }

    // 来源与链接行：标题后显示 ArrowUpRight，点击后用浏览器打开
    ShadTraceLine* addLinkLine(ShadTrace* trace, const QString& title, const QString& url)
    {
        ShadTraceLine* line = trace->addLine(ShadIconType::Globe, title);
        line->setIndicatorIcon(ShadIconType::ArrowUpRight);
        line->setUrl(QUrl(url));
        ExampleI18n::bind(line, [line, url]() { line->setToolTip(QCoreApplication::translate("TracePage", "在浏览器中打开 %1").arg(url)); });
        return line;
    }
} // namespace

TracePage::TracePage(QWidget* parent)
    : ExampleBasePage([] { return tr("执行记录"); }, [] { return tr("ShadTrace / ShadTraceLine / ShadTraceCollapsible：按行显示 agent 的执行过程，每行可展开详情或打开链接。"); }, parent)
{
    // 基础用法：一行文字与一个可展开的行
    ShadCard* usageCard = addSection([] { return tr("基础用法"); }, [] { return tr("添加普通行与可展开的行"); });
    ShadTrace* usageTrace = createTrace(this);
    addTraceLine(usageTrace, ShadIconType::Search, [] { return tr("已搜索 frozen-lockfile"); });
    ShadTraceCollapsible* versionRow = addTraceCollapsible(usageTrace, ShadIconType::Terminal, [] { return tr("已运行 node --version"); });
    versionRow->contentLayout()->addWidget(createOutput(QStringLiteral("$ node --version\nv24.18.0"), ShadCodeBlockType::Default, versionRow));
    addStretchItem(usageCard, usageTrace);

    // 单行：没有详情，只是文字
    ShadCard* lineCard = addSection([] { return tr("单行"); }, [] { return tr("默认不可点击，只作为文字显示"); });
    ShadTrace* lineTrace = createTrace(this);
    addTraceLine(lineTrace, ShadIconType::GitBranch, [] { return tr("已创建分支 fix/frozen-lockfile"); });
    addStretchItem(lineCard, lineTrace);

    // 状态：完成、失败、运行中、未开始
    ShadCard* statusCard = addSection([] { return tr("状态"); }, [] { return tr("用颜色与动画区分状态，标题也要写明状态"); });
    ShadTrace* statusTrace = createTrace(this);
    addTraceLine(statusTrace, ShadIconType::CircleCheck, [] { return tr("已运行 pnpm install"); });
    addTraceLine(statusTrace, ShadIconType::CircleAlert, [] { return tr("运行 pnpm lint 失败"); })->setStatus(ShadTraceType::Error);
    addTraceLine(statusTrace, ShadIconType::LoaderCircle, [] { return tr("正在运行 pnpm build"); })->setStatus(ShadTraceType::Running);
    addTraceLine(statusTrace, ShadIconType::Circle, [] { return tr("运行 pnpm test"); })->setStatus(ShadTraceType::Pending);
    addStretchItem(statusCard, statusTrace);

    // 描述：改动统计、耗时与结果数；标题过长时省略，描述保持完整
    ShadCard* descriptionCard = addSection([] { return tr("描述"); }, [] { return tr("描述显示数量、耗时或改动统计"); });
    ShadTrace* descriptionTrace = createTrace(this);
    ShadTraceLine* editLine = addTraceLine(descriptionTrace, ShadIconType::FilePen, [] { return tr("已编辑 components/settings/notification-preferences-form.tsx"); });
    editLine->setDescription(QStringLiteral("+48 −12"));
    editLine->setDescriptionMonospace(true);
    addTraceLine(descriptionTrace, ShadIconType::Terminal, [] { return tr("已运行 pnpm test"); })->setDescription(QStringLiteral("14s"));
    ShadTraceLine* useChatLine = addTraceLine(descriptionTrace, ShadIconType::Search, [] { return tr("已搜索 useChat"); });
    ExampleI18n::bind(useChatLine, [useChatLine]() { useChatLine->setDescription(tr("8 个结果")); });
    addStretchItem(descriptionCard, descriptionTrace);

    // 可展开：详情区放命令输出
    ShadCard* collapsibleCard = addSection([] { return tr("可展开"); }, [] { return tr("触发行带箭头，展开后显示详情区"); });
    ShadTrace* collapsibleTrace = createTrace(this);
    ShadTraceCollapsible* nodeRow = addTraceCollapsible(collapsibleTrace, ShadIconType::Terminal, [] { return tr("已运行 node --version"); });
    nodeRow->contentLayout()->addWidget(createOutput(QStringLiteral("$ node --version\nv24.18.0"), ShadCodeBlockType::Default, nodeRow));
    addStretchItem(collapsibleCard, collapsibleTrace);

    // 展开状态：默认展开、外部控制、运行时自动展开
    ShadCard* openCard = addSection([] { return tr("展开状态"); }, [] { return tr("默认展开或外部控制，运行中的行自动展开"); });
    ShadTrace* openTrace = createTrace(this);
    ShadTraceCollapsible* openNodeRow = addTraceCollapsible(openTrace, ShadIconType::Terminal, [] { return tr("已运行 node --version"); });
    openNodeRow->contentLayout()->addWidget(createOutput(QStringLiteral("$ node --version\nv24.18.0"), ShadCodeBlockType::Default, openNodeRow));
    openNodeRow->setExpanded(true);
    ShadTraceCollapsible* pnpmRow = addTraceCollapsible(openTrace, ShadIconType::Terminal, [] { return tr("已运行 pnpm --version"); });
    pnpmRow->contentLayout()->addWidget(createOutput(QStringLiteral("$ pnpm --version\n10.33.4"), ShadCodeBlockType::Default, pnpmRow));
    ShadButton* pnpmButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    pnpmButton->setButtonSize(ShadButtonType::Small);
    const auto updatePnpmButton = [pnpmButton, pnpmRow]() { pnpmButton->setText(pnpmRow->isExpanded() ? tr("隐藏 pnpm 输出") : tr("显示 pnpm 输出")); };
    connect(pnpmButton, &ShadButton::clicked, pnpmRow, [pnpmRow]() { pnpmRow->setExpanded(!pnpmRow->isExpanded()); });
    connect(pnpmRow, &ShadTraceCollapsible::expandedChanged, pnpmButton, updatePnpmButton);
    ExampleI18n::bind(pnpmButton, updatePnpmButton);

    // 模拟一次构建：运行中展开并逐行输出，结束后折叠；用户点击过触发行则保持用户的选择
    struct BuildRun
    {
        int step{0};
        bool isUserChoice{false};
    };
    std::shared_ptr<BuildRun> buildRun = std::make_shared<BuildRun>();
    ShadTraceCollapsible* buildRow = openTrace->addCollapsible(ShadIconType::Terminal, QString());
    ShadCodeBlock* buildOutput = createOutput(QString(), ShadCodeBlockType::Default, buildRow);
    buildRow->contentLayout()->addWidget(buildOutput);
    const QStringList buildLines{QStringLiteral("$ pnpm build"), QStringLiteral("> next build"), QStringLiteral("   Creating an optimized production build ..."), QStringLiteral(" ✓ Compiled successfully in 4.2s"), QStringLiteral(" ✓ Generating static pages (12/12)")};
    QTimer* buildTimer = new QTimer(buildRow);
    buildTimer->setInterval(kBuildIntervalMs);
    // 计时器运行中即构建运行中，标题按当前状态生成
    const auto updateBuildTitle = [buildRow, buildTimer]() { buildRow->getTriggerLine()->setTitle(buildTimer->isActive() ? tr("正在运行 pnpm build") : tr("已运行 pnpm build")); };
    connect(buildRow->getTriggerLine(), &ShadTraceLine::clicked, buildRow, [buildRun]() { buildRun->isUserChoice = true; });
    connect(buildTimer, &QTimer::timeout, buildRow, [buildRow, buildOutput, buildTimer, buildRun, buildLines, updateBuildTitle]()
    {
        if (buildRun->step < buildLines.size())
        {
            buildOutput->appendCode(buildLines.at(buildRun->step) + u'\n');
            ++buildRun->step;
            return;
        }
        buildTimer->stop();
        ShadTraceLine* line = buildRow->getTriggerLine();
        line->setStatus(ShadTraceType::Done);
        updateBuildTitle();
        line->setDescription(QStringLiteral("4s"));
        if (!buildRun->isUserChoice)
        {
            buildRow->setExpanded(false);
        }
    });
    const auto startBuild = [buildRow, buildOutput, buildTimer, buildRun, updateBuildTitle]()
    {
        buildRun->step = 0;
        buildRun->isUserChoice = false;
        buildOutput->setCode(QString());
        ShadTraceLine* line = buildRow->getTriggerLine();
        line->setStatus(ShadTraceType::Running);
        line->setDescription(QString());
        buildRow->setExpanded(true);
        buildTimer->start();
        updateBuildTitle();
    };
    ShadButton* rerunButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    rerunButton->setButtonSize(ShadButtonType::Small);
    rerunButton->setShadIcon(ShadIconType::RotateCcw);
    ExampleI18n::bind(rerunButton, [rerunButton]() { rerunButton->setText(tr("重新运行构建")); });
    connect(rerunButton, &ShadButton::clicked, buildRow, startBuild);
    startBuild();
    ExampleI18n::bind(buildRow, updateBuildTitle);
    addStretchItem(openCard, openTrace);
    addRow(openCard, {pnpmButton, rerunButton});

    // 链接：点击后用浏览器打开
    ShadCard* linkCard = addSection([] { return tr("链接"); }, [] { return tr("setUrl：把一行设为链接，用系统浏览器打开"); });
    ShadTrace* linkTrace = createTrace(this);
    addLinkLine(linkTrace, QStringLiteral("Install Astro | docs.astro.build"), QStringLiteral("https://docs.astro.build/en/install-and-setup/"));
    addStretchItem(linkCard, linkTrace);

    // 浮层：对应 Dialog / Sheet 示例，组件库中用 ShadPopover 显示详情
    ShadCard* popoverCard = addSection([] { return tr("浮层"); }, [] { return tr("点击可点击的行时打开浮层"); });
    ShadTrace* popoverTrace = createTrace(this);
    ShadTraceLine* searchLine = addTraceLine(popoverTrace, ShadIconType::Search, [] { return tr("已在网上搜索缓存键"); });
    searchLine->setIndicatorIcon(ShadIconType::ChevronRight);
    searchLine->setClickable(true);
    ShadPopover* searchPopover = new ShadPopover(searchLine);
    searchPopover->setContentWidth(kPopoverWidth);
    ExampleI18n::bind(searchPopover, [searchLine, searchPopover]()
    {
        searchLine->setDescription(tr("3 个结果"));
        searchPopover->setTitle(tr("搜索：pnpm cache key"));
        searchPopover->setDescription(tr("3 个结果，按相关度排序"));
    });
    ShadTrace* resultTrace = new ShadTrace(searchPopover);
    addLinkLine(resultTrace, QStringLiteral("Caching | pnpm.io"), QStringLiteral("https://pnpm.io/cli/store"));
    addLinkLine(resultTrace, QStringLiteral("actions/cache | github.com"), QStringLiteral("https://github.com/actions/cache"));
    addLinkLine(resultTrace, QStringLiteral("Turborepo caching | turbo.build"), QStringLiteral("https://turbo.build/repo/docs/crafting-your-repository/caching"));
    searchPopover->contentLayout()->addWidget(resultTrace);
    connect(searchLine, &ShadTraceLine::clicked, searchPopover, [searchPopover, searchLine]() { searchPopover->toggle(searchLine, ShadPlacementType::Bottom, ShadPlacementType::Start); });
    addStretchItem(popoverCard, popoverTrace);

    // 分组：相邻的几行放在同一个 ShadTrace 中
    ShadCard* groupCard = addSection([] { return tr("分组"); }, [] { return tr("相邻的几行放在同一个 ShadTrace 中"); });
    ShadTrace* groupTrace = createTrace(this);
    ShadTraceLine* lockfileLine = addTraceLine(groupTrace, ShadIconType::Search, [] { return tr("已搜索 lockfile 设置"); });
    ExampleI18n::bind(lockfileLine, [lockfileLine]() { lockfileLine->setDescription(tr("4 个结果")); });
    addTraceLine(groupTrace, ShadIconType::FileText, [] { return tr("已读取 pnpm-workspace.yaml"); });
    addTraceLine(groupTrace, ShadIconType::Terminal, [] { return tr("已运行 pnpm install"); })->setDescription(QStringLiteral("9s"));
    addStretchItem(groupCard, groupTrace);

    // 来源：展开后是一组链接行
    ShadCard* sourcesCard = addSection([] { return tr("来源"); }, [] { return tr("展开后显示一组来源链接"); });
    ShadTrace* sourcesTrace = createTrace(this);
    ShadTraceCollapsible* sourcesRow = addTraceCollapsible(sourcesTrace, ShadIconType::Globe, [] { return tr("使用了 3 个来源"); });
    ShadTrace* sourceList = new ShadTrace(sourcesRow);
    addLinkLine(sourceList, QStringLiteral("Install Astro | docs.astro.build"), QStringLiteral("https://docs.astro.build/en/install-and-setup/"));
    addLinkLine(sourceList, QStringLiteral("create-astro | npmjs.com"), QStringLiteral("https://www.npmjs.com/package/create-astro"));
    addLinkLine(sourceList, QStringLiteral("Settings: minimumReleaseAge | pnpm.io"), QStringLiteral("https://pnpm.io/settings#minimumreleaseage"));
    sourcesRow->contentLayout()->addWidget(sourceList);
    sourcesRow->setExpanded(true);
    addStretchItem(sourcesCard, sourcesTrace);

    // 思维链：父行的状态与标题由子步骤汇总得出
    ShadCard* thoughtCard = addSection([] { return tr("思维链"); }, [] { return tr("子步骤依次完成，父行按子步骤汇总状态"); });
    struct ThoughtStep
    {
        ShadIconType::IconName icon;
        ExampleI18n::TextFunction pending;
        ExampleI18n::TextFunction running;
        ExampleI18n::TextFunction done;
    };
    const QList<ThoughtStep> thoughtSteps{
        {ShadIconType::FileText, [] { return tr("读取 package.json"); }, [] { return tr("正在读取 package.json"); }, [] { return tr("已读取 package.json"); }},
        {ShadIconType::FolderSearch, [] { return tr("查找 next.config"); }, [] { return tr("正在查找 next.config"); }, [] { return tr("已找到 next.config.ts"); }},
        {ShadIconType::ListChecks, [] { return tr("检查构建脚本"); }, [] { return tr("正在检查构建脚本"); }, [] { return tr("已检查构建脚本"); }},
    };
    ShadTrace* thoughtTrace = createTrace(this);
    ShadTraceCollapsible* thoughtRow = thoughtTrace->addCollapsible(ShadIconType::Brain, QString());
    ShadTrace* stepTrace = new ShadTrace(thoughtRow);
    QList<ShadTraceLine*> stepLines;
    for (const ThoughtStep& step : thoughtSteps)
    {
        stepLines.append(stepTrace->addLine(step.icon, step.pending()));
    }
    thoughtRow->contentLayout()->addWidget(stepTrace);
    thoughtRow->setExpanded(true);
    std::shared_ptr<int> finishedSteps = std::make_shared<int>(0);
    // 按当前完成的步骤数设置状态与标题，切换语言时也调用
    const auto updateThought = [thoughtRow, stepLines, thoughtSteps, finishedSteps]()
    {
        const int count = int(thoughtSteps.size());
        for (int index = 0; index < count; ++index)
        {
            const ThoughtStep& step = thoughtSteps.at(index);
            ShadTraceLine* line = stepLines.at(index);
            if (index < *finishedSteps)
            {
                line->setStatus(ShadTraceType::Done);
                line->setTitle(step.done());
            }
            else if (index == *finishedSteps)
            {
                line->setStatus(ShadTraceType::Running);
                line->setTitle(step.running());
            }
            else
            {
                line->setStatus(ShadTraceType::Pending);
                line->setTitle(step.pending());
            }
        }
        // 有步骤在运行时父行也在运行，标题为当前步骤；全部完成后汇总
        ShadTraceLine* parentLine = thoughtRow->getTriggerLine();
        if (*finishedSteps < count)
        {
            parentLine->setStatus(ShadTraceType::Running);
            parentLine->setTitle(tr("正在思考：%1").arg(thoughtSteps.at(*finishedSteps).running()));
            parentLine->setDescription(QString());
        }
        else
        {
            parentLine->setStatus(ShadTraceType::Done);
            parentLine->setTitle(tr("思考了 %1 个步骤").arg(count));
            parentLine->setDescription(QStringLiteral("4s"));
        }
    };
    QTimer* thoughtTimer = new QTimer(thoughtRow);
    thoughtTimer->setInterval(kThoughtIntervalMs);
    connect(thoughtTimer, &QTimer::timeout, thoughtRow, [thoughtTimer, finishedSteps, updateThought, thoughtSteps]()
    {
        ++*finishedSteps;
        updateThought();
        if (*finishedSteps >= thoughtSteps.size())
        {
            thoughtTimer->stop();
        }
    });
    ShadButton* replayThoughtButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    replayThoughtButton->setButtonSize(ShadButtonType::Small);
    replayThoughtButton->setShadIcon(ShadIconType::RotateCcw);
    ExampleI18n::bind(replayThoughtButton, [replayThoughtButton]() { replayThoughtButton->setText(tr("重新思考")); });
    connect(replayThoughtButton, &ShadButton::clicked, thoughtRow, [thoughtTimer, finishedSteps, updateThought]()
    {
        *finishedSteps = 0;
        updateThought();
        thoughtTimer->start();
    });
    ExampleI18n::bind(thoughtRow, updateThought);
    thoughtTimer->start();
    addStretchItem(thoughtCard, thoughtTrace);
    addItem(thoughtCard, replayThoughtButton);

    // 提交：每个文件一行，点击后在浮层中显示 diff
    ShadCard* commitCard = addSection([] { return tr("提交"); }, [] { return tr("文件行显示改动行数，点击后在浮层中查看 diff"); });
    struct CommitFile
    {
        QString path;
        bool isAdded;
        int additions;
        int deletions;
        QStringList diff;
    };
    const QList<CommitFile> commitFiles{
        {QStringLiteral("app/login/page.tsx"), false, 2, 2, {QStringLiteral("   const { next } = await searchParams"), QStringLiteral("-  const redirectTo ="), QStringLiteral("-    next && next.startsWith(\"/\") ? next : \"/\""), QStringLiteral("+  const redirectTo = safeRedirect(next)"), QStringLiteral("+"), QStringLiteral("   return <LoginForm redirectTo={redirectTo} />")}},
        {QStringLiteral("lib/auth/redirect.ts"), true, 7, 0, {QStringLiteral("+export function safeRedirect(next: string | undefined) {"), QStringLiteral("+  if (!next || !next.startsWith(\"/\") || next.startsWith(\"//\")) {"), QStringLiteral("+    return \"/\""), QStringLiteral("+  }"), QStringLiteral("+"), QStringLiteral("+  return next"), QStringLiteral("+}")}},
    };
    ShadTrace* commitTrace = createTrace(this);
    ShadTraceCollapsible* commitRow = addTraceCollapsible(commitTrace, ShadIconType::GitCommitHorizontal, [] { return tr("已提交 a1b2c3：修复登录后的跳转"); });
    ShadTrace* fileTrace = new ShadTrace(commitRow);
    for (const CommitFile& file : commitFiles)
    {
        const QString path = file.path;
        const bool isAdded = file.isAdded;
        ShadTraceLine* fileLine = addTraceLine(fileTrace, isAdded ? ShadIconType::FilePlus : ShadIconType::FilePen, [path, isAdded] { return isAdded ? tr("已添加 %1").arg(path) : tr("已编辑 %1").arg(path); });
        fileLine->setDescription(QStringLiteral("+%1 −%2").arg(file.additions).arg(file.deletions));
        fileLine->setDescriptionMonospace(true);
        fileLine->setIndicatorIcon(ShadIconType::ChevronRight);
        fileLine->setClickable(true);
        // diff 的第一列为 +/- 标记，显示时去掉并按标记设置行样式
        QStringList code;
        for (const QString& line : file.diff)
        {
            code.append(line.mid(1));
        }
        ShadPopover* diffPopover = new ShadPopover(fileLine);
        diffPopover->setContentWidth(kPopoverWidth);
        diffPopover->setTitle(file.path);
        const int additions = file.additions;
        const int deletions = file.deletions;
        ExampleI18n::bind(diffPopover, [diffPopover, additions, deletions]() { diffPopover->setDescription(tr("%1 处新增，%2 处删除").arg(additions).arg(deletions)); });
        ShadCodeBlock* diffBlock = new ShadCodeBlock(code.join(u'\n'), diffPopover);
        diffBlock->setLanguage(QStringLiteral("tsx"));
        for (int index = 0; index < file.diff.size(); ++index)
        {
            const QChar marker = file.diff.at(index).at(0);
            if (marker == u'+')
            {
                diffBlock->setLineVariant(index, ShadCodeBlockType::AddedLine);
            }
            else if (marker == u'-')
            {
                diffBlock->setLineVariant(index, ShadCodeBlockType::RemovedLine);
            }
        }
        diffPopover->contentLayout()->addWidget(diffBlock);
        connect(fileLine, &ShadTraceLine::clicked, diffPopover, [diffPopover, fileLine]() { diffPopover->toggle(fileLine, ShadPlacementType::Bottom, ShadPlacementType::Start); });
    }
    commitRow->contentLayout()->addWidget(fileTrace);
    commitRow->setExpanded(true);
    addStretchItem(commitCard, commitTrace);

    // 完整回合：回合汇总 → 推理与命令 → 单个命令的输出，中间夹一段说明文字
    ShadCard* turnCard = addSection([] { return tr("完整回合"); }, [] { return tr("三层嵌套：回合展开为步骤，步骤再展开为输出"); });
    ShadTrace* turnTrace = createTrace(this);
    ShadTraceCollapsible* turnRow = addTraceCollapsible(turnTrace, ShadIconType::None, [] { return tr("工作了 2 分 4 秒，1 个命令失败"); });
    turnRow->getTriggerLine()->setStatus(ShadTraceType::Error);
    ShadTrace* turnSteps = new ShadTrace(turnRow);
    ShadTraceCollapsible* reasoningRow = addTraceCollapsible(turnSteps, ShadIconType::Brain, [] { return tr("思考"); });
    reasoningRow->getTriggerLine()->setDescription(QStringLiteral("12s"));
    addTraceText(reasoningRow, [] { return tr("在生成项目之前，先确认安装了哪些包管理器。"); });
    ShadTraceCollapsible* commandsRow = addTraceCollapsible(turnSteps, ShadIconType::Terminal, [] { return tr("运行了 2 个命令，1 个失败"); });
    commandsRow->getTriggerLine()->setStatus(ShadTraceType::Error);
    ShadTrace* commandList = new ShadTrace(commandsRow);
    addTraceLine(commandList, ShadIconType::Terminal, [] { return tr("已运行 node --version"); });
    ShadTraceCollapsible* failedRow = addTraceCollapsible(commandList, ShadIconType::Terminal, [] { return tr("运行 pnpm --version 失败"); });
    failedRow->getTriggerLine()->setStatus(ShadTraceType::Error);
    failedRow->contentLayout()->addWidget(createOutput(QStringLiteral("command not found: pnpm"), ShadCodeBlockType::Destructive, failedRow));
    commandsRow->contentLayout()->addWidget(commandList);
    commandsRow->setExpanded(true);
    turnRow->contentLayout()->addWidget(turnSteps);
    addTraceText(turnRow, [] { return tr("已安装 Node，没有安装 pnpm。接下来读取文档配置。"); });
    ShadTrace* readTrace = new ShadTrace(turnRow);
    ShadTraceLine* readLine = addTraceLine(readTrace, ShadIconType::FileText, [] { return tr("已读取 apps/v4/lib/docs.ts"); });
    readLine->setIndicatorIcon(ShadIconType::ChevronRight);
    readLine->setClickable(true);
    turnRow->contentLayout()->addWidget(readTrace);
    turnRow->setExpanded(true);
    addStretchItem(turnCard, turnTrace);
}

TracePage::~TracePage()
{
}
