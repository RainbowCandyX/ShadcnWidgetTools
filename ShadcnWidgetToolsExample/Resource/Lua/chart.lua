-- 图表：折线图、柱状图、散点图与 K 线图
local window = shad.Window.new()
window.windowTitle = "Lua 图表"
window:resize(860, 680)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)

local tabs = shad.Tabs.new()
root:addWidget(tabs)
root:addStretch(1)

-- 新建标签页放入图表
local function addChartPage(title, chart)
    local page = shad.Widget.new()
    local layout = shad.VBox.new(page)
    layout:setContentsMargins(0, 12, 0, 0)
    chart:setFixedHeight(420)
    layout:addWidget(chart)
    tabs:addTab(title, page)
end

local months = {"1 月", "2 月", "3 月", "4 月", "5 月", "6 月"}

-- 折线图
local line = shad.LineChart.new()
line:setCategories(months)
line:addSeries("桌面端", {186, 305, 237, 73, 209, 214}, shad.ThemeType.Chart1)
line:addSeries("移动端", {80, 200, 120, 190, 130, 140}, shad.ThemeType.Chart2)
line.legendVisible = true
line.yAxisVisible = true
line.dotsVisible = true
addChartPage("折线图", line)

-- 柱状图
local bar = shad.BarChart.new()
bar:setCategories(months)
bar:addSeries("收入", {420, 380, 510, 460, 590, 640})
bar:addSeries("支出", {260, 300, 280, 340, 310, 390}, "#f97316")
bar.stackMode = shad.ChartType.Stacked
bar.legendVisible = true
addChartPage("柱状图", bar)

-- 散点图
local function randomPoints(count, centerX, centerY)
    local points = {}
    for i = 1, count do
        points[i] = {centerX + (math.random() - 0.5) * 40, centerY + (math.random() - 0.5) * 40}
    end
    return points
end
local scatter = shad.ScatterChart.new()
scatter:addSeries("A 组", randomPoints(80, 40, 60), shad.ThemeType.Chart1)
scatter:addSeries("B 组", randomPoints(80, 70, 35), shad.ThemeType.Chart3)
scatter.legendVisible = true
addChartPage("散点图", scatter)

-- K 线图
local function randomCandles(count, price)
    local candles = {}
    for i = 1, count do
        local open = price
        price = math.max(1, price * (1 + (math.random() - 0.48) * 0.04))
        local high = math.max(open, price) * (1 + math.random() * 0.015)
        local low = math.min(open, price) * (1 - math.random() * 0.015)
        candles[i] = {label = "第 " .. i .. " 天", open = open, high = high, low = low, close = price, volume = 50000 + math.random(0, 150000)}
    end
    return candles
end
local candlestick = shad.CandlestickChart.new()
candlestick:setCandles(randomCandles(60, 100))
candlestick:setMovingAverages({5, 10})
candlestick.volumeVisible = true
candlestick.legendVisible = true
candlestick.zoomEnabled = true
addChartPage("K 线图", candlestick)

-- 折线图定时追加数据
local month = 6
local timer = shad.Timer.new()
timer.interval = 1000
timer:on("timeout", function()
    month = month + 1
    line:appendData({month .. " 月"}, {{math.random(60, 320)}, {math.random(60, 220)}}, 12)
end)
timer:start()

print("图表脚本已加载：折线图 " .. line:getSeriesCount() .. " 个系列，K 线 " .. candlestick:getCandleCount() .. " 根")

window:show()
