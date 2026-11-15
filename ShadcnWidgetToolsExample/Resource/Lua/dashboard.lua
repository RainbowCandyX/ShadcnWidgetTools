-- 数据面板：统计数值、环形进度、评分与二维码
local window = shad.Window.new()
window.windowTitle = "Lua 数据面板"
window:resize(780, 600)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)
root.spacing = 16

local range = shad.Segmented.new()
range:addItem("今日")
range:addItem("本周")
range:addItem("本月")
local rangeRow = shad.HBox.new()
rangeRow:addWidget(range)
rangeRow:addStretch(1)
root:addLayout(rangeRow)

-- 统计数值
local statistics = shad.HBox.new()
statistics.spacing = 16
local function addStatistic(title, value, suffix, trend)
    local card = shad.Card.new()
    local item = shad.Statistic.new(title)
    item.value = value
    item.suffix = suffix
    item.trend = trend
    card:contentLayout():addWidget(item)
    statistics:addWidget(card, 1)
    return item
end
local visits = addStatistic("访问量", 12840, " 次", shad.StatisticType.Up)
local orders = addStatistic("订单", 386, " 单", shad.StatisticType.Up)
local refund = addStatistic("退款率", 2.4, " %", shad.StatisticType.Down)
refund.precision = 1
root:addLayout(statistics)

-- 完成度与二维码
local details = shad.HBox.new()
details.spacing = 16

local goalCard = shad.Card.new("目标完成度", "拖动评分会写入日志")
local goal = shad.RadialProgress.new()
goal.value = 68
local rating = shad.Rating.new()
rating.rating = 4
goalCard:contentLayout():addWidget(goal, 0, shad.Qt.AlignHCenter)
goalCard:contentLayout():addWidget(rating, 0, shad.Qt.AlignHCenter)
details:addWidget(goalCard, 1)

local linkCard = shad.Card.new("项目地址", "扫码打开仓库")
local qrCode = shad.QrCode.new("https://github.com/RainbowCandyX/ShadcnWidgetTools")
linkCard:contentLayout():addWidget(qrCode, 0, shad.Qt.AlignHCenter)
details:addWidget(linkCard, 1)
root:addLayout(details)

local pagination = shad.Pagination.new()
pagination.pageCount = 12
root:addWidget(pagination)
root:addStretch(1)

local scales = {1, 7, 30}
range:on("currentIndexChanged", function(index)
    local scale = scales[index + 1]
    visits.value = 12840 * scale
    orders.value = 386 * scale
    goal.value = math.min(100, 68 + index * 12)
    print("统计范围切换为第 " .. (index + 1) .. " 项")
end)

rating:on("ratingChanged", function(value)
    print("评分：" .. value)
end)

pagination:on("currentPageChanged", function(page)
    print("当前页：" .. page)
end)

-- 定时器模拟实时数据
local timer = shad.Timer.new()
timer.interval = 2000
timer:on("timeout", function()
    visits.value = visits.value + math.random(1, 50)
end)
timer:start()

window:show()
