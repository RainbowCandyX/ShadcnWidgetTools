-- 定时器窗口：卸载脚本后定时器随之停止
local window = shad.Window.new()
window.windowTitle = "Lua 定时器"
window:resize(320, 200)

local layout = shad.VBox.new(window)
layout:setContentsMargins(24, 16, 24, 24)
layout.spacing = 12

local label = shad.Label.new("已运行 0 秒")
local progress = shad.Progress.new()
layout:addWidget(label)
layout:addWidget(progress)
layout:addStretch(1)

local seconds = 0
local timer = shad.Timer.new()
timer.interval = 1000
timer:on("timeout", function()
    seconds = seconds + 1
    label.text = "已运行 " .. seconds .. " 秒"
    progress.value = (seconds * 10) % 110
    if seconds % 5 == 0 then
        print("已运行 " .. seconds .. " 秒")
    end
end)
timer:start()

window:show()
