-- 表单：常用输入组件，提交时校验并弹出通知
local window = shad.Window.new()
window.windowTitle = "Lua 表单"
window:resize(460, 760)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)

local card = shad.Card.new("新建任务", "填写后点击提交，名称为空时输入框会标红")
root:addWidget(card)
root:addStretch(1)
local form = card:contentLayout()

-- 在表单中添加一行
local function addField(title, widget)
    form:addWidget(shad.Label.new(title))
    form:addWidget(widget)
end

local name = shad.Input.new()
name.placeholderText = "任务名称（必填）"
addField("名称", name)

local category = shad.ComboBox.new()
category:addItems({"开发", "设计", "测试", "运维"})
addField("分类", category)

local priorityLabel = shad.Label.new("优先级：3")
local priority = shad.Slider.new()
priority.minimum = 1
priority.maximum = 5
priority.value = 3
form:addWidget(priorityLabel)
form:addWidget(priority)
priority:on("valueChanged", function(value)
    priorityLabel.text = "优先级：" .. value
end)

local hours = shad.SpinBox.new()
hours.minimum = 1
hours.maximum = 99
hours.value = 8
addField("预计工时（小时）", hours)

addField("截止日期", shad.DatePicker.new())

-- 同一个父组件中的单选按钮自动互斥
local normal = shad.RadioButton.new("普通")
local urgent = shad.RadioButton.new("加急")
normal.checked = true
local levelRow = shad.HBox.new()
levelRow:addWidget(normal)
levelRow:addWidget(urgent)
levelRow:addStretch(1)
form:addWidget(shad.Label.new("紧急程度"))
form:addLayout(levelRow)

local notify = shad.Checkbox.new("完成后通知我")
form:addWidget(notify)

local description = shad.Textarea.new()
description.placeholderText = "补充说明"
addField("说明", description)

local reset = shad.Button.new("重置")
reset.variant = shad.ButtonType.Outline
local submit = shad.Button.new("提交")
local footer = card:footerLayout()
footer:addStretch(1)
footer:addWidget(reset)
footer:addWidget(submit)

-- 通知
local toaster = shad.Toaster.new(window)

submit:on("clicked", function()
    local text = name.text
    name.invalid = (text == "")
    if text == "" then
        toaster:add("提交失败", "请填写任务名称", shad.ToastType.Error)
        return
    end
    local level = urgent.checked and "加急" or "普通"
    print(string.format("提交任务：%s，分类 %s，优先级 %d，工时 %d 小时，%s", text, category.currentText, priority.value, hours.value, level))
    toaster:add("已提交", text .. " 已加入任务列表", shad.ToastType.Success)
end)

reset:on("clicked", function()
    name.text = ""
    name.invalid = false
    description.plainText = ""
    priority.value = 3
    hours.value = 8
    notify.checked = false
    normal.checked = true
    print("表单已重置")
end)

window:show()
