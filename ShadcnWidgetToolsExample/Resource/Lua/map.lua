-- 地图：矢量瓦片、标记、路线、浮动面板与移动中的车辆
local window = shad.Window.new()
window.windowTitle = "Lua 地图"
window:resize(900, 620)

local root = shad.VBox.new(window)
root:setContentsMargins(24, 16, 24, 24)
root.spacing = 12

local map = shad.Map.new()
map:setVectorTileUrl("https://tiles.openfreemap.org/planet")
map.tileFormat = shad.MapType.Vector
map.attribution = "© OpenMapTiles © OpenStreetMap contributors"

-- 标记：坐标写作 {纬度, 经度}
local landmarks = {
    {name = "台北 101", coordinate = {25.0338, 121.5645}, color = "#2563eb"},
    {name = "中正纪念堂", coordinate = {25.0346, 121.5218}, color = "#dc2626"},
    {name = "龙山寺", coordinate = {25.0372, 121.4999}, color = "#d97706"},
}
for _, landmark in ipairs(landmarks) do
    local marker = map:addMarker(landmark.coordinate)
    marker.label = landmark.name
    marker.color = landmark.color
    marker.popupTitle = landmark.name
    marker.popupText = "点击标记打开的弹出框"
end

-- 路线与车辆
local stops = {
    {25.0478, 121.5170},
    {25.0448, 121.5233},
    {25.0424, 121.5329},
    {25.0416, 121.5437},
    {25.0415, 121.5512},
    {25.0413, 121.5578},
    {25.0411, 121.5660},
}
local route = map:addRoute(stops)
route.color = "#2563eb"
local passed = map:addRoute({stops[1], stops[1]})
passed.color = "#9ca3af"
local origin = map:addMarker(stops[1])
origin.label = "起点"
origin.color = "#16a34a"
local destination = map:addMarker(stops[#stops])
destination.label = "终点"
local vehicle = map:addMarker(stops[1])
vehicle.toolTip = "配送车"
vehicle.color = "#2563eb"
vehicle.selected = true

-- 浮动面板：fitBounds 会避开它
local panel = shad.Card.new("配送路线", "台北车站 → 市政府")
panel:setFixedWidth(240)
local status = shad.Label.new("尚未出发")
status.colorRole = shad.ThemeType.MutedForeground
panel:contentLayout():addWidget(status)
map:addOverlay(panel, shad.MapType.TopLeft)
map:fitBounds(stops, 64)

local start = shad.Button.new("开始配送")
local globe = shad.Button.new("地球仪")
globe.variant = shad.ButtonType.Outline
local fit = shad.Button.new("回到路线")
fit.variant = shad.ButtonType.Outline
local row = shad.HBox.new()
row:addWidget(start)
row:addWidget(globe)
row:addWidget(fit)
row:addStretch(1)
root:addLayout(row)
root:addWidget(map, 1)

-- 按进度在相邻两个路线点之间插值，把路线分成走过与剩余两段
local function splitRoute(progress)
    local scaled = progress * (#stops - 1)
    local index = math.min(math.floor(scaled), #stops - 2)
    local ratio = scaled - index
    local from = stops[index + 1]
    local to = stops[index + 2]
    local position = {from[1] + (to[1] - from[1]) * ratio, from[2] + (to[2] - from[2]) * ratio}
    local passedStops = {}
    for i = 1, index + 1 do
        passedStops[#passedStops + 1] = stops[i]
    end
    passedStops[#passedStops + 1] = position
    local remainingStops = {position}
    for i = index + 2, #stops do
        remainingStops[#remainingStops + 1] = stops[i]
    end
    return position, passedStops, remainingStops
end

-- 车辆移动到当前进度的位置，走过的路线变灰
local progress = 0
local function applyProgress()
    local position, passedStops, remainingStops = splitRoute(progress)
    vehicle:setCoordinate(position)
    passed:setCoordinates(passedStops)
    route:setCoordinates(remainingStops)
    status.text = "已完成 " .. math.floor(progress * 100) .. "%"
end

local timer = shad.Timer.new()
timer.interval = 50
timer:on("timeout", function()
    progress = math.min(1, progress + 0.005)
    applyProgress()
    if progress >= 1 then
        timer:stop()
        print("配送完成")
    end
end)

start:on("clicked", function()
    progress = 0
    applyProgress()
    timer:start()
end)

globe:on("clicked", function()
    local isGlobe = map.projection == shad.MapType.Globe
    map.projection = isGlobe and shad.MapType.Mercator or shad.MapType.Globe
    globe.text = isGlobe and "地球仪" or "平面"
end)

fit:on("clicked", function()
    map:fitBounds(stops, 64)
end)

-- 信号的坐标参数是带 latitude、longitude 字段的表
map:on("clicked", function(coordinate)
    print(string.format("点击位置：%.4f, %.4f", coordinate.latitude, coordinate.longitude))
end)

map:on("markerClicked", function(marker)
    print("点击标记：" .. (marker.label ~= "" and marker.label or marker.toolTip))
end)

window:show()
