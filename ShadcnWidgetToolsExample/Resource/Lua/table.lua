-- 数据表格与树形视图，使用 shad.Model 作为数据模型
local window = shad.Window.new()
window.windowTitle = "Lua 表格与树"
window:resize(820, 640)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)

local tabs = shad.Tabs.new()
root:addWidget(tabs)
root:addStretch(1)

local function addPage(title)
    local page = shad.Widget.new()
    local layout = shad.VBox.new(page)
    layout:setContentsMargins(0, 12, 0, 0)
    layout.spacing = 12
    tabs:addTab(title, page)
    return layout
end

-- 数据表格
local tableLayout = addPage("数据表格")
local model = shad.Model.new()
model:setHeaders({"订单号", "客户", "状态", "金额"})
local customers = {"张伟", "王芳", "李娜", "刘洋", "陈静", "杨帆", "赵磊", "黄敏"}
local states = {"待付款", "处理中", "已完成", "已取消"}
for i = 1, 36 do
    model:addRow({"ORD-" .. (1000 + i), customers[(i - 1) % #customers + 1], states[(i - 1) % #states + 1], 80 + (i * 37) % 900})
end

local dataTable = shad.DataTable.new()
dataTable:setSourceModel(model)
dataTable:setFilterColumn(1)
dataTable:setFilterPlaceholderText("按客户筛选...")
dataTable:setColumnSortable(0, true)
dataTable:setColumnSortable(3, true)
dataTable:setColumnHideable(2, true)
dataTable:setColumnHideable(3, true)
dataTable:setSelectionEnabled(true)
dataTable:setPageSize(8)
tableLayout:addWidget(dataTable)

local summary = shad.Label.new("未选择任何行")
summary.colorRole = shad.ThemeType.MutedForeground
tableLayout:addWidget(summary)

dataTable:on("selectionChanged", function()
    local rows = dataTable:getSelectedRows()
    local total = 0
    for _, row in ipairs(rows) do
        total = total + model:getValue(row, 3)
    end
    summary.text = "已选择 " .. #rows .. " 行，金额合计 " .. total
    print("选择变化：" .. #rows .. " 行，合计 " .. total)
end)

-- 树形视图
local treeLayout = addPage("树形视图")
local treeModel = shad.Model.new()
treeModel:setTree({
    {text = "ShadcnWidgetTools", checked = false, children = {
        {text = "按钮与操作", checked = false, children = {
            {text = "ShadButton", checked = true},
            {text = "ShadToggle", checked = false},
            {text = "ShadSegmented", checked = false},
        }},
        {text = "数据展示", checked = false, children = {
            {text = "ShadDataTable", checked = true},
            {text = "ShadTreeView", checked = true},
            {text = "ShadLineChart", checked = false},
        }},
    }},
    {text = "ThirdParty", children = {
        "LuaJIT",
        "LuaBridge3",
    }},
})

local tree = shad.TreeView.new()
tree:setModel(treeModel)
tree:expandAll()
tree:setFixedHeight(360)
treeLayout:addWidget(tree)

local path = shad.Label.new("点击节点查看路径")
path.colorRole = shad.ThemeType.MutedForeground
treeLayout:addWidget(path)

tree:on("clicked", function()
    local names = tree:currentPath()
    path.text = "当前节点：" .. table.concat(names, " / ")
    print("点击节点：" .. tree:currentText())
end)

window:show()
