#include "DiffViewPage.h"

#include <QList>
#include <QString>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDiffView.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kContextLines = 2;

    QString oldCode()
    {
        return QStringLiteral("#include <QString>\n"
                              "\n"
                              "int sum(const QList<int>& values)\n"
                              "{\n"
                              "    int total = 0;\n"
                              "    for (int i = 0; i < values.size(); ++i)\n"
                              "    {\n"
                              "        total += values[i];\n"
                              "    }\n"
                              "    return total;\n"
                              "}\n"
                              "\n"
                              "void printSum(const QList<int>& values)\n"
                              "{\n"
                              "    qDebug() << \"sum:\" << sum(values);\n"
                              "}\n");
    }

    QString newCode()
    {
        return QStringLiteral("#include <QList>\n"
                              "#include <QString>\n"
                              "\n"
                              "qint64 sum(const QList<int>& values)\n"
                              "{\n"
                              "    qint64 total = 0;\n"
                              "    for (const int value : values)\n"
                              "    {\n"
                              "        total += value;\n"
                              "    }\n"
                              "    return total;\n"
                              "}\n"
                              "\n"
                              "void printSum(const QList<int>& values)\n"
                              "{\n"
                              "    qInfo() << \"sum:\" << sum(values);\n"
                              "}\n");
    }

    QString oldNotes()
    {
        return QStringLiteral("# Release Notes\n"
                              "\n"
                              "The chart components are rendered on the GPU through QRhi and fall back to the CPU when no GPU is available, so they keep working on remote desktops and virtual machines.\n"
                              "\n"
                              "The example program ships prebuilt packages for Windows and macOS.\n");
    }

    QString newNotes()
    {
        return QStringLiteral("# Release Notes\n"
                              "\n"
                              "The chart components are rendered on the GPU through QRhi and fall back to software rendering when no GPU is available, so they keep working on remote desktops, virtual machines and containers.\n"
                              "\n"
                              "The example program ships prebuilt packages for Windows, macOS and Linux.\n");
    }

    QString oldConfig()
    {
        return QStringLiteral("{\n"
                              "  \"name\": \"dashboard\",\n"
                              "  \"version\": \"1.0.0\",\n"
                              "  \"private\": true,\n"
                              "  \"scripts\": {\n"
                              "    \"dev\": \"vite\",\n"
                              "    \"build\": \"vite build\",\n"
                              "    \"preview\": \"vite preview\",\n"
                              "    \"lint\": \"eslint .\"\n"
                              "  },\n"
                              "  \"dependencies\": {\n"
                              "    \"clsx\": \"^2.1.1\",\n"
                              "    \"lucide-react\": \"^0.468.0\",\n"
                              "    \"react\": \"^18.3.1\",\n"
                              "    \"react-dom\": \"^18.3.1\",\n"
                              "    \"tailwind-merge\": \"^2.5.5\"\n"
                              "  },\n"
                              "  \"devDependencies\": {\n"
                              "    \"eslint\": \"^9.17.0\",\n"
                              "    \"tailwindcss\": \"^3.4.17\",\n"
                              "    \"typescript\": \"^5.7.2\",\n"
                              "    \"vite\": \"^6.0.5\"\n"
                              "  }\n"
                              "}\n");
    }

    QString newConfig()
    {
        return QStringLiteral("{\n"
                              "  \"name\": \"dashboard\",\n"
                              "  \"version\": \"1.1.0\",\n"
                              "  \"private\": true,\n"
                              "  \"scripts\": {\n"
                              "    \"dev\": \"vite\",\n"
                              "    \"build\": \"vite build\",\n"
                              "    \"preview\": \"vite preview\",\n"
                              "    \"lint\": \"eslint .\"\n"
                              "  },\n"
                              "  \"dependencies\": {\n"
                              "    \"clsx\": \"^2.1.1\",\n"
                              "    \"lucide-react\": \"^0.468.0\",\n"
                              "    \"react\": \"^18.3.1\",\n"
                              "    \"react-dom\": \"^18.3.1\",\n"
                              "    \"tailwind-merge\": \"^2.5.5\"\n"
                              "  },\n"
                              "  \"devDependencies\": {\n"
                              "    \"eslint\": \"^9.17.0\",\n"
                              "    \"tailwindcss\": \"^3.4.17\",\n"
                              "    \"typescript\": \"^5.7.2\",\n"
                              "    \"vite\": \"^6.0.5\",\n"
                              "    \"vitest\": \"^2.1.8\"\n"
                              "  }\n"
                              "}\n");
    }
} // namespace

