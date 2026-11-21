-- 展示组件
local window = shad.Window.new()
window.windowTitle = "Lua 展示组件"
window:resize(640, 760)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)

-- 内容放进滚动区域
local scroll = shad.ScrollArea.new()
root:addWidget(scroll)
local content = shad.Widget.new()
local layout = shad.VBox.new(content)
layout.spacing = 16
scroll:setWidget(content)

-- 面包屑
local breadcrumb = shad.Breadcrumb.new()
breadcrumb:setPath({"首页", "组件", "展示"})
layout:addWidget(breadcrumb)
breadcrumb:on("linkClicked", function(index)
    print("面包屑：点击了第 " .. index .. " 项 " .. breadcrumb:getItemText(index))
end)

-- 描述列表
local details = shad.Descriptions.new("版本信息")
details.columnCount = 2
details.bordered = true
details:addItem("名称", "ShadcnWidgetTools")
details:addItem("版本", "1.3.2")
details:addItem("许可证", "MIT")
details:addItem("状态", shad.Badge.new("稳定"))
layout:addWidget(details)

-- 列表项
local group = shad.ItemGroup.new()
local review = shad.Item.new("设计评审", "周三下午三点，会议室 A")
review.clickable = true
local open = shad.Button.new("查看")
open.variant = shad.ButtonType.Outline
review:addActionWidget(open)
group:addItem(review)
group:addSeparator()
local verify = shad.Item.new("两步验证", "登录时需要输入验证码")
verify:addActionWidget(shad.Switch.new())
group:addItem(verify)
layout:addWidget(group)
review:on("clicked", function()
    print("列表项：设计评审")
end)
open:on("clicked", function()
    print("列表项：查看")
end)

-- 头像组与按键
local row = shad.HBox.new()
local avatars = shad.AvatarGroup.new()
avatars:addAvatar(shad.Avatar.new("张"))
avatars:addAvatar(shad.Avatar.new("李"))
avatars:addAvatar(shad.Avatar.new("王"))
avatars.countText = "+5"
row:addWidget(avatars)
row:addStretch(1)
row:addWidget(shad.Label.new("打开命令面板"))
row:addWidget(shad.KbdGroup.new({"Ctrl", "K"}))
layout:addLayout(row)

-- 附件
local attachments = shad.AttachmentGroup.new()
attachments:addAttachment(shad.Attachment.new("季度报告.pdf", "PDF，2.4 MB"))
attachments:addAttachment(shad.Attachment.new("会议纪要.docx", "Word，86 KB"))
attachments:addAttachment(shad.Attachment.new("预算表.xlsx", "Excel，310 KB"))
layout:addWidget(attachments)

local markdown = shad.MarkdownView.new()
markdown.markdown = "### Markdown 视图\n\n支持 **加粗**、`行内代码` 与列表：\n\n- 第一项\n- 第二项"
layout:addWidget(markdown)

-- 虚拟列表：数据来自 shad.Model
local model = shad.Model.new()
model:addRow({"构建发布包"})
model:addRow({"运行单元测试"})
model:addRow({"更新文档"})
local list = shad.ListView.new()
list:setModel(model)
list:setItemDescription(0, "打包三个平台的安装包")
list:setItemTrailingText(0, "已完成")
list:setItemTrailingColor(0, "#22c55e")
list:setItemDescription(1, "共 128 个用例")
list:setItemTrailingText(1, "失败")
list:setItemTrailingColor(1, shad.ThemeType.Destructive)
list:setItemDescription(2, "README 与示例脚本")
list:setItemTrailingText(2, "等待中")
list:setItemChecked(2, true)
list:setFixedHeight(200)
layout:addWidget(list)
list:on("clicked", function()
    print("虚拟列表：第 " .. list:currentRow() .. " 行 " .. list:currentText())
end)

-- 空状态
local empty = shad.Empty.new("没有更多内容", "列表为空时显示这样的占位内容")
empty.borderVisible = true
local reload = shad.Button.new("重新加载")
empty:contentLayout():addWidget(reload)
layout:addWidget(empty)
reload:on("clicked", function()
    print("空状态：重新加载")
end)

-- 结果页
local result = shad.Result.new(shad.ResultType.Success, "提交成功", "预计 1 到 2 个工作日内完成审核")
local back = result:addButton("返回列表")
result:addButton("查看详情", shad.ButtonType.Outline)
layout:addWidget(result)
back:on("clicked", function()
    print("结果页：返回列表")
end)

layout:addStretch(1)
window:show()
