-- 共享与隔离状态的区别
load_count = (load_count or 0) + 1
print("全局变量 load_count = " .. load_count)

local window = shad.Window.new()
window.windowTitle = "Lua 共享状态"
window:resize(360, 200)

local layout = shad.VBox.new(window)
layout:setContentsMargins(24, 16, 24, 24)
layout.spacing = 12

local label = shad.Label.new("这是第 " .. load_count .. " 次加载")
local hint = shad.Label.new("打开共享状态后重新加载，计数会继续增加")
hint.colorRole = shad.ThemeType.MutedForeground
hint.wordWrap = true
layout:addWidget(label)
layout:addWidget(hint)
layout:addStretch(1)

window:show()
