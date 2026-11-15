-- 布局与容器
local window = shad.MainWindow.new()
window.windowTitle = "Lua 布局与容器"
window:resize(900, 600)

local shell = shad.SidebarLayout.new()
window:setCentralWidget(shell)

-- 侧栏
local sidebar = shad.Sidebar.new()
local navigation = sidebar:addGroup("导航")
local overview = navigation:addItem(0, "概览")
local reports = navigation:addItem(0, "报表")
reports:addSubItem("月报")
reports:addSubItem("年报")
local settings = sidebar:addGroup("设置")
settings:addItem(0, "偏好")
sidebar:setActiveButton(overview)
shell:setSidebar(sidebar)

local content = shad.Widget.new()
local contentLayout = shad.VBox.new(content)
contentLayout:setContentsMargins(16, 12, 16, 12)
shell:setContentWidget(content)

-- 工具栏
local toolbar = shad.Toolbar.new()
toolbar.buttonStyle = shad.ToolbarType.TextOnly
toolbar:addWidget(shad.SidebarTrigger.new(sidebar))
toolbar:addSeparator()
local newTab = toolbar:addAction(0, "新建标签")
local closeTab = toolbar:addAction(0, "关闭当前标签")
contentLayout:addWidget(toolbar)

-- 标签视图
local tabs = shad.TabView.new()
tabs.tabsClosable = true
contentLayout:addWidget(tabs, 1)

-- 状态栏
local status = shad.StatusBar.new()
local countLabel = shad.Label.new("")
status:addPermanentWidget(countLabel)
contentLayout:addWidget(status)

local function makePanel(text)
    local panel = shad.Widget.new()
    local layout = shad.VBox.new(panel)
    layout:addWidget(shad.Label.new(text))
    layout:addStretch(1)
    return panel
end

-- 面板组
local split = shad.Resizable.new(shad.Qt.Horizontal)
split.gripVisible = true
split:addWidget(makePanel("左侧面板，拖动中间的分隔线调整大小"))
split:addWidget(makePanel("右侧面板"))
split:setSizes({260, 420})
tabs:addTab("分栏", split)

-- 折叠面板
local collapsePage = shad.Widget.new()
local collapseLayout = shad.VBox.new(collapsePage)
local collapsible = shad.Collapsible.new()
local trigger = shad.Button.new("展开或折叠详细信息")
trigger.variant = shad.ButtonType.Outline
collapsible:headerLayout():addWidget(trigger)
collapsible:setTrigger(trigger)
collapsible:contentLayout():addWidget(shad.Label.new("这段文字放在折叠面板的内容区中"))
collapsible.open = true
collapseLayout:addWidget(collapsible)
collapseLayout:addStretch(1)
tabs:addTab("折叠面板", collapsePage)

-- 流式布局
local flowPage = shad.Widget.new()
local flow = shad.FlowLayout.new(flowPage)
for index = 1, 12 do
    flow:addWidget(shad.Badge.new("标签 " .. index))
end
tabs:addTab("流式布局", flowPage)

local created = 0

local function updateCount()
    countLabel.text = tabs:count() .. " 个标签"
end

-- 移除标签不会删除页面，页面需要自己删除
local function closeTabAt(index)
    if index < 0 then
        return
    end
    local page = tabs:widget(index)
    tabs:removeTab(index)
    if page then
        page:delete()
    end
    updateCount()
end

newTab:on("triggered", function()
    created = created + 1
    local index = tabs:addTab("新标签 " .. created, makePanel("这是第 " .. created .. " 个新建的标签页"))
    tabs.currentIndex = index
    updateCount()
    status:showMessage("已新建标签", 2000)
end)

closeTab:on("triggered", function()
    closeTabAt(tabs.currentIndex)
end)

tabs:on("tabCloseRequested", closeTabAt)

sidebar:on("itemActivated", function()
    local button = sidebar:getActiveButton()
    if button then
        status:showMessage("已选择：" .. button.text, 2000)
    end
end)

updateCount()
window:show()
