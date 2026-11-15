-- 计数器窗口：按钮计数与输入回显
print("计数器脚本开始加载")

local window = shad.Window.new()
window.windowTitle = "Lua 计数器"
window:resize(360, 280)

local layout = shad.VBox.new(window)
layout:setContentsMargins(24, 16, 24, 24)
layout.spacing = 12

local count = 0
local label = shad.Label.new("点击次数：0")
local button = shad.Button.new("点击 +1")
local input = shad.Input.new()
input.placeholderText = "输入文字"
local echo = shad.Label.new("")
echo.colorRole = shad.ThemeType.MutedForeground

layout:addWidget(label)
layout:addWidget(button)
layout:addWidget(input)
layout:addWidget(echo)
layout:addStretch(1)

button:on("clicked", function()
    count = count + 1
    label.text = "点击次数：" .. count
    print("按钮被点击，当前次数：" .. count)
end)

input:on("textChanged", function(text)
    echo.text = "你输入了：" .. text
end)

window:show()
