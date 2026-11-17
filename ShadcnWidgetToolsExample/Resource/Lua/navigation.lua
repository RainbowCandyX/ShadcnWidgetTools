-- 页内导航：锚点目录与回到顶部
local window = shad.Window.new()
window.windowTitle = "Lua 页内导航"
window:resize(760, 560)

local root = shad.HBox.new(window)
root:setContentsMargins(24, 16, 24, 24)
root.spacing = 24

-- 左侧目录，右侧可滚动的文档
local anchor = shad.Anchor.new()
anchor:setFixedWidth(180)
root:addWidget(anchor, 0, shad.Qt.AlignTop)

local area = shad.ScrollArea.new()
root:addWidget(area, 1)

local document = shad.Widget.new()
local documentLayout = shad.VBox.new(document)
documentLayout:setContentsMargins(0, 0, 16, 0)
documentLayout.spacing = 12

-- 标题与级别，级别 1 的链接在目录中缩进
local headings = {
    {"快速开始", 0},
    {"安装", 1},
    {"第一个窗口", 1},
    {"组件", 0},
    {"按钮", 1},
    {"表单", 1},
    {"主题", 0},
    {"常见问题", 0},
}

for _, heading in ipairs(headings) do
    local title = shad.Label.new(heading[1])
    title.colorRole = shad.ThemeType.Foreground
    documentLayout:addWidget(title)
    anchor:addLink(heading[1], title, heading[2])
    for paragraph = 1, 3 do
        local text = shad.Label.new("这是「" .. heading[1] .. "」的第 " .. paragraph .. " 段占位文字，用于撑开内容高度，便于观察滚动时目录中当前链接的变化。")
        text.wordWrap = true
        text.colorRole = shad.ThemeType.MutedForeground
        documentLayout:addWidget(text)
    end
end
documentLayout:addStretch(1)
area:setWidget(document)

-- 目录绑定滚动区域，区块顶部与可视区顶部保留 8px
anchor:setScrollArea(area)
anchor.offset = 8

-- 向下滚动 200px 后右下角出现回到顶部按钮
local backTop = shad.BackTop.new(area)
backTop.visibilityHeight = 200
backTop.offset = 16

anchor:on("currentIndexChanged", function(index)
    if index >= 0 then
        window.windowTitle = "Lua 页内导航 - " .. anchor:getLinkTitle(index)
    end
end)

window:show()
