-- 标签页：代码块、差异对比与折叠面板
local window = shad.Window.new()
window.windowTitle = "Lua 标签页"
window:resize(760, 560)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)

local tabs = shad.Tabs.new()
root:addWidget(tabs)
root:addStretch(1)

-- 新建标签页并返回其布局
local function addPage(title)
    local page = shad.Widget.new()
    local layout = shad.VBox.new(page)
    layout:setContentsMargins(0, 12, 0, 0)
    layout.spacing = 12
    tabs:addTab(title, page)
    return layout
end

-- 第一页：代码块
local codeLayout = addPage("代码块")
local code = shad.CodeBlock.new([[
{
  "name": "dashboard",
  "version": "1.1.0",
  "scripts": {
    "dev": "vite",
    "build": "vite build"
  }
}]])
code.language = "json"
code.title = "package.json"
code.lineNumbersVisible = true
code.copyButtonVisible = true
codeLayout:addWidget(code)
code:on("copied", function()
    print("代码已复制到剪贴板")
end)

-- 第二页：差异对比
local diffLayout = addPage("差异对比")
local split = shad.Switch.new("分栏视图")
local wrap = shad.Switch.new("自动换行")
local options = shad.HBox.new()
options:addWidget(split)
options:addWidget(wrap)
options:addStretch(1)
diffLayout:addLayout(options)

local diff = shad.DiffView.new()
diff.language = "cpp"
diff:setTexts([[
int sum(const QList<int>& values)
{
    int total = 0;
    for (int i = 0; i < values.size(); ++i)
    {
        total += values[i];
    }
    return total;
}]], [[
qint64 sum(const QList<int>& values)
{
    qint64 total = 0;
    for (const int value : values)
    {
        total += value;
    }
    return total;
}]])
diffLayout:addWidget(diff)
print("差异对比：新增 " .. diff:getAddedLineCount() .. " 行，删除 " .. diff:getRemovedLineCount() .. " 行")

split:on("toggled", function(checked)
    diff.mode = checked and shad.DiffViewType.Split or shad.DiffViewType.Unified
end)
wrap:on("toggled", function(checked)
    diff.wordWrap = checked
end)

-- 第三页：折叠面板
local accordionLayout = addPage("折叠面板")
local accordion = shad.Accordion.new()
accordion:addItem("library", "ShadcnWidgetTools 是什么？", "基于 Qt 6 Widgets 的 shadcn/ui 风格组件库，组件全部自绘。")
accordion:addItem("script", "脚本能做什么？", "创建窗口与组件、读写属性、连接信号，并通过定时器驱动界面更新。")
local progress = shad.Progress.new()
progress.value = 60
accordion:addItem("widget", "内容也可以是组件", progress)
accordion:setItemExpanded("library", true)
accordionLayout:addWidget(accordion)

tabs:on("currentChanged", function(index)
    print("切换到第 " .. (index + 1) .. " 个标签页")
end)

window:show()
