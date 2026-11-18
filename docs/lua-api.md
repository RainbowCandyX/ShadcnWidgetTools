# Lua 接口

`ShadcnWidgetToolsLua` 提供给脚本的全部接口。编译与 C++ 侧的用法见 [README](../README.md#用-lua-创建组件)。

本文的写法：方括号中的参数可以省略，`->` 后面是返回值，序号、行号与列号都从 0 开始。

## 目录

- [通用规则](#通用规则)
- [全局对象与功能模块](#全局对象与功能模块)
- [基础对象](#基础对象)
- [窗口与对话框](#窗口与对话框)
- [只有构造函数的组件](#只有构造函数的组件)
- [按钮与反馈](#按钮与反馈)
- [输入](#输入)
- [表单](#表单)
- [数据展示](#数据展示)
- [图表](#图表)
- [数据模型与表格](#数据模型与表格)
- [布局与容器](#布局与容器)
- [弹层与菜单](#弹层与菜单)

## 通用规则

### 属性、信号与槽

每个对象的 Qt 属性、信号与槽都自动可用，不需要逐个注册，本文只列出属性之外的方法。属性名与信号名以组件头文件中的 `Q_PROPERTY` 与 `Q_SIGNALS` 为准。

```lua
local button = shad.Button.new("保存")
button.variant = shad.ButtonType.Outline        -- 读写属性
button:setEnabled(false)                        -- 也可以用 setXxx / getXxx / isXxx
button:on("clicked", function() print("点击") end)  -- 连接信号
button:click()                                  -- 调用槽
```

### 所有对象都有的方法

```
object:on(signalName, callback)   连接信号，回调的参数为信号的参数
object:delete()                   删除对象
object:className()   -> string    C++ 类名
object:equals(other) -> boolean   两个句柄是否指向同一个对象，不能用 == 比较
object:isValid()     -> boolean   对象是否还存在
```

对象被 Qt 销毁后句柄失效，读属性得到 `nil`，调用方法会报 Lua 错误。由脚本创建且没有父对象的组件，在句柄被回收时删除；加入布局或设置父对象后由 Qt 管理。

### 构造函数

组件写作 `shad.<名称>.new(...)`，最后一个参数都是可以省略的父对象。带文字、方向或模式的构造函数，这个参数也可以省略：

```lua
shad.Label.new("文字")
shad.Label.new("文字", parent)
shad.RadioGroup.new(shad.Qt.Horizontal)
shad.RadioGroup.new()
```

### 值的写法

| 类型 | 写法 |
| --- | --- |
| 枚举 | `shad.<名称>` 下的数值，名称为去掉 `Shad` 前缀的枚举命名空间，例如 `shad.ButtonType.Outline`、`shad.Qt.AlignCenter` |
| 标志位 | 枚举值相加，例如 `shad.WindowType.AllButtons - shad.WindowType.ThemeButton` |
| 颜色 | 字符串，例如 `"#3b82f6"`、`"#803b82f6"`、`"red"` |
| 图标 | `ShadIconType::IconName` 的数值，`0` 表示不显示 |
| 日期与时间 | ISO 格式的字符串，例如 `"2026-01-31"`、`"09:30"`、`"09:30:00"` |
| 快捷键 | 字符串，例如 `"Ctrl+Shift+K"` |
| 尺寸、坐标、矩形 | 写入用数组 `{宽, 高}`、`{x, y}`、`{x, y, 宽, 高}`；读取得到带 `width`、`height`、`x`、`y` 字段的表 |
| 字符串列表、整数列表 | Lua 数组 |

## 全局对象与功能模块

```
shad.theme                         主题管理器，属性见 ShadTheme，例如 shad.theme.themeMode = shad.ThemeType.Dark
shad.theme:isDark() -> boolean
shad.runtime                       运行时名称，例如 "LuaJIT 2.1"
print(...)                         输出交给宿主程序
```

### shad.json

```
shad.json.decode(text)            -> value            失败时返回 nil 与错误说明
shad.json.encode(value, [pretty]) -> string
```

### shad.xml

```
shad.xml.decode(text) -> node     失败时返回 nil 与错误说明
```

节点为 `{name = ..., attributes = {...}, text = ..., children = {节点, ...}}`。

### shad.http

请求是异步的，只允许 `http` 与 `https` 地址。引擎没有开放网络时没有这个模块。

```
shad.http.get(url, [options], callback)
shad.http.head(url, [options], callback)
shad.http.delete(url, [options], callback)
shad.http.post(url, body, [options], callback)
shad.http.put(url, body, [options], callback)
shad.http.patch(url, body, [options], callback)
shad.http.request(options, callback)
```

- `options`：`headers`（表）、`query`（表，拼到地址后面）、`timeout`（毫秒）。
- `shad.http.request` 的 `options` 另有 `url`、`method`（任意只含字母的方法名，省略为 `GET`）与 `body`。
- `body`：字符串原样发送，表按 JSON 发送，`nil` 表示没有正文。有正文但没有指定 `Content-Type` 时为 `application/json`。
- 回调的参数为 `{ok, status, body, headers, url, error}`，`ok` 表示状态码在 200 到 299 之间。

## 基础对象

### Widget

`shad.Widget.new([parent])` 创建空白组件。以下方法对所有组件都可用：

```
resize(width, height)
move(x, y)
setFixedSize(width, height)
setFixedWidth(width)
setFixedHeight(height)
setMinimumSize(width, height)
setMaximumSize(width, height)
setContentsMargins(left, top, right, bottom)
setParent(parent)
setLayout(layout)
adjustSize()
```

`show()`、`hide()`、`close()`、`setFocus()` 等是 QWidget 的槽，可以直接调用；`windowTitle`、`toolTip`、`enabled`、`visible` 等是属性。

### 布局

```
shad.VBox.new([parent])
shad.HBox.new([parent])
shad.Grid.new([parent])
shad.FlowLayout.new([parent])
```

传入组件作为 `parent` 时，布局直接设置给这个组件。

```
所有布局
  setContentsMargins(left, top, right, bottom)
  addWidget(widget)
  removeWidget(widget)
  spacing                              属性（FlowLayout 除外）

VBox、HBox
  addWidget(widget, [stretch], [alignment])
  addLayout(layout, [stretch])
  addStretch([stretch])
  addSpacing(size)

Grid
  addWidget(widget, row, column, [rowSpan], [columnSpan], [alignment])
  addLayout(layout, row, column, [rowSpan], [columnSpan])

FlowLayout
  setHorizontalSpacing(spacing)  getHorizontalSpacing() -> number
  setVerticalSpacing(spacing)    getVerticalSpacing()   -> number
```

`alignment` 取 `shad.Qt` 中的对齐值。

### Timer

`shad.Timer.new([parent])`。属性 `interval`、`singleShot`，槽 `start()`、`stop()`，信号 `timeout`。

### ActionGroup

`shad.ActionGroup.new([parent])`，用于让菜单的单选项互斥。

### QAction

菜单项、命令面板的项与工具栏按钮都是 `QAction`，由对应的 `addItem`、`addAction` 等方法返回。属性 `text`、`checked`、`enabled`、`visible`，信号 `triggered`、`toggled`。

## 窗口与对话框

### Window

`shad.Window.new([parent])`，可以直接设置布局的独立窗口，标题用 `windowTitle` 属性。

```
setButtonFlags(flags)
getButtonFlags() -> number
```

`flags` 为 `shad.WindowType` 的标志位：`MinimizeButton`、`MaximizeButton`、`CloseButton`、`StayOnTopButton`、`ThemeButton`、`AllButtons`。窗口默认显示全部按钮。

### MainWindow

`shad.MainWindow.new([parent])`，对应 `ShadWindow`。内容用 `setCentralWidget` 设置，不能再设置布局。

```
setCentralWidget(widget)             getCentralWidget()         -> widget
setTitleBarWidget(area, widget)      getTitleBarWidget(area)    -> widget
setButtonFlags(flags)                getButtonFlags()           -> number
```

`area` 取 `shad.WindowType.LeftArea` 或 `RightArea`。

### Dialog

`shad.Dialog.new([title, description], [parent])`。`open()`、`exec()`、`accept()`、`reject()` 是槽，结果通过 `accepted`、`rejected`、`finished` 信号得到。对话框默认只有关闭按钮。

```
contentLayout() -> layout
footerLayout()  -> layout
setButtonFlags(flags)
getButtonFlags() -> number
```

### AlertDialog

`shad.AlertDialog.new([title, description], [parent])`，继承 Dialog。

```
setMediaIcon(icon)      getMediaIcon() -> icon
setMediaImagePath(path)
actionButton() -> Button
cancelButton() -> Button

shad.AlertDialog.confirm(parent, title, description, [actionText], [cancelText], [actionVariant]) -> boolean
```

`confirm` 会阻塞到对话框关闭。

### InputDialog

`shad.InputDialog.new([parent])`，继承 Dialog。输入的值用 `textValue`、`intValue`、`doubleValue` 属性读写。

```
setIntRange(minimum, maximum)       getIntMinimum() / getIntMaximum()       -> number
setIntStep(step)                    getIntStep()                            -> number
setDoubleRange(minimum, maximum)    getDoubleMinimum() / getDoubleMaximum() -> number
setDoubleDecimals(decimals)         getDoubleDecimals()                     -> number
setDoubleStep(step)                 getDoubleStep()                         -> number

shad.InputDialog.getText(parent, title, description, label, [text], [echoMode])                              -> string
shad.InputDialog.getMultiLineText(parent, title, description, label, [text])                                 -> string
shad.InputDialog.getInt(parent, title, description, label, [value], [minimum], [maximum], [step])            -> number
shad.InputDialog.getDouble(parent, title, description, label, [value], [minimum], [maximum], [decimals], [step]) -> number
```

四个静态函数会阻塞到对话框关闭，取消时返回 `nil`。

### ColorDialog

`shad.ColorDialog.new([initialColor], [parent])`，继承 Dialog。当前颜色用 `currentColor` 属性读写。

```
colorPicker()  -> ColorPicker
okButton()     -> Button
cancelButton() -> Button

shad.ColorDialog.getColor([initialColor], [parent], [title]) -> string
```

`getColor` 会阻塞到对话框关闭，取消时返回 `nil`。

### CommandDialog

`shad.CommandDialog.new([parent])`，继承 Dialog。快捷键用 `shortcut` 属性读写，创建时会注册程序内的全局快捷键，设为空字符串表示不注册。

```
command() -> Command

shad.CommandDialog.setDefaultShortcut(sequence)
shad.CommandDialog.getDefaultShortcut() -> string
```

## 只有构造函数的组件

这些组件只通过属性、信号与槽使用。

| 名称 | 构造函数 |
| --- | --- |
| Label | `shad.Label.new([text], [parent])` |
| Input | `shad.Input.new([parent])` |
| Textarea | `shad.Textarea.new([parent])` |
| Checkbox | `shad.Checkbox.new([text], [parent])` |
| Switch | `shad.Switch.new([text], [parent])` |
| RadioButton | `shad.RadioButton.new([text], [parent])` |
| Slider | `shad.Slider.new([orientation], [parent])` |
| Progress | `shad.Progress.new([parent])` |
| Separator | `shad.Separator.new([orientation], [parent])` |
| Skeleton | `shad.Skeleton.new([parent])` |
| Marquee | `shad.Marquee.new([text], [parent])` |
| SpinBox | `shad.SpinBox.new([parent])` |
| DoubleSpinBox | `shad.DoubleSpinBox.new([parent])` |
| Rating | `shad.Rating.new([parent])` |
| RadialProgress | `shad.RadialProgress.new([parent])` |
| Statistic | `shad.Statistic.new([text], [parent])` |
| QrCode | `shad.QrCode.new([text], [parent])` |
| Avatar | `shad.Avatar.new([fallbackText], [parent])` |
| Calendar | `shad.Calendar.new([parent])` |
| DatePicker | `shad.DatePicker.new([parent])` |
| Pagination | `shad.Pagination.new([parent])` |
| CodeEditor | `shad.CodeEditor.new([code], [parent])` |
| Kbd | `shad.Kbd.new([text], [parent])` |
| FieldSeparator | `shad.FieldSeparator.new([text], [parent])` |
| ScrollBar | `shad.ScrollBar.new([orientation], [parent])` |
| NavigationMenuLink | `shad.NavigationMenuLink.new([title, description], [parent])` |
| TreeView | `shad.TreeView.new([parent])` |
| LineChart | `shad.LineChart.new([parent])` |
| AreaChart | `shad.AreaChart.new([parent])` |
| BarChart | `shad.BarChart.new([parent])` |

`ComboBox` 另有下面几个方法：

```
shad.ComboBox.new([parent])
addItem(text)
addItems(texts)
itemText(index) -> string
removeItem(index)
```

## 按钮与反馈

```
Button          shad.Button.new([text], [parent])
  setShadIcon(icon)

Badge           shad.Badge.new([text], [parent])
  setShadIcon(icon)

Spinner         shad.Spinner.new([parent])
  setShadIcon(icon)

Alert           shad.Alert.new([parent])
  setShadIcon(icon)

Card            shad.Card.new([title, description], [parent])
  contentLayout() -> layout
  footerLayout()  -> layout

Toaster         shad.Toaster.new(host)
  add(title, [description], [type]) -> id        type 取 shad.ToastType
  dismiss(id)
  dismissAll()

Segmented       shad.Segmented.new([parent])
  addItem(text) -> index
  removeItem(index)
  setItemText(index, text)

Accordion       shad.Accordion.new([parent])
  addItem(value, title, content)                 content 为组件时返回是否成功，为文字时返回显示文字的标签
  removeItem(value)
  setItemExpanded(value, expanded)

Tabs            shad.Tabs.new([parent])
  addTab(text, [page]) -> index
  removeTab(index)
  count() -> number
  widget(index) -> widget
  setTabText(index, text)
```

### Notification

没有构造函数，`position` 取 `shad.NotificationType`，`duration` 为毫秒。

```
shad.Notification.info(title, text, [duration], [position], [parent])
shad.Notification.success(title, text, [duration], [position], [parent])
shad.Notification.warning(title, text, [duration], [position], [parent])
shad.Notification.error(title, text, [duration], [position], [parent])
shad.Notification.closeAll([parent])
```

### Tooltip

没有构造函数。提示文字用组件的 `toolTip` 属性设置。

```
shad.Tooltip.install()
shad.Tooltip.uninstall()
shad.Tooltip.isInstalled() -> boolean
shad.Tooltip.setShowDelay(milliseconds)
shad.Tooltip.setShowDelay(widget, milliseconds)
shad.Tooltip.getShowDelay([widget]) -> number
shad.Tooltip.setSide(widget, side)               side 取 shad.PlacementType
shad.Tooltip.getSide(widget) -> number
shad.Tooltip.setShortcut(widget, keys)           keys 为按键文字的数组或快捷键字符串，只用于显示
shad.Tooltip.getShortcut(widget) -> 数组
shad.Tooltip.showText(widget, {x, y, 宽, 高}, text)
shad.Tooltip.hideText()
```

## 输入

```
TimePicker      shad.TimePicker.new([parent])
  clear()
  getDisplayText() -> string
  showPopup()  hidePopup()  isPopupVisible() -> boolean

DateTimePicker  shad.DateTimePicker.new([parent])                dateTime、secondsVisible、nowButtonVisible、placeholderText、displayFormat、calendarIconVisible、chevronVisible 是属性
  getDisplayText() -> string
  calendar() -> Calendar                                         浮层中的日历，可设置 minimumDate、maximumDate
  clear()  showPopup()  hidePopup()  isPopupVisible() -> boolean

InputOtp        shad.InputOtp.new([parent])
  setGroups(groups)        getGroups() -> 数组       每组的格子数，例如 {3, 3}
  getMaxLength() -> number
  clear()
  setPattern(pattern)      getPattern() -> string    整个值必须匹配的正则表达式

InputGroup      shad.InputGroup.new([control], [parent])         control 与 position 取 shad.InputGroupType
  getInput()    -> Input
  getTextarea() -> Textarea
  addText(position, text) -> Label
  addIcon(position, icon) -> widget
  addButton(position, text, [icon], [size]) -> Button
  addWidget(position, widget)
  addStretch(position)

KeySequenceEdit shad.KeySequenceEdit.new([sequence], [parent])
  clear()
  isRecording() -> boolean

ColorPicker     shad.ColorPicker.new([parent])
  setPresetColors(colors)  getPresetColors() -> 数组
  setCustomColors(colors)  getCustomColors() -> 数组  空字符串表示空槽
  pickScreenColor()
  isScreenPickingSupported() -> boolean

Toggle          shad.Toggle.new([text], [parent]) 或 shad.Toggle.new(icon, [parent])
  setShadIcon(icon)        getShadIcon() -> icon

ToggleGroup     shad.ToggleGroup.new([selectionMode], [parent])  selectionMode 取 shad.ToggleGroupType
  addItem(text) 或 addItem(icon, [text]) -> Toggle
  itemAt(index) -> Toggle
  count() -> number
  setItemChecked(index, checked)   isItemChecked(index) -> boolean
  getCheckedIndexes() -> 数组

RadioGroup      shad.RadioGroup.new([orientation], [parent])
  addItem(text) -> RadioButton
  addItems(texts)
  itemAt(index) -> RadioButton
  count() -> number

ButtonGroup     shad.ButtonGroup.new([orientation], [parent])
  addWidget(widget)
  insertWidget(index, widget)
  removeWidget(widget)
  addSeparator()
  addText(text, [icon]) -> ButtonGroupText
  count() -> number
  widgetAt(index) -> widget

ButtonGroupText shad.ButtonGroupText.new([text], [parent])
  setShadIcon(icon)        getShadIcon() -> icon

FileDropZone    shad.FileDropZone.new([parent])
  setShadIcon(icon)        getShadIcon() -> icon

SignaturePad    shad.SignaturePad.new([parent])
  isEmpty() -> boolean
  strokeCount() -> number
  toPngBase64([trimmed]) -> string                   没有笔迹时返回 nil

AutoComplete    shad.AutoComplete.new([parent])                  继承 Input；options、filterMode、maxVisibleItems 是属性
  setModel(model)          model() -> Model                      传 nil 恢复用 options 设置的选项
  showPopup()  hidePopup()  isPopupVisible() -> boolean
```

`filterMode` 取 `shad.AutoCompleteType`。用户输入时发出 `searchTextChanged(text)`，选中建议时发出 `optionSelected(text, row)`。远程搜索把 `filterMode` 设为 `NoFilter`，在 `searchTextChanged` 中取得结果后写入 `options`。

### TagInput

`shad.TagInput.new([parent])`。`tags`、`placeholderText`、`separators`、`maxTags`、`duplicatesAllowed`、`readOnly`、`invalid` 是属性，`clear()` 是槽。标签变化时发出 `tagsChanged(tags)`、`tagAdded(tag)`、`tagRemoved(tag)`，重复或超出上限时发出 `tagRejected(tag)`。

```
addTag(tag) -> boolean
removeTag(tag) -> boolean
removeTagAt(index)
tagAt(index) -> string
count() -> number
lineEdit() -> widget                     内部输入框
```

### Transfer

`shad.Transfer.new([parent])`。`targetKeys`、`searchEnabled` 是属性，`moveToTarget()`、`moveToSource()` 是槽；移动后发出 `targetKeysChanged(targetKeys, direction, movedKeys)`，`direction` 取 `shad.TransferType`。每一项写作 `{key = "1", title = "标题", description = "描述", icon = 0, disabled = false}`，除 `key` 与 `title` 外都可以省略。

```
setItems(items)          getItems() -> 数组
setTitles(sourceTitle, targetTitle)
getSourceTitle() -> string
getTargetTitle() -> string
getSourceCheckedKeys() -> 数组
getTargetCheckedKeys() -> 数组
```

### TreeSelect

`shad.TreeSelect.new([selectionMode], [parent])`，`selectionMode` 取 `shad.ComboBoxType`。节点用路径表示：从根节点到该节点的文字数组。

```
addItem(text, [parentPath]) -> path
clearItems()
setModel(model)                      使用 shad.Model，传 nil 恢复内部模型
model()    -> Model
treeView() -> TreeView
setCurrentPath(path)                 传 nil 或空数组清空
currentPath()    -> path
getCurrentText() -> string
setPathChecked(path, checked)
isPathChecked(path) -> boolean
getCheckedPaths() -> 路径数组
getCheckedTexts() -> 数组
clearChecked()
showPopup()  hidePopup()  isPopupVisible() -> boolean
```

多选需要节点带复选框，请用 `addItem` 添加节点，不要用 `setModel`。

## 表单

```
Field           shad.Field.new([label, control], [parent])
  labelWidget() -> Label
  setControl(control)      getControl() -> widget
  setErrors(errors)        getErrors()  -> 数组
  setError(error)
  clearErrors()

FieldGroup      shad.FieldGroup.new([parent])
  addWidget(widget)
  addSeparator([text]) -> FieldSeparator
  removeWidget(widget)

FieldSet        shad.FieldSet.new([legend, description], [parent])
  addWidget(widget)
  contentLayout() -> layout
```

### Questionnaire

`shad.Questionnaire.new([parent])`。题目与选项都用名称标识，`previous()`、`next()`、`skip()`、`submit()`、`reset()` 是槽。

```
addItem(name, title, [description])
getItems() -> 数组
setItemRequired(name, required)      isItemRequired(name) -> boolean
setItemMultiple(name, multiple)      isItemMultiple(name) -> boolean
setItemDisabled(name, disabled)      isItemDisabled(name) -> boolean
setItemTitle(name, title)
setItemDescription(name, description)
setItemInput(name, placeholder)
setItemError(name, message)          getItemError(name)   -> string
getItemStatus(name) -> number        取值见 shad.QuestionnaireType
addChoice(name, value, label, [description])
setChoiceDisabled(name, value, disabled)
setChoiceLabel(name, value, label)
setChoiceDescription(name, value, description)
setPreviousText(text)  getPreviousText() -> string
setSkipText(text)      getSkipText()     -> string
setNextText(text)      getNextText()     -> string
setSubmitText(text)    getSubmitText()   -> string
getCurrentIndex() -> number
getItemCount()    -> number
getAnswers(name)  -> 数组
setAnswers(name, answers)
getAllAnswers()   -> 表               以题目名称为键、答案数组为值
```

### PropertyGrid

`shad.PropertyGrid.new([parent])`。属性用 `key` 标识，值的类型决定编辑器。添加属性的函数都返回是否成功。

```
addProperty(key, label, value, [group])              value 为字符串、整数、小数或布尔值
addDoubleProperty(key, label, value, [group])
addColorProperty(key, label, color, [group])
addEnumProperty(key, label, options, [currentIndex], [group])
addWidgetProperty(key, label, editor, [group])
removeProperty(key)
clear()
hasProperty(key) -> boolean
getKeys() -> 数组
setValue(key, value)                     getValue(key) -> value
setPropertyLabel(key, label)             getPropertyLabel(key) -> string
setPropertyDescription(key, description) getPropertyDescription(key) -> string
setPropertyReadOnly(key, readOnly)       isPropertyReadOnly(key) -> boolean
setPropertyRange(key, minimum, maximum)
setEnumOptions(key, options)
getEditor(key) -> widget
setGroupTitle(group, title)              getGroupTitle(group) -> string
setGroupExpanded(group, expanded)        isGroupExpanded(group) -> boolean
getCurrentKey() -> string
```

## 数据展示

```
CodeBlock       shad.CodeBlock.new([code], [parent])
  appendCode(text)
  setLineVariant(lineIndex, variant)             variant 取 shad.CodeBlockType
  clearLineVariants()
  addActionWidget(widget)
  footerLayout() -> layout

DiffView        shad.DiffView.new([parent])
  setTexts(oldText, newText)
  getAddedLineCount()   -> number
  getRemovedLineCount() -> number
  selectedText() -> string

Empty           shad.Empty.new([icon], [title, description], [parent])
  setShadIcon(icon)        getShadIcon() -> icon
  setMediaWidget(widget)   getMediaWidget() -> widget
  contentLayout() -> layout

Result          shad.Result.new([status, title, description], [parent])   status、color、title、description 是属性，status 取 shad.ResultType
  setShadIcon(icon)        getShadIcon() -> icon     替换状态图标，传 0 恢复；与 color 组合可以表达任意状态
  setMediaWidget(widget)   getMediaWidget() -> widget   用任意组件取代图标，传 nil 恢复
  addButton(text, [variant]) -> Button               variant 取 shad.ButtonType
  addActionWidget(widget)
  addContentWidget(widget)                           加在说明与操作区之间

KbdGroup        shad.KbdGroup.new([keys], [parent])
  addKey(text)  -> Kbd
  addText(text) -> Label
  clear()
  bindAction(action)       getBoundAction() -> QAction
  shad.KbdGroup.keyTexts(sequence)     -> 数组
  shad.KbdGroup.shortcutText(sequence) -> string

Item            shad.Item.new([title, description], [parent])
  setMediaIcon(icon)       getMediaIcon() -> icon
  setMediaImagePath(path)
  setMediaWidget(widget)   getMediaWidget() -> widget
  addActionWidget(widget)
  removeActionWidget(widget)
  setHeaderWidget(widget)  getHeaderWidget() -> widget
  setFooterWidget(widget)  getFooterWidget() -> widget
  setHeaderText(text)
  setFooterText(text)

ItemGroup       shad.ItemGroup.new([parent])
  addItem(item)
  insertItem(index, item)
  removeItem(item)
  addSeparator()
  items() -> 数组

Descriptions    shad.Descriptions.new([title], [parent])
  setExtra(widget)         getExtra() -> widget
  addItem(label, text, [span]) 或 addItem(label, widget, [span]) -> index
  setItemLabel(index, label)   getItemLabel(index) -> string
  setItemText(index, text)     getItemText(index)  -> string
  getItemWidget(index) -> widget
  setItemSpan(index, span)     getItemSpan(index)  -> number
  removeItem(index)
  clear()
  count() -> number

Breadcrumb      shad.Breadcrumb.new([parent])
  addLink(text) -> index
  addPage(text) -> index
  addEllipsis([menu]) -> index
  addDropdown(text, menu) -> index
  setPath(texts)
  clear()
  count() -> number
  setItemText(index, text)     getItemText(index) -> string
  setSeparatorIcon(icon)       getSeparatorIcon() -> icon

AvatarGroup     shad.AvatarGroup.new([parent])
  addAvatar(avatar) -> index
  insertAvatar(index, avatar) -> index
  removeAvatar(index)
  count() -> number
  avatar(index) -> Avatar
  setCountIcon(icon)       getCountIcon() -> icon
  clearCount()
  hasCount() -> boolean

Attachment      shad.Attachment.new([icon], [title, description], [parent])
  setShadIcon(icon)        getShadIcon() -> icon
  setImagePath(path)
  addAction(icon, toolTip) -> Button
  getActions() -> 数组
  clearActions()
  setFile(path) -> boolean

AttachmentGroup shad.AttachmentGroup.new([parent])
  addAttachment(attachment)
  insertAttachment(index, attachment)
  removeAttachment(attachment)
  clear()
  count() -> number
  attachmentAt(index) -> Attachment
  scrollToAttachment(attachment)

MarkdownView    shad.MarkdownView.new([parent])
  appendMarkdown(markdown)
  clear()
  setBaseUrl(url)          getBaseUrl() -> string

ImageViewer     shad.ImageViewer.new([parent])
  setImagePaths(paths)     getImagePaths() -> 数组
  getImageCount() -> number
  open([index])
  isOpen() -> boolean
  shad.ImageViewer.preview(parent, paths, [index])

ImageCropper    shad.ImageCropper.new([parent])
  setImagePath(path) -> boolean
  rotateLeft()  rotateRight()
  flipHorizontal()  flipVertical()
  reset()
  setCropRect({x, y, 宽, 高})   getCropRect() -> 表
  croppedPngBase64() -> string                   没有图片时返回 nil

Timeline        shad.Timeline.new([parent])                      mode、reverse、pending 是属性，mode 取 shad.TimelineType
  addItem(title, [time], [description]) -> TimelineItem
  itemAt(index) -> TimelineItem
  count() -> number
  clear()
  pendingItem() -> TimelineItem                  没有设置 pending 时返回 nil

TimelineItem    由 Timeline:addItem 创建                         title、time、description、color 是属性，color 取 shad.ThemeType
  setIcon(icon)            getIcon() -> icon
  contentLayout() -> layout
```

### LogView

`shad.LogView.new([parent])`。`maxEntryCount`、`autoScroll`、`wordWrap`、`timestampVisible`、`levelVisible`、`timestampFormat`、`minimumLevel`、`filterText`、`placeholderText` 是属性，`minimumLevel` 取 `shad.LogViewType`；`debug(message)`、`info(message)`、`success(message)`、`warning(message)`、`error(message)`、`scrollToBottom()` 是槽。条数变化时发出 `countChanged(count)`。

```
append(level, message)                   level 取 shad.LogViewType
count() -> number                        全部条数，包括被过滤掉的
visibleCount() -> number
clear()
```

### ListView

`shad.ListView.new([parent])`。数据来自 `shad.Model`，每一行的附加内容用下面的方法设置，设置类方法返回是否成功。

```
setModel(model)
emptyState() -> Empty
setItemDescription(row, text)
setItemTrailingText(row, text)
setItemTrailingColor(row, [color])   color 为颜色字符串或 shad.ThemeType 的值，省略时恢复默认
setItemIcon(row, icon)
setItemChecked(row, checked)         同时给这一行加上复选框
isItemChecked(row) -> boolean
currentRow()  -> number
currentText() -> string
```

## 图表

折线图、面积图与柱状图共用下面的方法。`color` 为 `shad.ThemeType` 的值或颜色字符串。

```
setCategories(categories)
setTickLabels(labels)
addSeries(label, values, [color]) -> index
setSeriesValues(index, values)       getSeriesValues(index) -> 数组
appendData(categories, seriesValues, [maxCount])     seriesValues 为数组的数组
setSeriesLabel(index, label)         getSeriesLabel(index) -> string
setSeriesIcon(index, icon)
clearSeries()
getSeriesCount() -> number
isZoomed() -> boolean
resetZoom()
```

```
ScatterChart    shad.ScatterChart.new([parent])                  points 为 {{x, y}, ...}
  addSeries(label, points, [color]) -> index
  setSeriesPoints(index, points)
  appendPoints(index, points, [maxCount])
  setSeriesLabel(index, label)
  clearSeries()
  getSeriesCount() -> number

Heatmap         shad.Heatmap.new([parent])
  setData(rowCount, columnCount, values)
  appendRow(values, [maxRows])
  setRowLabels(labels)
  setColumnLabels(labels)
  setValueRange(minimum, maximum)
  resetValueRange()
  getRowCount() -> number
  getColumnCount() -> number

CandlestickChart shad.CandlestickChart.new([parent])
  setCandles(candles)
  appendCandles(candles, [maxCount])
  updateLastCandle(candle)
  setMovingAverages(periods)
  clearCandles()
  getCandleCount() -> number
```

一根 K 线为 `{label = "...", open = 1, high = 2, low = 0.5, close = 1.5, volume = 100}`。

## 数据模型与表格

### Model

`shad.Model.new([parent])`，表格、树形视图、列表与树形下拉共用的数据模型。

```
setHeaders(labels)
addRow(values) -> row
setRows(rows)                        rows 为数组的数组
setTree(nodes)
getValue(row, column) -> value
setValue(row, column, value)
removeRow(row)
clearRows()
rowCount()    -> number
columnCount() -> number
```

树的节点是文字，或 `{text = ..., icon = ..., checked = ..., children = {节点, ...}}`。

### 视图

```
Table、TreeView、ListView 共用
  setModel(model)
  currentRow()  -> number
  currentText() -> string
  currentPath() -> 数组               从根节点到当前项的文字

Table           shad.Table.new([parent])
  setColumnFixedWidth(column, width)
  setColumnSortable(column, sortable)
  setFooterText(column, text)
  clearFooter()

DataTable       shad.DataTable.new([parent])
  setSourceModel(model)
  getSelectedRows() -> 数组
  setFilterColumn(column)
  setFilterPlaceholderText(text)
  setColumnSortable(column, sortable)
  setColumnHideable(column, hideable)
  setColumnVisible(column, visible)    isColumnVisible(column) -> boolean
  setColumnFixedWidth(column, width)
  setSelectionEnabled(enabled)         isSelectionEnabled() -> boolean
  clearSelection()
  setPageSize(pageSize)                getPageSize()  -> number
  setPageIndex(pageIndex)              getPageIndex() -> number
  getPageCount() -> number
  setEmptyText(text)
  getTable()      -> Table
  getPagination() -> Pagination
```

## 布局与容器

```
ScrollArea      shad.ScrollArea.new([parent])
  setWidget(widget)        widget() -> widget
  getOverlayScrollBar(orientation) -> ScrollBar

Resizable       shad.Resizable.new([orientation], [parent])
  addWidget(widget)
  insertWidget(index, widget)
  count() -> number
  widget(index) -> widget
  indexOf(widget) -> index
  setSizes(sizes)          sizes() -> 数组
  setStretchFactor(index, stretch)
  setCollapsible(index, collapsible)

StackedWidget   shad.StackedWidget.new([parent])                 切换页面用 currentIndex 属性
  addWidget(page) -> index
  insertWidget(index, page) -> index
  removeWidget(page)
  count() -> number
  widget(index) -> widget
  indexOf(page) -> index
  currentWidget() -> widget

Collapsible     shad.Collapsible.new([parent])                   toggle() 是槽
  headerLayout()  -> layout
  contentLayout() -> layout
  setTrigger(button)       getTrigger() -> button

Expander        shad.Expander.new([parent])                      toggle() 是槽
  headerLayout() -> layout
  addItem(widget)
  removeItem(widget)
  getItems() -> 数组

Carousel        shad.Carousel.new([parent])
  addItem(item) -> index
  insertItem(index, item) -> index
  removeItem(index)
  count() -> number
  item(index) -> widget
  indexOf(item) -> index
  scrollPrev()  scrollNext()
  scrollTo(index, [animated])
  canScrollPrev() -> boolean
  canScrollNext() -> boolean
  snapCount() -> number

Toolbar         shad.Toolbar.new([orientation], [parent])
  addAction(icon, text) -> QAction
  addAction(action, [icon])
  addMenu(icon, text) -> DropdownMenu
  setMenuTitle(menu, title)
  addWidget(widget)
  addSeparator()
  addStretch()
  removeAction(action)
  clear()

StatusBar       shad.StatusBar.new([parent])
  addWidget(widget, [stretch])
  addPermanentWidget(widget, [stretch])
  addText(text) -> Label
  removeWidget(widget)
  showMessage(message, [timeout])
  clearMessage()
  currentMessage() -> string

Anchor          shad.Anchor.new([parent])                        currentIndex、offset、duration 是属性
  setScrollArea(area)      getScrollArea() -> widget
  addLink(title, target, [level]) -> index       target 必须在 area 的内容组件中
  setLinkTitle(index, title)   getLinkTitle(index) -> string
  getLinkTarget(index) -> widget
  count() -> number
  clear()

BackTop         shad.BackTop.new([area])                         visibilityHeight、offset、duration 是属性，scrollToTop() 是槽
  setTarget(area)          getTarget() -> widget                 按钮成为 area 的子组件
```

### Steps

`shad.Steps.new([parent])`。`currentIndex`、`status`、`orientation`、`clickable` 是属性，`status` 取 `shad.StepsType`，`next()`、`previous()` 是槽；用户点击步骤时发出 `stepClicked(index)`。

```
addStep(title, [description]) -> index
setStepTitle(index, title)               getStepTitle(index) -> string
setStepDescription(index, description)   getStepDescription(index) -> string
setStepIcon(index, icon)                 getStepIcon(index) -> icon
setStepDisabled(index, disabled)         isStepDisabled(index) -> boolean
getStepStatus(index) -> number           取值见 shad.StepsType
count() -> number
clear()
```

### TabView

`shad.TabView.new([parent])`。当前标签用 `currentIndex` 属性读写。

```
addTab(text, page) 或 addTab(icon, text, page) -> index     page 不是组件时返回 -1
insertTab(index, icon, text, page) -> index
removeTab(index)                     不删除页面
moveTab(from, to)
count() -> number
widget(index) -> widget
indexOf(page) -> index
setTabText(index, text)              tabText(index) -> string
setTabIcon(index, icon)              tabIcon(index) -> icon
setTabFlags(index, flags)            tabFlags(index) -> number     flags 取 shad.TabViewType
setTabFlag(index, flag, [on])
setTabToolTip(index, toolTip)        tabToolTip(index) -> string
setTabData(index, data)              tabData(index) -> value
tabBar() -> widget
```

### 停靠布局

停靠位置与面板的限制取 `shad.DockType`。

```
DockPanel       shad.DockPanel.new(id, [parent]) 或 shad.DockPanel.new(id, icon, title, widget, [parent])
  getId() -> string
  setWidget(widget)            getWidget() -> widget
  setIcon(icon)                getIcon() -> icon
  setTitleToolTip(toolTip)     getTitleToolTip() -> string
  setPanelFlags(flags)         getPanelFlags() -> number
  setPanelFlag(flag, [on])
  getDockManager() -> DockManager
  isDocument() -> boolean
  isFloating() -> boolean
  isOpen() -> boolean
  toggleViewAction() -> QAction

DockManager     shad.DockManager.new([parent])
  addPanel(panel, [area], [relativeTo])
  addDocument(panel)
  tabifyPanel(panel, existing)
  floatPanel(panel, [{x, y, 宽, 高}])
  removePanel(panel)
  closePanel(panel)
  showPanel(panel)
  setCurrentPanel(panel)       getCurrentPanel() -> DockPanel
  panels() -> 数组
  findPanel(id) -> DockPanel
  saveState() -> string                          Base64 文本
  restoreState(state) -> boolean
```

### 侧栏

```
Sidebar         shad.Sidebar.new([parent])
  toggle()
  isIconCollapsed() -> boolean
  setExpandedWidthRange(minimum, maximum)
  getMinimumExpandedWidth() -> number
  getMaximumExpandedWidth() -> number
  headerLayout() / footerLayout() / contentLayout() -> layout
  addGroup([label]) -> SidebarGroup
  setActiveButton(button)      getActiveButton() -> SidebarMenuButton

SidebarGroup    shad.SidebarGroup.new([label], [parent])
  addItem(icon, text) -> SidebarMenuButton
  addWidget(widget)
  menuLayout() -> layout
  setActionIcon(icon)          getActionIcon() -> icon
  getActionRect() -> 表

SidebarMenuButton  shad.SidebarMenuButton.new([icon], [text], [parent])
  setShadIcon(icon)            getShadIcon() -> icon
  setAvatar(avatar)            getAvatar() -> Avatar
  setActionIcon(icon)          getActionIcon() -> icon
  setActionAlwaysVisible(alwaysVisible)   isActionAlwaysVisible() -> boolean
  getActionRect() -> 表
  addSubItem(text, [icon]) -> SidebarMenuButton     按钮需要先加入布局
  hasSubItems() -> boolean
  isSubItem() -> boolean

SidebarLayout   shad.SidebarLayout.new([parent])
  setSidebar(sidebar)          getSidebar() -> Sidebar
  setContentWidget(widget)     getContentWidget() -> widget

SidebarTrigger  shad.SidebarTrigger.new([sidebar], [parent])
  setSidebar(sidebar)          getSidebar() -> Sidebar
```

## 弹层与菜单

弹出方向 `side` 与对齐方式 `align` 取 `shad.PlacementType`。

```
Drawer          shad.Drawer.new([title, description], [parent])  open() 与 close() 是槽
  contentLayout() -> layout
  footerLayout()  -> layout
  isOpen() -> boolean
  setSnapPoints(points)        getSnapPoints() -> 数组

LoadingOverlay  shad.LoadingOverlay.new([target])                loading、text、delay、radius、spinnerSize 是属性，start()、stop() 是槽
  setTarget(target)            getTarget() -> widget             遮罩成为 target 的子组件

Popover         shad.Popover.new([parent])
  contentLayout() -> layout
  popup(anchor, [side], [align])
  toggle(anchor, [side], [align])

HoverCard       shad.HoverCard.new([parent])
  setTrigger(trigger)          getTrigger() -> widget
  contentLayout() -> layout
  setSide(side)                getSide()  -> number
  setAlign(align)              getAlign() -> number
  popup()

Popconfirm      shad.Popconfirm.new([title, description], [parent])   继承 Popover，结果通过 confirmed 与 cancelled 信号得到
  setShadIcon(icon)            getShadIcon() -> icon
  confirmButton() -> Button
  cancelButton()  -> Button
  attach(button, [side], [align])

Command         shad.Command.new([parent])
  addGroup(heading) -> index
  setGroupHeading(groupIndex, heading)
  addItem(text, [icon], [shortcutText]) -> QAction
  setItemKeywords(action, keywords)
  addSeparator()
  clear()

Menubar         shad.Menubar.new([parent])
  addMenu(title) -> DropdownMenu
  removeMenu(menu)
  setMenuTitle(menu, title)
  getMenus() -> 数组

NavigationMenu  shad.NavigationMenu.new([parent])
  addMenu(title, content)
  setMenuTitle(content, title)
  addLink(title) -> QAction
  closeMenu()
```

### DropdownMenu

`shad.DropdownMenu.new([parent])`。

```
addItem(text, [icon], [shortcutText]) -> QAction
addCheckableItem(text, [checked])     -> QAction
addRadioItem(text, group, [checked])  -> QAction     group 为 shad.ActionGroup
addMenu(text, [icon]) -> DropdownMenu
menuAction() -> QAction
addLabel(text, [inset]) -> index
setLabelText(labelIndex, text)
addWidget(widget)
addSeparator()
clear()
setItemDestructive(action, destructive)
setItemInset(action, inset)
setMatchAnchorWidth(match)            isMatchAnchorWidth() -> boolean
popup(anchor, [alignment])            alignment 取 shad.Qt.AlignLeft 或 AlignRight
popup(anchor, side, align)
popup({x, y, 宽, 高}, [alignment])     以屏幕坐标的矩形为锚点
popupAt(x, y)                         屏幕坐标
```

菜单项被点击时请连接 `QAction` 的 `triggered` 信号。

### ContextMenu

`shad.ContextMenu.new([parent])`，继承 DropdownMenu。

```
attachTo(target)
detachFrom(target)
```
