#include "CodeBlockPage.h"

#include <QChar>
#include <QHBoxLayout>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QWidget>
#include <QtGlobal>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadCodeBlock.h"
#include "ShadDef.h"
#include "ShadIconType.h"

namespace
{
    // 小尺寸示例并排显示
    constexpr int kSmallBlockWidth = 280 + 2 * ExampleLayout::kFocusRingWidth;
    // max-h-72 与 max-h-64
    constexpr int kLogMaxHeight = 288;
    constexpr int kCollapsedHeight = 256;
    // 流式输出每次追加的字符数与间隔
    constexpr int kStreamChunkSize = 6;
    constexpr int kStreamIntervalMs = 40;

    ShadCodeBlock* createBlock(const QString& code, QWidget* parent)
    {
        return new ShadCodeBlock(code, parent);
    }

    // 操作区中的图标按钮：Ghost 样式，icon-xs 尺寸；提示文字由调用方绑定
    ShadButton* createActionButton(ShadIconType::IconName icon, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Ghost, parent);
        button->setButtonSize(ShadButtonType::IconExtraSmall);
        button->setShadIcon(icon);
        return button;
    }
} // namespace

CodeBlockPage::CodeBlockPage(QWidget* parent)
    : ExampleBasePage([] { return tr("代码块"); }, [] { return tr("ShadCodeBlock：用于显示代码、终端输出、日志与 diff，支持语法高亮。"); }, parent)
{
    // 基础用法：只有代码区
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("setCode 设置代码，可拖动选择并复制"); });
    const QString buildOutput = QStringLiteral("$ pnpm build\n"
        "> next build\n"
        "\n"
        "   Next.js 15.3.0\n"
        "   Creating an optimized production build ...\n"
        " ✓ Compiled successfully in 4.2s\n"
        " ✓ Generating static pages (12/12)\n");
    addStretchItem(basicCard, createBlock(buildOutput, this));

    // 标题栏：标题、图标与复制按钮
    ShadCard* headerCard = addSection([] { return tr("标题栏"); }, [] { return tr("显示标题栏，结束侧带复制按钮"); });
    const QString installOutput = QStringLiteral("$ pnpm add @tanstack/react-query\n"
        "Packages: +2\n"
        "Progress: resolved 312, reused 310, downloaded 2, added 2, done\n"
        "\n"
        "dependencies:\n"
        "+ @tanstack/react-query 5.62.0\n");
    ShadCodeBlock* headerBlock = createBlock(installOutput, this);
    headerBlock->setTitleIcon(ShadIconType::Terminal);
    headerBlock->setCopyButtonVisible(true);
    addStretchItem(headerCard, headerBlock);

    // 操作按钮：换行开关 + 复制
    ShadCard* actionsCard = addSection([] { return tr("操作按钮"); }, [] { return tr("addActionWidget 添加操作按钮，这里用于切换自动换行"); });
    const QString lintOutput = QStringLiteral("$ pnpm lint\n"
        "./components/data-table.tsx\n"
        "  42:7  Warning: React Hook useMemo has a missing dependency: 'columns'. Either include it or remove the dependency array.  react-hooks/exhaustive-deps\n"
        "  88:3  Warning: Unexpected console statement.  no-console\n"
        "\n"
        "✖ 2 problems (0 errors, 2 warnings)\n");
    ShadCodeBlock* lintBlock = createBlock(lintOutput, this);
    lintBlock->setTitle(QStringLiteral("pnpm lint"));
    lintBlock->setTitleIcon(ShadIconType::Terminal);
    ShadButton* wrapButton = createActionButton(ShadIconType::WrapText, lintBlock);
    lintBlock->addActionWidget(wrapButton);
    lintBlock->setCopyButtonVisible(true);
    // 提示文字按当前是否换行生成，切换换行与切换语言时都重新设置
    const auto updateWrapToolTip = [wrapButton, lintBlock]()
    {
        wrapButton->setToolTip(lintBlock->isWordWrap() ? tr("取消自动换行") : tr("自动换行", "按钮提示"));
    };
    connect(wrapButton, &ShadButton::clicked, lintBlock, [lintBlock]()
    {
        lintBlock->setWordWrap(!lintBlock->isWordWrap());
    });
    connect(lintBlock, &ShadCodeBlock::wordWrapChanged, wrapButton, [wrapButton, updateWrapToolTip](bool wrap)
    {
        wrapButton->setVariant(wrap ? ShadButtonType::Secondary : ShadButtonType::Ghost);
        updateWrapToolTip();
    });
    ExampleI18n::bind(wrapButton, updateWrapToolTip);
    addStretchItem(actionsCard, lintBlock);

    // 悬浮操作：没有标题栏时，按钮悬浮在右上角
    ShadCard* floatingCard = addSection([] { return tr("悬浮操作"); }, [] { return tr("无标题时操作按钮悬浮在右上角，悬停或聚焦时淡入"); });
    const QString floatingCode = QStringLiteral("export function formatBytes(bytes: number) {\n"
        "  const units = [\"B\", \"KB\", \"MB\", \"GB\"]\n"
        "  const index = Math.floor(Math.log(bytes) / Math.log(1024))\n"
        "  return `${(bytes / 1024 ** index).toFixed(1)} ${units[index]}`\n"
        "}\n");
    ShadCodeBlock* floatingBlock = createBlock(floatingCode, this);
    floatingBlock->setLanguage(QStringLiteral("ts"));
    floatingBlock->setCopyButtonVisible(true);
    addStretchItem(floatingCard, floatingBlock);

    // 行号
    ShadCard* numbersCard = addSection([] { return tr("行号"); }, [] { return tr("显示行号，行号不会被选中或复制"); });
    const QString packageJson = QStringLiteral("{\n"
        "  \"name\": \"my-app\",\n"
        "  \"version\": \"0.1.0\",\n"
        "  \"private\": true,\n"
        "  \"scripts\": {\n"
        "    \"dev\": \"next dev --turbopack\",\n"
        "    \"build\": \"next build\"\n"
        "  }\n"
        "}\n");
    ShadCodeBlock* numbersBlock = createBlock(packageJson, this);
    numbersBlock->setTitle(QStringLiteral("package.json"));
    numbersBlock->setTitleIcon(ShadIconType::FileJson);
    numbersBlock->setLanguage(QStringLiteral("json"));
    numbersBlock->setLineNumbersVisible(true);
    numbersBlock->setCopyButtonVisible(true);
    addStretchItem(numbersCard, numbersBlock);

    // 起始行号
    ShadCard* startLineCard = addSection([] { return tr("起始行号"); }, [] { return tr("设置第一行的行号，用于展示文件中的一段代码"); });
    const QString dashboardCode = QStringLiteral("export default async function Page() {\n"
        "  const data = await getDashboardData()\n"
        "\n"
        "  return <Dashboard data={data} />\n"
        "}\n");
    ShadCodeBlock* startLineBlock = createBlock(dashboardCode, this);
    startLineBlock->setTitle(QStringLiteral("app/dashboard/page.tsx"));
    startLineBlock->setTitleIcon(ShadIconType::FileCode);
    startLineBlock->setLanguage(QStringLiteral("tsx"));
    startLineBlock->setLineNumbersVisible(true);
    startLineBlock->setStartLine(24);
    addStretchItem(startLineCard, startLineBlock);

    // 自动换行
    ShadCard* wrapCard = addSection([] { return tr("自动换行"); }, [] { return tr("开启时按单词换行，关闭时超出宽度横向滚动"); });
    const QString errorMessage = QStringLiteral("Error: Failed to collect page data for /dashboard/settings/notifications. The route handler threw an exception while reading NEXT_PUBLIC_NOTIFICATIONS_ENDPOINT from the environment.\n");
    ShadCodeBlock* wrapBlock = createBlock(errorMessage, this);
    wrapBlock->setWordWrap(true);
    ShadCodeBlock* scrollBlock = createBlock(errorMessage, this);
    addStretchItem(wrapCard, wrapBlock);
    addStretchItem(wrapCard, scrollBlock);

    // 高亮行
    ShadCard* highlightCard = addSection([] { return tr("高亮行"); }, [] { return tr("用背景色标出需要强调的重点行"); });
    const QString nextConfig = QStringLiteral("import type { NextConfig } from \"next\"\n"
        "\n"
        "const nextConfig: NextConfig = {\n"
        "  experimental: {\n"
        "    reactCompiler: true,\n"
        "  },\n"
        "}\n"
        "\n"
        "export default nextConfig\n");
    ShadCodeBlock* highlightBlock = createBlock(nextConfig, this);
    highlightBlock->setTitle(QStringLiteral("next.config.ts"));
    highlightBlock->setTitleIcon(ShadIconType::FileCode);
    highlightBlock->setLanguage(QStringLiteral("ts"));
    highlightBlock->setLineNumbersVisible(true);
    highlightBlock->setLineVariant(3, ShadCodeBlockType::HighlightedLine);
    highlightBlock->setLineVariant(4, ShadCodeBlockType::HighlightedLine);
    addStretchItem(highlightCard, highlightBlock);

    // Diff：按每行的 +/- 前缀设置样式，显示时去掉前缀
    ShadCard* diffCard = addSection([] { return QStringLiteral("Diff"); }, [] { return tr("标记新增行与删除行，标记不会被复制"); });
    const QStringList patchLines{
        QStringLiteral(" export function Greeting({ name }: { name: string }) {"),
        QStringLiteral("-  return <p>Hello, {name}</p>"),
        QStringLiteral("+  return <p className=\"text-sm text-muted-foreground\">"),
        QStringLiteral("+    Hello, {name}!"),
        QStringLiteral("+  </p>"),
        QStringLiteral(" }"),
    };
    QStringList diffCode;
    for (const QString& line : patchLines)
    {
        diffCode.append(line.mid(1));
    }
    ShadCodeBlock* diffBlock = createBlock(diffCode.join(u'\n'), this);
    diffBlock->setTitle(QStringLiteral("components/greeting.tsx"));
    diffBlock->setTitleIcon(ShadIconType::FileCode);
    diffBlock->setLanguage(QStringLiteral("tsx"));
    diffBlock->setCopyButtonVisible(true);
    for (int index = 0; index < patchLines.size(); ++index)
    {
        const QChar marker = patchLines.at(index).at(0);
        if (marker == u'+')
        {
            diffBlock->setLineVariant(index, ShadCodeBlockType::AddedLine);
        }
        else if (marker == u'-')
        {
            diffBlock->setLineVariant(index, ShadCodeBlockType::RemovedLine);
        }
    }
    addStretchItem(diffCard, diffBlock);

    // 限制高度：超出后纵向滚动，上下边缘渐隐
    ShadCard* heightCard = addSection([] { return tr("限制高度"); }, [] { return tr("限制内容高度，超出后纵向滚动"); });
    QStringList logLines;
    const QStringList routes{
        QStringLiteral("/"), QStringLiteral("/dashboard"), QStringLiteral("/api/users"), QStringLiteral("/settings"),
        QStringLiteral("/api/auth/session")
    };
    for (int index = 0; index < 30; ++index)
    {
        const QString route = routes.at(index % routes.size());
        logLines.append(QStringLiteral("[12:04:%1] GET %2 200 in %3ms").arg(index * 2 % 60, 2, 10, QChar(u'0')).arg(route).arg(12 + index * 7 % 90));
    }
    ShadCodeBlock* heightBlock = createBlock(logLines.join(u'\n'), this);
    heightBlock->setTitle(QStringLiteral("dev.log"));
    heightBlock->setTitleIcon(ShadIconType::ScrollText);
    heightBlock->setMaximumContentHeight(kLogMaxHeight);
    addStretchItem(heightCard, heightBlock);

    // 展开更多：底栏按钮切换限高
    ShadCard* moreCard = addSection([] { return tr("展开更多"); }, [] { return tr("收起时限制高度，点击底栏按钮展开全部代码"); });
    const QString chatCode = QStringLiteral("\"use client\"\n"
        "\n"
        "import { useChat } from \"@ai-sdk/react\"\n"
        "\n"
        "export function Chat() {\n"
        "  const { messages, input, handleInputChange, handleSubmit } = useChat()\n"
        "\n"
        "  return (\n"
        "    <div className=\"flex flex-col gap-4\">\n"
        "      {messages.map((message) => (\n"
        "        <div key={message.id} className=\"whitespace-pre-wrap\">\n"
        "          {message.role === \"user\" ? \"User: \" : \"AI: \"}\n"
        "          {message.content}\n"
        "        </div>\n"
        "      ))}\n"
        "      <form onSubmit={handleSubmit}>\n"
        "        <input\n"
        "          value={input}\n"
        "          placeholder=\"Say something...\"\n"
        "          onChange={handleInputChange}\n"
        "        />\n"
        "      </form>\n"
        "    </div>\n"
        "  )\n"
        "}\n");
    ShadCodeBlock* moreBlock = createBlock(chatCode, this);
    moreBlock->setTitle(QStringLiteral("components/chat.tsx"));
    moreBlock->setTitleIcon(ShadIconType::FileCode);
    moreBlock->setLanguage(QStringLiteral("tsx"));
    moreBlock->setCopyButtonVisible(true);
    moreBlock->setMaximumContentHeight(kCollapsedHeight);
    ShadButton* moreButton = new ShadButton(QString(), ShadButtonType::Ghost, moreBlock);
    moreButton->setButtonSize(ShadButtonType::Small);
    moreButton->setShadIcon(ShadIconType::ChevronDown);
    moreBlock->footerLayout()->addStretch(1);
    moreBlock->footerLayout()->addWidget(moreButton);
    moreBlock->footerLayout()->addStretch(1);
    // 按钮文字按当前是否展开生成：限高为 0 表示已展开
    const auto updateMoreButton = [moreBlock, moreButton]()
    {
        moreButton->setText(moreBlock->getMaximumContentHeight() == 0 ? tr("收起") : tr("展开"));
    };
    connect(moreButton, &ShadButton::clicked, moreBlock, [moreBlock, moreButton, updateMoreButton]()
    {
        const bool isExpanded = moreBlock->getMaximumContentHeight() == 0;
        moreBlock->setMaximumContentHeight(isExpanded ? kCollapsedHeight : 0);
        updateMoreButton();
        moreButton->setShadIcon(isExpanded ? ShadIconType::ChevronDown : ShadIconType::ChevronUp);
    });
    ExampleI18n::bind(moreButton, updateMoreButton);
    addStretchItem(moreCard, moreBlock);

    // 小尺寸
    ShadCard* sizeCard = addSection([] { return tr("小尺寸"); }, [] { return tr("更紧凑的小尺寸，适合嵌在工具调用记录中"); });
    ShadCodeBlock* versionBlock = new ShadCodeBlock(QStringLiteral("$ node --version\nv22.11.0\n"), this);
    versionBlock->setCodeBlockSize(ShadCodeBlockType::Small);
    versionBlock->setFixedWidth(kSmallBlockWidth);
    ShadCodeBlock* devBlock = new ShadCodeBlock(QStringLiteral("$ pnpm dev\n  ▲ Next.js 15.3.0\n  - Local: http://localhost:3000\n ✓ Ready in 1.2s\n"), this);
    devBlock->setCodeBlockSize(ShadCodeBlockType::Small);
    devBlock->setTitleIcon(ShadIconType::Terminal);
    devBlock->setCopyButtonVisible(true);
    devBlock->setFixedWidth(kSmallBlockWidth);
    addRow(sizeCard, {versionBlock, devBlock});

    // 错误样式
    ShadCard* destructiveCard = addSection([] { return tr("错误样式"); }, [] { return tr("标题与代码为危险色，用于错误输出与堆栈"); });
    const QString stackTrace = QStringLiteral("TypeError: Cannot read properties of undefined (reading 'map')\n"
        "    at UserList (components/user-list.tsx:14:22)\n"
        "    at renderWithHooks (react-dom.development.js:16305:18)\n"
        "    at mountIndeterminateComponent (react-dom.development.js:20069:13)\n");
    ShadCodeBlock* destructiveBlock = createBlock(stackTrace, this);
    destructiveBlock->setVariant(ShadCodeBlockType::Destructive);
    destructiveBlock->setTitleIcon(ShadIconType::CircleAlert);
    destructiveBlock->setCopyButtonVisible(true);
    addStretchItem(destructiveCard, destructiveBlock);

    // 语法高亮
    ShadCard* syntaxCard = addSection([] { return tr("语法高亮"); }, [] { return tr("setLanguage 开启语法高亮，配色随主题切换"); });
    const QString weatherJson = QStringLiteral("{\n"
        "  \"location\": \"San Francisco, CA\",\n"
        "  \"temperature\": 18.5,\n"
        "  \"unit\": \"celsius\",\n"
        "  \"conditions\": [\"cloudy\", \"windy\"],\n"
        "  \"precipitation\": null,\n"
        "  \"isDaytime\": true\n"
        "}\n");
    ShadCodeBlock* syntaxBlock = createBlock(weatherJson, this);
    syntaxBlock->setTitle(QStringLiteral("getWeather"));
    syntaxBlock->setTitleIcon(ShadIconType::Braces);
    syntaxBlock->setLanguage(QStringLiteral("json"));
    syntaxBlock->setCopyButtonVisible(true);
    addStretchItem(syntaxCard, syntaxBlock);

    // 多语言：支持的语言高亮，其他语言按纯文本显示
    ShadCard* languagesCard = addSection([] { return tr("多语言"); }, [] { return tr("内置多种语言，未内置的语言按纯文本显示"); });
    // 示例代码中的注释与文字随语言切换；代码块只读，重新设置代码不影响换行、行号与高亮行设置
    struct LanguageDemo
    {
        QString title;
        QString language;
        ExampleI18n::TextFunction code;
    };
    const QList<LanguageDemo> languageDemos{
        {
            QStringLiteral("globals.css"), QStringLiteral("css"),
            []
            {
                return tr("/* 卡片 */\n.card {\n  border-radius: var(--radius);\n  padding: 1.5rem;\n  color: #0a0a0a;\n}\n\n.card:hover {\n  box-shadow: 0 1px 2px rgb(0 0 0 / 0.05) !important;\n}\n");
            }
        },
        {
            QStringLiteral("forecast.py"), QStringLiteral("python"),
            []
            {
                return tr("from dataclasses import dataclass\n\n@dataclass\nclass Forecast:\n    city: str\n    high: float\n\ndef warmest(items: list[Forecast]) -> str:\n    \"\"\"返回最高温度的城市\"\"\"\n    best = max(items, key=lambda item: item.high)\n    return f\"{best.city}: {best.high:.1f}°C\"  # 保留一位小数\n");
            }
        },
        {
            QStringLiteral("main.cpp"), QStringLiteral("cpp"),
            []
            {
                return tr("#include <QApplication>\n\nint main(int argc, char* argv[])\n{\n    QApplication app(argc, argv);\n    // 显示一个代码块\n    ShadCodeBlock block(QStringLiteral(\"Hello\"));\n    block.setLanguage(QStringLiteral(\"cpp\"));\n    block.show();\n    return app.exec();\n}\n");
            }
        },
        {
            QStringLiteral("notes.txt"), QStringLiteral("txt"),
            [] { return tr("这是一段纯文本，不会被高亮。\nUnlisted languages render as plain text.\n"); }
        },
    };
    for (const LanguageDemo& demo : languageDemos)
    {
        ShadCodeBlock* block = createBlock(QString(), this);
        block->setTitle(demo.title);
        block->setTitleIcon(ShadIconType::FileCode);
        block->setLanguage(demo.language);
        block->setCopyButtonVisible(true);
        const ExampleI18n::TextFunction code = demo.code;
        ExampleI18n::bind(block, [block, code]() { block->setCode(code()); });
        addStretchItem(languagesCard, block);
    }

    // 终端输出：解析 ANSI 颜色转义码
    ShadCard* ansiCard = addSection([] { return tr("终端输出"); }, [] { return tr("解析终端颜色转义码，复制时去掉转义码"); });
    const QString testOutput = QStringLiteral("$ pnpm test\n"
        "\x1b[32m✓\x1b[39m src/format.test.ts \x1b[2m(4 tests)\x1b[22m\n"
        "\x1b[32m✓\x1b[39m src/parse.test.ts \x1b[2m(7 tests)\x1b[22m\n"
        "\x1b[31m✗\x1b[39m src/stream.test.ts \x1b[2m(3 tests | 1 failed)\x1b[22m\n"
        "\n"
        "\x1b[31mAssertionError: expected 'hello' to be 'hello world'\x1b[39m\n"
        "\n"
        "Tests  \x1b[31m1 failed\x1b[39m | \x1b[32m13 passed\x1b[39m (14)\n");
    ShadCodeBlock* ansiBlock = createBlock(testOutput, this);
    ansiBlock->setTitleIcon(ShadIconType::Terminal);
    ansiBlock->setLanguage(QStringLiteral("ansi"));
    ansiBlock->setWordWrap(true);
    ansiBlock->setCopyButtonVisible(true);
    addStretchItem(ansiCard, ansiBlock);

    // 流式输出：定时追加代码，只重新解析新增的行
    ShadCard* streamCard = addSection([] { return tr("流式输出"); }, [] { return tr("appendCode 逐段追加代码，点击重播按钮重新开始"); });
    const QString debounceCode = QStringLiteral("import { useEffect, useState } from \"react\"\n"
        "\n"
        "export function useDebounce<T>(value: T, delay = 300) {\n"
        "  const [debounced, setDebounced] = useState(value)\n"
        "\n"
        "  useEffect(() => {\n"
        "    const timer = setTimeout(() => setDebounced(value), delay)\n"
        "    return () => clearTimeout(timer)\n"
        "  }, [value, delay])\n"
        "\n"
        "  return debounced\n"
        "}\n");
    ShadCodeBlock* streamBlock = createBlock(QString(), this);
    streamBlock->setTitle(QStringLiteral("hooks/use-debounce.ts"));
    streamBlock->setTitleIcon(ShadIconType::FileCode);
    streamBlock->setLanguage(QStringLiteral("ts"));
    ShadButton* replayButton = createActionButton(ShadIconType::RotateCcw, streamBlock);
    streamBlock->addActionWidget(replayButton);
    streamBlock->setCopyButtonVisible(true);
    QTimer* streamTimer = new QTimer(streamBlock);
    streamTimer->setInterval(kStreamIntervalMs);
    std::shared_ptr<int> streamPosition = std::make_shared<int>(0);
    connect(streamTimer, &QTimer::timeout, streamBlock, [streamBlock, streamTimer, streamPosition, debounceCode]()
    {
        if (*streamPosition >= debounceCode.size())
        {
            streamTimer->stop();
            return;
        }
        streamBlock->appendCode(debounceCode.mid(*streamPosition, kStreamChunkSize));
        *streamPosition += kStreamChunkSize;
    });
    connect(replayButton, &ShadButton::clicked, streamBlock, [streamBlock, streamTimer, streamPosition]()
    {
        *streamPosition = 0;
        streamBlock->setCode(QString());
        streamTimer->start();
    });
    streamTimer->start();
    addStretchItem(streamCard, streamBlock);

    ExampleI18n::bind(this, [headerBlock, devBlock, destructiveBlock, ansiBlock, replayButton]()
    {
        headerBlock->setTitle(tr("终端"));
        devBlock->setTitle(tr("终端"));
        destructiveBlock->setTitle(tr("未处理的运行时错误"));
        ansiBlock->setTitle(tr("终端"));
        replayButton->setToolTip(tr("重播"));
    });
}

CodeBlockPage::~CodeBlockPage()
{
}
