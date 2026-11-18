-- 标签输入、日期时间选择、日志视图与加载遮罩
local window = shad.Window.new()
window.windowTitle = "Lua 发布记录"
window:resize(640, 620)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)
root.spacing = 12

local card = shad.Card.new("新建发布", "填写标签与发布时间后点击发布，结果写入下方的日志")
root:addWidget(card)
local content = card:contentLayout()

-- 标签输入
local tags = shad.TagInput.new()
tags.placeholderText = "输入标签后按回车"
tags.maxTags = 5
tags:addTag("stable")
tags:addTag("macOS")
content:addWidget(tags)

-- 日期时间选择
local schedule = shad.DateTimePicker.new()
schedule.secondsVisible = false
schedule.calendarIconVisible = true
schedule.placeholderText = "选择发布时间"
content:addWidget(schedule)

local publish = shad.Button.new("发布")
content:addWidget(publish)

-- 加载遮罩盖在卡片上
local overlay = shad.LoadingOverlay.new(card)
overlay.text = "正在发布..."

-- 日志视图
local log = shad.LogView.new()
log.placeholderText = "暂无日志"
log.maxEntryCount = 200
root:addWidget(log)

tags:on("tagAdded", function(tag)
    log:debug("添加标签：" .. tag)
end)
tags:on("tagRemoved", function(tag)
    log:debug("移除标签：" .. tag)
end)
tags:on("tagRejected", function(tag)
    log:warning("标签被拒绝：" .. tag)
end)
schedule:on("dateTimeChanged", function()
    log:info("发布时间：" .. schedule:getDisplayText())
end)

local timer = shad.Timer.new()
timer.interval = 1500
timer.singleShot = true
timer:on("timeout", function()
    overlay:stop()
    log:success("发布完成，共 " .. tags:count() .. " 个标签")
end)

publish:on("clicked", function()
    if tags:count() == 0 then
        log:error("至少需要一个标签")
        return
    end
    if schedule:getDisplayText() == "" then
        log:error("请选择发布时间")
        return
    end
    log:info("开始发布，计划时间 " .. schedule:getDisplayText())
    overlay:start()
    timer:start()
end)

window:show()
