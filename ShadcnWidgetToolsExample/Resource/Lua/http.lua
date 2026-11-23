-- 网络与数据：JSON、XML 与异步 HTTP
local window = shad.Window.new()
window.windowTitle = "Lua 网络与数据"
window:resize(560, 520)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)
root.spacing = 12

-- JSON
local data, jsonError = shad.json.decode('{"name": "ShadcnWidgetTools", "tags": ["qt", "lua"], "stars": 128, "archived": false}')
if data then
    print("JSON 解析：name = " .. data.name .. "，第一个标签 = " .. data.tags[1] .. "，stars = " .. data.stars)
    print("JSON 输出：" .. shad.json.encode({name = data.name, count = #data.tags}))
else
    print("JSON 解析失败：" .. jsonError)
end
local _, badJsonError = shad.json.decode("{not json}")
print("错误的 JSON 返回 nil 与说明：" .. tostring(badJsonError))

-- XML
local document, xmlError = shad.xml.decode([[
<library name="ShadcnWidgetTools" version="1.5.0">
    <component type="chart">ShadLineChart</component>
    <component type="input">ShadInput</component>
</library>]])
if document then
    print("XML 根元素：" .. document.name .. "，version = " .. document.attributes.version)
    for _, child in ipairs(document.children) do
        print("  " .. child.name .. "（" .. child.attributes.type .. "）：" .. child.text)
    end
else
    print("XML 解析失败：" .. xmlError)
end

-- HTTP：异步请求，完成后调用回调
local card = shad.Card.new("HTTP 请求", "GET 从 GitHub 接口读取仓库信息，POST 把数据发给 httpbin.org 并显示服务器收到的内容")
root:addWidget(card)
root:addStretch(1)
local content = card:contentLayout()

local status = shad.Label.new("尚未请求")
status.colorRole = shad.ThemeType.MutedForeground
local result = shad.CodeBlock.new("")
result.language = "json"
result.wordWrap = true
result.maximumContentHeight = 220
local getButton = shad.Button.new("GET 请求")
local postButton = shad.Button.new("POST 请求")
postButton.variant = shad.ButtonType.Outline
local buttons = shad.HBox.new()
buttons:addWidget(getButton)
buttons:addWidget(postButton)
buttons:addStretch(1)
content:addLayout(buttons)
content:addWidget(status)
content:addWidget(result)

local function setBusy(button, isBusy)
    button.loading = isBusy
    getButton.enabled = not isBusy
    postButton.enabled = not isBusy
    if isBusy then
        status.text = "请求中..."
    end
end

-- 请求失败或响应不是 JSON 时返回 nil
local function decodeResponse(response)
    if not response.ok then
        status.text = "请求失败：状态码 " .. response.status .. "，" .. tostring(response.error)
        print("HTTP 请求失败：" .. tostring(response.error))
        return nil
    end
    local data, decodeError = shad.json.decode(response.body)
    if not data then
        status.text = "响应不是有效的 JSON：" .. decodeError
        return nil
    end
    status.text = "状态码 " .. response.status .. "，共 " .. #response.body .. " 字节"
    return data
end

-- GET：选项中可以设置请求头、查询参数与超时
getButton:on("clicked", function()
    setBusy(getButton, true)
    shad.http.get("https://api.github.com/repos/shadcn-ui/ui", {
        headers = {["User-Agent"] = "ShadcnWidgetTools-Lua", ["Accept"] = "application/json"},
        timeout = 15000,
    }, function(response)
        setBusy(getButton, false)
        local repository = decodeResponse(response)
        if not repository then
            return
        end
        result.code = shad.json.encode({
            name = repository.full_name,
            description = repository.description,
            stars = repository.stargazers_count,
            language = repository.language,
        }, true)
        print("HTTP GET 成功：" .. repository.full_name .. "，" .. repository.stargazers_count .. " 个星标")
    end)
end)

-- POST：正文是表时按 JSON 发送
postButton:on("clicked", function()
    setBusy(postButton, true)
    shad.http.post("https://httpbin.org/post", {title = "Lua 发送的数据", tags = {"qt", "lua"}, count = 3}, {
        headers = {["User-Agent"] = "ShadcnWidgetTools-Lua", ["X-Request-Source"] = "example"},
        query = {page = 1},
        timeout = 15000,
    }, function(response)
        setBusy(postButton, false)
        local echo = decodeResponse(response)
        if not echo then
            return
        end
        result.code = shad.json.encode({url = echo.url, json = echo.json, contentType = echo.headers["Content-Type"]}, true)
        print("HTTP POST 成功：服务器收到 " .. shad.json.encode(echo.json))
    end)
end)

window:show()
