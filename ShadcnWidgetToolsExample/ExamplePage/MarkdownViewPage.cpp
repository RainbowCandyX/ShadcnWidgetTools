#include "MarkdownViewPage.h"

#include <QScrollBar>
#include <QString>
#include <QTimer>
#include <QtGlobal>

#include <memory>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadMarkdownView.h"

namespace
{
    constexpr int kStreamChunkSize = 4;
    constexpr int kStreamIntervalMs = 30;
    constexpr int kFollowThreshold = 4;

    struct StreamState
    {
        QString text;
        qsizetype position{0};
        bool isFollowing{false};
    };
} // namespace

MarkdownViewPage::MarkdownViewPage(QWidget* parent)
    : ExampleBasePage([] { return tr("Markdown"); }, [] { return tr("ShadMarkdownView：渲染 Markdown 文本，代码块使用 ShadCodeBlock，支持流式追加。"); }, parent)
{
    const ExampleI18n::TextFunction renderSample = []
    {
        return tr("# 欢迎使用 ShadMarkdownView\n"
            "\n"
            "这是一段包含 **粗体**、*斜体* 和 `行内代码` 的段落，还有一个[链接](https://www.qt.io)。\n"
            "\n"
            "## 列表\n"
            "\n"
            "- 无序列表第一项\n"
            "- 无序列表第二项\n"
            "\n"
            "1. 有序列表第一项\n"
            "2. 有序列表第二项\n"
            "\n"
            "- [x] 已完成的任务\n"
            "- [ ] 未完成的任务\n"
            "\n"
            "### 引用\n"
            "\n"
            "> 引用块使用次要文字颜色，左侧带有竖线。\n"
            "\n"
            "## 表格\n"
            "\n"
            "| 组件 | 说明 | 状态 |\n"
            "| --- | --- | --- |\n"
            "| ShadCodeBlock | 代码块 | 可用 |\n"
            "| ShadMarkdownView | Markdown 渲染 | 可用 |\n"
            "\n"
            "## 代码\n"
            "\n"
            "```cpp\n"
            "#include <QApplication>\n"
            "\n"
            "int main(int argc, char* argv[])\n"
            "{\n"
            "    QApplication app(argc, argv);\n"
            "    return app.exec();\n"
            "}\n"
            "```\n"
            "\n"
            "```bash\n"
            "cmake -S . -B build\n"
            "cmake --build build\n"
            "```\n"
            "\n"
            "---\n"
            "\n"
            "文末的水平线之上是各种元素的示例。\n");
    };

    const ExampleI18n::TextFunction streamSample = []
    {
        return tr("## 流式输出\n"
            "\n"
            "下面的内容逐字追加，**代码块**也会随之增长：\n"
            "\n"
            "```python\n"
            "def fibonacci(count):\n"
            "    values = [0, 1]\n"
            "    while len(values) < count:\n"
            "        values.append(values[-1] + values[-2])\n"
            "    return values[:count]\n"
            "```\n"
            "\n"
            "- 代码块闭合后继续输出文字\n"
            "- 只有最后一段会被重新构建\n"
            "\n"
            "输出结束。\n");
    };

    ShadCard* renderCard = addSection([] { return tr("渲染"); }, [] { return tr("setMarkdown 渲染标题、列表、引用、表格、代码块与水平线"); });
    ShadMarkdownView* renderView = new ShadMarkdownView(this);
    ExampleI18n::bind(renderView, [renderView, renderSample]() { renderView->setMarkdown(renderSample()); });
    addStretchItem(renderCard, renderView);

    ShadCard* streamCard = addSection([] { return tr("流式输出"); }, [] { return tr("appendMarkdown 逐段追加文字，模拟 AI 逐字输出"); });
    ShadMarkdownView* streamView = new ShadMarkdownView(this);
    ShadButton* startButton = new ShadButton(QString(), ShadButtonType::Default, this);
    ShadButton* resetButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    QTimer* streamTimer = new QTimer(this);
    streamTimer->setInterval(kStreamIntervalMs);
    const std::shared_ptr<StreamState> state = std::make_shared<StreamState>();
    connect(streamTimer, &QTimer::timeout, streamView, [streamView, streamTimer, state]()
    {
        if (state->position >= state->text.size())
        {
            streamTimer->stop();
            return;
        }
        streamView->appendMarkdown(state->text.mid(state->position, kStreamChunkSize));
        state->position += kStreamChunkSize;
    });
    connect(startButton, &ShadButton::clicked, streamView, [streamView, streamTimer, state, streamSample]()
    {
        state->text = streamSample();
        state->position = 0;
        state->isFollowing = true;
        streamView->clear();
        streamTimer->start();
    });
    QScrollBar* pageScrollBar = verticalScrollBar();
    connect(pageScrollBar, &QScrollBar::valueChanged, this, [pageScrollBar, streamTimer, state](int value)
    {
        if (streamTimer->isActive())
        {
            state->isFollowing = value >= pageScrollBar->maximum() - kFollowThreshold;
        }
    });
    connect(pageScrollBar, &QScrollBar::rangeChanged, this, [pageScrollBar, streamTimer, state](int minimum, int maximum)
    {
        Q_UNUSED(minimum);
        if (streamTimer->isActive() && state->isFollowing)
        {
            pageScrollBar->setValue(maximum);
        }
    });
    connect(resetButton, &ShadButton::clicked, streamView, [streamView, streamTimer, state]()
    {
        streamTimer->stop();
        state->position = 0;
        streamView->clear();
    });
    ExampleI18n::bind(this, [startButton, resetButton]()
    {
        startButton->setText(tr("开始"));
        resetButton->setText(tr("重置"));
    });
    addRow(streamCard, {startButton, resetButton});
    addStretchItem(streamCard, streamView);
}

MarkdownViewPage::~MarkdownViewPage()
{
}
