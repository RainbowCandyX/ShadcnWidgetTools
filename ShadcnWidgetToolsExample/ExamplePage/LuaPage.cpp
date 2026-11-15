#include "LuaPage.h"

#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "LuaScriptManager.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadCodeBlock.h"
#include "ShadCodeEditor.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadLuaDef.h"
#include "ShadLuaEngine.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kEditorHeight = 360 + 2 * ExampleLayout::kFocusRingWidth;
    constexpr int kOutputHeight = 160;

    struct RunState
    {
        QPointer<ShadLuaEngine> engine;
        int scriptId{0};
        bool isShared{false};
        // 加载期间，输出与错误属于本区域
        bool isLoading{false};
    };

    // 示例脚本只使用 Lua 5.1 语法
    QString sampleScript()
    {
        return QStringLiteral("-- 用 Lua 创建一张卡片：属性直接读写，信号用 :on 连接\n"
                              "local card = shad.Card.new(\"Lua 组件\", \"这张卡片由 \" .. shad.runtime .. \" 创建\")\n"
                              "local layout = card:contentLayout()\n"
                              "\n"
                              "local input = shad.Input.new()\n"
                              "input.placeholderText = \"输入你的名字\"\n"
                              "layout:addWidget(input)\n"
                              "\n"
                              "local progress = shad.Progress.new()\n"
                              "progress.value = 30\n"
                              "layout:addWidget(progress)\n"
                              "\n"
                              "local hello = shad.Button.new(\"问好\")\n"
                              "local increase = shad.Button.new(\"增加进度\")\n"
                              "increase.variant = shad.ButtonType.Outline\n"
                              "local toggle = shad.Switch.new(\"显示进度条\")\n"
                              "toggle.checked = true\n"
                              "local row = shad.HBox.new()\n"
                              "row:addWidget(hello)\n"
                              "row:addWidget(increase)\n"
                              "row:addWidget(toggle)\n"
                              "row:addStretch(1)\n"
                              "layout:addLayout(row)\n"
                              "\n"
                              "local label = shad.Label.new(\"点击按钮试试\")\n"
                              "label.colorRole = shad.ThemeType.MutedForeground\n"
                              "layout:addWidget(label)\n"
                              "\n"
                              "-- 全局变量：共享状态下再次运行脚本会保留上一次的计数，隔离状态下每次从 0 开始\n"
                              "count = count or 0\n"
                              "hello:on(\"clicked\", function()\n"
                              "    count = count + 1\n"
                              "    local name = input.text\n"
                              "    if name == \"\" then name = \"世界\" end\n"
                              "    label.text = \"你好，\" .. name .. \"！累计点击 \" .. count .. \" 次\"\n"
                              "end)\n"
                              "increase:on(\"clicked\", function()\n"
                              "    progress.value = (progress.value + 10) % 110\n"
                              "end)\n"
                              "toggle:on(\"toggled\", function(checked)\n"
                              "    progress.visible = checked\n"
                              "end)\n"
                              "\n"
                              "return card\n");
    }
} // namespace

