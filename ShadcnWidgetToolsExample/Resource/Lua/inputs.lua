-- 输入与表单组件
local window = shad.Window.new()
window.windowTitle = "Lua 输入与表单"
window:resize(520, 780)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)

-- 内容放进滚动区域
local scroll = shad.ScrollArea.new()
root:addWidget(scroll)
local content = shad.Widget.new()
local contentLayout = shad.VBox.new(content)
scroll:setWidget(content)

-- 表单分组
local fieldSet = shad.FieldSet.new("账户设置", "每一行是一个表单项：标签、组件与说明")
local group = shad.FieldGroup.new()
fieldSet:addWidget(group)
local submit = shad.Button.new("提交")
contentLayout:addWidget(fieldSet)
contentLayout:addWidget(submit)
contentLayout:addStretch(1)

-- 输入组
local site = shad.InputGroup.new()
site:addText(shad.InputGroupType.InlineStart, "https://")
site:getInput().placeholderText = "example.com"
local siteField = shad.Field.new("网站", site)
siteField.description = "只填写域名"
group:addWidget(siteField)

-- 时间
local time = shad.TimePicker.new()
time.secondsVisible = false
time.time = "09:30"
group:addWidget(shad.Field.new("提醒时间", time))

-- 验证码
local code = shad.InputOtp.new()
code:setGroups({3, 3})
code:setPattern("^[0-9]*$")
group:addWidget(shad.Field.new("验证码", code))
code:on("completed", function(value)
    print("验证码已填满：" .. value)
end)

group:addSeparator("偏好")

local align = shad.ToggleGroup.new(shad.ToggleGroupType.Single)
align:addItem("左对齐")
align:addItem("居中")
align:addItem("右对齐")
align.currentIndex = 1
group:addWidget(shad.Field.new("对齐方式", align))

local plan = shad.RadioGroup.new(shad.Qt.Horizontal)
plan:addItems({"免费版", "专业版", "企业版"})
plan.currentIndex = 0
group:addWidget(shad.Field.new("套餐", plan))

-- 树形下拉
local region = shad.TreeSelect.new()
local asia = region:addItem("亚洲")
region:addItem("上海", asia)
region:addItem("东京", asia)
local europe = region:addItem("欧洲")
region:addItem("柏林", europe)
region.placeholderText = "选择地区"
region.fullPathVisible = true
group:addWidget(shad.Field.new("地区", region))

-- 快捷键
local shortcut = shad.KeySequenceEdit.new("Ctrl+Shift+K")
group:addWidget(shad.Field.new("快捷键", shortcut))

submit:on("clicked", function()
    local domain = site:getInput().text
    siteField.invalid = (domain == "")
    if domain == "" then
        siteField:setError("请填写网站域名")
        return
    end
    siteField:clearErrors()
    print("网站：https://" .. domain)
    print("提醒时间：" .. tostring(time.time) .. "，验证码：" .. code.value)
    print("对齐方式：第 " .. align.currentIndex .. " 项，套餐：第 " .. plan.currentIndex .. " 项")
    print("地区：" .. table.concat(region:currentPath(), " / ") .. "，快捷键：" .. tostring(shortcut.keySequence))
end)

window:show()
