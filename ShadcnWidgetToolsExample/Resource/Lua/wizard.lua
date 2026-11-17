-- 分步向导：步骤条、自动补全、穿梭框与时间轴
local window = shad.Window.new()
window.windowTitle = "Lua 分步向导"
window:resize(640, 620)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)
root.spacing = 16

-- 步骤条，可以点击已经走过的步骤返回
local steps = shad.Steps.new()
steps:addStep("项目", "名称与负责人")
steps:addStep("成员", "选择参与的成员")
steps:addStep("确认", "检查操作记录")
steps.clickable = true
root:addWidget(steps)

local pages = shad.StackedWidget.new()
root:addWidget(pages, 1)

-- 第一步：自动补全
local projectPage = shad.Widget.new()
local projectLayout = shad.VBox.new(projectPage)
projectLayout:setContentsMargins(0, 0, 0, 0)

projectLayout:addWidget(shad.Label.new("项目模板（模糊匹配）"))
local template = shad.AutoComplete.new()
template.placeholderText = "输入模板名称，例如 web"
template.options = {"Web 应用", "桌面应用", "命令行工具", "移动应用", "数据看板", "后台服务", "浏览器扩展"}
projectLayout:addWidget(template)

projectLayout:addWidget(shad.Label.new("负责人邮箱（按输入生成选项）"))
local owner = shad.AutoComplete.new()
owner.placeholderText = "输入用户名"
owner.filterMode = shad.AutoCompleteType.NoFilter
projectLayout:addWidget(owner)
projectLayout:addStretch(1)

-- 根据 @ 之前的用户名与之后已输入的部分补全域名
owner:on("searchTextChanged", function(text)
    local name, domain = text:match("^([^@]*)@?(.*)$")
    local options = {}
    if name ~= "" then
        for _, candidate in ipairs({"gmail.com", "outlook.com", "qq.com", "163.com"}) do
            if candidate:sub(1, #domain) == domain then
                options[#options + 1] = name .. "@" .. candidate
            end
        end
    end
    owner.options = options
end)
pages:addWidget(projectPage)

-- 第二步：穿梭框
local memberPage = shad.Widget.new()
local memberLayout = shad.VBox.new(memberPage)
memberLayout:setContentsMargins(0, 0, 0, 0)
local transfer = shad.Transfer.new()
transfer.searchEnabled = true
transfer:setTitles("全部成员", "项目成员")
local members = {}
for index = 1, 12 do
    members[#members + 1] = {key = tostring(index), title = "成员 " .. index, description = "member" .. index .. "@example.com", disabled = index % 5 == 0}
end
transfer:setItems(members)
memberLayout:addWidget(transfer, 1)
pages:addWidget(memberPage)

-- 第三步：时间轴记录前两步的操作
local confirmPage = shad.Widget.new()
local confirmLayout = shad.VBox.new(confirmPage)
confirmLayout:setContentsMargins(0, 0, 0, 0)
local timeline = shad.Timeline.new()
timeline.pending = "等待创建项目..."
confirmLayout:addWidget(timeline)
confirmLayout:addStretch(1)
pages:addWidget(confirmPage)

-- 默认开放的标准库不含 os，时间一栏显示操作序号
local logCount = 0

local function log(title, description)
    logCount = logCount + 1
    return timeline:addItem(title, "第 " .. logCount .. " 项操作", description)
end

template:on("optionSelected", function(text)
    log("选择模板", text)
end)

owner:on("optionSelected", function(text)
    log("设置负责人", text)
end)

transfer:on("targetKeysChanged", function(keys, direction, moved)
    local action = direction == shad.TransferType.ToTarget and "添加成员" or "移除成员"
    log(action, #moved .. " 人，当前共 " .. #keys .. " 人")
end)

-- 底部按钮
local buttons = shad.Widget.new()
local buttonLayout = shad.HBox.new(buttons)
buttonLayout:setContentsMargins(0, 0, 0, 0)
buttonLayout:addStretch(1)
local previous = shad.Button.new("上一步")
previous.variant = shad.ButtonType.Outline
local nextButton = shad.Button.new("下一步")
buttonLayout:addWidget(previous)
buttonLayout:addWidget(nextButton)
root:addWidget(buttons)

local function updatePage(index)
    pages.currentIndex = index
    previous.enabled = index > 0
    nextButton.text = index == steps:count() - 1 and "创建项目" or "下一步"
end

steps:on("currentIndexChanged", updatePage)

previous:on("clicked", function()
    steps.status = shad.StepsType.Process
    steps:previous()
end)

nextButton:on("clicked", function()
    if steps.currentIndex < steps:count() - 1 then
        steps:next()
        return
    end
    -- 最后一步：完成全部步骤，把进行中节点换成完成的节点
    steps.status = shad.StepsType.Finish
    timeline.pending = ""
    local done = log("项目已创建")
    done:setIcon(0xE226) -- circle-check
    done.color = shad.ThemeType.Chart2
    nextButton.enabled = false
end)

updatePage(0)
window:show()