LuaPage::LuaPage(QWidget* parent)
    : ExampleBasePage([] { return tr("Lua 脚本"); }, [] { return tr("ShadLuaEngine：通过 LuaBridge3 用 Lua 或 LuaJIT 创建组件，脚本可以运行在隔离或共享的 Lua 状态中。"); }, parent)
{
    ShadCard* managerCard = addSection([] { return tr("脚本管理器"); }, [] { return tr("勾选加载脚本，取消勾选卸载并关闭它创建的窗口；print 的输出与错误显示在日志中。打开共享状态后，新加载的脚本共用同一个 Lua 状态"); });
    addStretchItem(managerCard, new LuaScriptManager(this));

    ShadCard* scriptCard = addSection([] { return tr("脚本"); }, [] { return tr("脚本通过全局表 shad 创建组件，以 return 返回根组件，也可以不返回组件只执行功能；打开开关后在共享状态中运行，全局变量在多次运行之间保留"); });
    ShadCodeEditor* editor = new ShadCodeEditor(sampleScript(), this);
    editor->setFixedHeight(kEditorHeight);
    ShadButton* runButton = new ShadButton(QString(), ShadButtonType::Default, this);
    runButton->setButtonSize(ShadButtonType::Small);
    ExampleI18n::bind(runButton, [runButton]() { runButton->setText(tr("运行")); });
    ShadSwitch* sharedSwitch = new ShadSwitch(this);
    ExampleI18n::bind(sharedSwitch, [sharedSwitch]() { sharedSwitch->setText(tr("共享状态")); });
    ShadLabel* runtimeLabel = new ShadLabel(ShadLuaEngine::runtimeName(), this);
    runtimeLabel->setColorRole(ShadThemeType::MutedForeground);
    addStretchItem(scriptCard, editor);
    addRow(scriptCard, {runButton, sharedSwitch, runtimeLabel});

    ShadCard* previewCard = addSection([] { return tr("预览"); }, [] { return tr("脚本返回的组件显示在这里；不返回组件的纯功能脚本照常执行。print 的输出与错误信息显示在下方，程序不会中断"); });
    QWidget* previewArea = new QWidget(this);
    QVBoxLayout* previewLayout = new QVBoxLayout(previewArea);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    ShadLabel* hintLabel = new ShadLabel(this);
    hintLabel->setColorRole(ShadThemeType::MutedForeground);
    ExampleI18n::bind(hintLabel, [hintLabel]() { hintLabel->setText(tr("脚本已执行，没有返回组件")); });
    hintLabel->hide();
    ShadCodeBlock* outputBlock = new ShadCodeBlock(this);
    outputBlock->setWordWrap(true);
    outputBlock->setMaximumContentHeight(kOutputHeight);
    ExampleI18n::bind(outputBlock, [outputBlock]() { outputBlock->setTitle(tr("输出")); });
    outputBlock->hide();
    ShadCodeBlock* errorBlock = new ShadCodeBlock(this);
    errorBlock->setVariant(ShadCodeBlockType::Destructive);
    errorBlock->setWordWrap(true);
    errorBlock->hide();
    addStretchItem(previewCard, previewArea);
    addItem(previewCard, hintLabel);
    addStretchItem(previewCard, outputBlock);
    addStretchItem(previewCard, errorBlock);

    std::shared_ptr<RunState> state = std::make_shared<RunState>();
    // 共享引擎中只显示本区域运行的脚本的内容
    const auto connectEngine = [state, outputBlock, errorBlock](ShadLuaEngine* engine)
    {
        const auto isOwnScript = [state, engine]() { return state->isLoading || (state->engine == engine && state->scriptId != 0 && engine->getCurrentScript() == state->scriptId); };
        QObject::connect(engine, &ShadLuaEngine::messagePrinted, outputBlock, [isOwnScript, outputBlock](const QString& message)
        {
            if (isOwnScript())
            {
                outputBlock->appendCode(message + QStringLiteral("\n"));
                outputBlock->show();
            }
        });
        QObject::connect(engine, &ShadLuaEngine::errorOccurred, errorBlock, [isOwnScript, errorBlock](const QString& message)
        {
            if (isOwnScript())
            {
                errorBlock->setCode(message);
                errorBlock->show();
            }
        });
    };
    connectEngine(ShadLuaEngine::shared());

    connect(runButton, &ShadButton::clicked, previewArea, [editor, sharedSwitch, previewArea, previewLayout, hintLabel, outputBlock, errorBlock, connectEngine, state]()
    {
        // 卸载上一次运行的脚本
        if (state->scriptId != 0 && !state->engine.isNull())
        {
            state->engine->unload(state->scriptId);
            if (!state->isShared)
            {
                state->engine->deleteLater();
            }
        }
        state->engine = nullptr;
        state->scriptId = 0;
        hintLabel->hide();
        errorBlock->hide();
        outputBlock->setCode(QString());
        outputBlock->hide();

        const bool isShared = sharedSwitch->isChecked();
        ShadLuaEngine* engine = isShared ? ShadLuaEngine::shared() : new ShadLuaEngine(previewArea);
        if (!isShared)
        {
            connectEngine(engine);
        }
        state->isLoading = true;
        const int scriptId = engine->load(editor->toPlainText(), QStringLiteral("editor"));
        state->isLoading = false;
        if (scriptId == 0)
        {
            if (!isShared)
            {
                engine->deleteLater();
            }
            return;
        }
        state->engine = engine;
        state->scriptId = scriptId;
        state->isShared = isShared;
        QWidget* widget = engine->getScriptWidget(scriptId);
        if (widget == nullptr)
        {
            hintLabel->show();
            return;
        }
        previewLayout->addWidget(widget);
        widget->show();
    });
}

LuaPage::~LuaPage()
{
}
