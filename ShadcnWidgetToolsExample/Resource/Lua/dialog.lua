-- 对话框：确认对话框与自定义页脚
local window = shad.Window.new()
window.windowTitle = "Lua 对话框"
window:resize(460, 300)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)
root.spacing = 16

local alert = shad.Alert.new()
alert.title = "尚未操作"
alert.description = "点击下面的按钮打开确认对话框"
root:addWidget(alert)

local openButton = shad.Button.new("删除项目")
openButton.variant = shad.ButtonType.Destructive
local buttonRow = shad.HBox.new()
buttonRow:addWidget(openButton)
buttonRow:addStretch(1)
root:addLayout(buttonRow)
root:addStretch(1)

-- 对话框以窗口为父对象，随窗口一起销毁
local dialog = shad.Dialog.new("确定要删除吗？", "删除后无法恢复，项目中的全部数据都会被清除。", window)
local withFiles = shad.Checkbox.new("同时删除关联的文件")
dialog:contentLayout():addWidget(withFiles)

local cancel = shad.Button.new("取消")
cancel.variant = shad.ButtonType.Outline
local confirm = shad.Button.new("删除")
confirm.variant = shad.ButtonType.Destructive
local footer = dialog:footerLayout()
footer:addStretch(1)
footer:addWidget(cancel)
footer:addWidget(confirm)

cancel:on("clicked", function()
    dialog:reject()
end)
confirm:on("clicked", function()
    dialog:accept()
end)
openButton:on("clicked", function()
    dialog:open()
end)

dialog:on("accepted", function()
    alert.variant = shad.AlertType.Destructive
    alert.title = "项目已删除"
    alert.description = withFiles.checked and "关联的文件也已删除" or "关联的文件已保留"
    print("对话框：确认删除，删除关联文件 = " .. tostring(withFiles.checked))
end)
dialog:on("rejected", function()
    alert.variant = shad.AlertType.Default
    alert.title = "已取消"
    alert.description = "项目没有任何变化"
    print("对话框：已取消")
end)

window:show()
