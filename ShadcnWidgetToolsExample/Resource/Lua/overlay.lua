-- 弹层与菜单
local window = shad.Window.new()
window.windowTitle = "Lua 弹层与菜单"
window:resize(560, 420)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)
root.spacing = 12

local result = shad.Label.new("点击下面的按钮，或在这行文字上点击右键")
result.colorRole = shad.ThemeType.MutedForeground

-- 菜单栏
local menubar = shad.Menubar.new()
local fileMenu = menubar:addMenu("文件")
local newItem = fileMenu:addItem("新建", 0, shad.KbdGroup.shortcutText("Ctrl+N"))
local autoSave = fileMenu:addCheckableItem("自动保存", true)
fileMenu:addSeparator()
local removeItem = fileMenu:addItem("删除")
fileMenu:setItemDestructive(removeItem, true)

-- 单选项用 shad.ActionGroup 互斥
local viewMenu = menubar:addMenu("视图")
local sizes = shad.ActionGroup.new(viewMenu)
viewMenu:addLabel("字号")
viewMenu:addRadioItem("小", sizes)
viewMenu:addRadioItem("中", sizes, true)
viewMenu:addRadioItem("大", sizes)

root:addWidget(menubar)
root:addWidget(result)

newItem:on("triggered", function()
    result.text = "菜单栏：新建"
end)
removeItem:on("triggered", function()
    result.text = "菜单栏：删除"
end)
autoSave:on("toggled", function(checked)
    result.text = "菜单栏：自动保存 " .. (checked and "开启" or "关闭")
end)

local buttons = shad.Widget.new()
local flow = shad.FlowLayout.new(buttons)
root:addWidget(buttons)
root:addStretch(1)

local function addButton(text)
    local button = shad.Button.new(text)
    button.variant = shad.ButtonType.Outline
    flow:addWidget(button)
    return button
end

-- 下拉菜单
local menu = shad.DropdownMenu.new(window)
menu:addLabel("账户")
local profile = menu:addItem("个人资料")
local logout = menu:addItem("退出登录")
menu:setItemDestructive(logout, true)
local menuButton = addButton("下拉菜单")
menuButton.toolTip = "点击弹出菜单"
shad.Tooltip.setShortcut(menuButton, {"Alt", "M"})
menuButton:on("clicked", function()
    menu:popup(menuButton)
end)
profile:on("triggered", function()
    result.text = "下拉菜单：个人资料"
end)
logout:on("triggered", function()
    result.text = "下拉菜单：退出登录"
end)

-- 右键菜单
local contextMenu = shad.ContextMenu.new(window)
local copy = contextMenu:addItem("复制文字")
contextMenu:attachTo(result)
copy:on("triggered", function()
    result.text = "右键菜单：复制文字"
end)

-- 浮层
local popover = shad.Popover.new(window)
popover.title = "尺寸"
popover.description = "设置图层的宽度"
local width = shad.Input.new()
width.placeholderText = "宽度"
popover:contentLayout():addWidget(width)
local popoverButton = addButton("浮层")
popoverButton:on("clicked", function()
    popover:toggle(popoverButton, shad.PlacementType.Bottom, shad.PlacementType.Start)
end)

-- 气泡确认
local deleteButton = addButton("气泡确认")
local popconfirm = shad.Popconfirm.new("删除这条记录？", "删除后无法恢复", window)
popconfirm.destructive = true
popconfirm:attach(deleteButton)
popconfirm:on("confirmed", function()
    result.text = "气泡确认：已确认"
end)
popconfirm:on("cancelled", function()
    result.text = "气泡确认：已取消"
end)

-- 抽屉
local drawer = shad.Drawer.new("通知设置", "从窗口边缘滑出，可以拖动关闭", window)
drawer:contentLayout():addWidget(shad.Switch.new("接收邮件通知"))
local closeDrawer = shad.Button.new("完成")
drawer:footerLayout():addWidget(closeDrawer)
closeDrawer:on("clicked", function()
    drawer:close()
end)
local drawerButton = addButton("抽屉")
drawerButton:on("clicked", function()
    drawer:open()
end)

-- 以下静态函数会阻塞到对话框关闭，取消时返回 false 或 nil
local confirmButton = addButton("确认对话框")
confirmButton:on("clicked", function()
    local isConfirmed = shad.AlertDialog.confirm(window, "确定要退出吗？", "未保存的修改会丢失", "退出", "取消", shad.ButtonType.Destructive)
    result.text = isConfirmed and "确认对话框：确认" or "确认对话框：取消"
end)

local inputButton = addButton("输入对话框")
inputButton:on("clicked", function()
    local name = shad.InputDialog.getText(window, "重命名", "输入新的名称", "名称", "未命名")
    result.text = name and ("输入对话框：" .. name) or "输入对话框：已取消"
end)

local colorButton = addButton("选择颜色")
colorButton:on("clicked", function()
    local color = shad.ColorDialog.getColor("#3b82f6", window)
    result.text = color and ("选择颜色：" .. color) or "选择颜色：已取消"
end)

-- 命令面板：快捷键设为空字符串表示不注册
local palette = shad.CommandDialog.new(window)
palette.shortcut = ""
local commands = palette:command()
commands:addGroup("操作")
local openSettings = commands:addItem("打开设置")
local checkUpdate = commands:addItem("检查更新")
openSettings:on("triggered", function()
    result.text = "命令面板：打开设置"
end)
checkUpdate:on("triggered", function()
    result.text = "命令面板：检查更新"
end)
local commandButton = addButton("命令面板")
commandButton:on("clicked", function()
    palette:open()
end)

-- 通知
local notifyButton = addButton("通知")
notifyButton:on("clicked", function()
    shad.Notification.success("已保存", "修改已经保存", 3000, shad.NotificationType.TopRight, window)
end)

window:show()