DiffViewPage::DiffViewPage(QWidget* parent)
    : ExampleBasePage([] { return tr("差异对比"); }, [] { return tr("ShadDiffView：按行比较两段文本，标出新增、删除与行内发生变化的字符。"); }, parent)
{
    ShadCard* unifiedCard = addSection([] { return tr("统一视图"); }, [] { return tr("setTexts 设置旧文本与新文本，setLanguage(\"cpp\") 开启语法高亮；左侧两列分别是旧行号与新行号"); });
    ShadDiffView* unifiedView = new ShadDiffView(oldCode(), newCode(), this);
    unifiedView->setLanguage(QStringLiteral("cpp"));
    ShadLabel* countLabel = new ShadLabel(this);
    countLabel->setColorRole(ShadThemeType::MutedForeground);
    ExampleI18n::bind(countLabel, [countLabel, unifiedView]() { countLabel->setText(tr("新增 %1 行，删除 %2 行").arg(unifiedView->getAddedLineCount()).arg(unifiedView->getRemovedLineCount())); });
    addStretchItem(unifiedCard, unifiedView);
    addItem(unifiedCard, countLabel);

    ShadCard* splitCard = addSection([] { return tr("分栏视图"); }, [] { return tr("setMode(Split)：旧文本在左、新文本在右，开关可随时切换"); });
    ShadSwitch* splitSwitch = new ShadSwitch(this);
    splitSwitch->setChecked(true);
    ExampleI18n::bind(splitSwitch, [splitSwitch]() { splitSwitch->setText(tr("分栏视图")); });
    ShadDiffView* splitView = new ShadDiffView(oldCode(), newCode(), this);
    splitView->setLanguage(QStringLiteral("cpp"));
    splitView->setMode(ShadDiffViewType::Split);
    connect(splitSwitch, &ShadSwitch::toggled, splitView, [splitView](bool checked) { splitView->setMode(checked ? ShadDiffViewType::Split : ShadDiffViewType::Unified); });
    addItem(splitCard, splitSwitch);
    addStretchItem(splitCard, splitView);

    ShadCard* foldCard = addSection([] { return tr("折叠未改动的行"); }, [] { return tr("setContextLines(2)：每处改动前后只保留 2 行，其余未改动的行折叠为一行，点击后展开"); });
    ShadDiffView* foldView = new ShadDiffView(oldConfig(), newConfig(), this);
    foldView->setLanguage(QStringLiteral("json"));
    foldView->setContextLines(kContextLines);
    addStretchItem(foldCard, foldView);

    ShadCard* wrapCard = addSection([] { return tr("自动换行与选择"); }, [] { return tr("setWordWrap(true) 让长行在栏位内折行；setSelectable(false) 关闭文字选择与复制"); });
    ShadSwitch* wrapSwitch = new ShadSwitch(this);
    wrapSwitch->setChecked(true);
    ExampleI18n::bind(wrapSwitch, [wrapSwitch]() { wrapSwitch->setText(tr("自动换行")); });
    ShadSwitch* selectableSwitch = new ShadSwitch(this);
    selectableSwitch->setChecked(true);
    ExampleI18n::bind(selectableSwitch, [selectableSwitch]() { selectableSwitch->setText(tr("可选择")); });
    ShadDiffView* wrapView = new ShadDiffView(oldNotes(), newNotes(), this);
    wrapView->setWordWrap(true);
    connect(wrapSwitch, &ShadSwitch::toggled, wrapView, [wrapView](bool checked) { wrapView->setWordWrap(checked); });
    connect(selectableSwitch, &ShadSwitch::toggled, wrapView, [wrapView](bool checked) { wrapView->setSelectable(checked); });
    addRow(wrapCard, {wrapSwitch, selectableSwitch});
    addStretchItem(wrapCard, wrapView);
}

DiffViewPage::~DiffViewPage()
{
}
