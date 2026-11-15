-- 纯功能脚本：不创建组件，只做计算与定时任务
print("纯功能脚本开始执行，运行时：" .. shad.runtime)

-- 计算：斐波那契数列
local function fibonacci(count)
    local values = {1, 1}
    for i = 3, count do
        values[i] = values[i - 1] + values[i - 2]
    end
    return values
end
print("斐波那契数列前 12 项：" .. table.concat(fibonacci(12), ", "))

-- 计算：质数
local function primes(limit)
    local isComposite = {}
    local result = {}
    for number = 2, limit do
        if not isComposite[number] then
            result[#result + 1] = number
            for multiple = number * number, limit, number do
                isComposite[multiple] = true
            end
        end
    end
    return result
end
print("50 以内的质数：" .. table.concat(primes(50), ", "))

-- 字符串处理
local text = "lua is small lua is fast lua is embeddable"
local counts = {}
for word in string.gmatch(text, "%a+") do
    counts[word] = (counts[word] or 0) + 1
end
print("单词 lua 出现 " .. counts.lua .. " 次，is 出现 " .. counts.is .. " 次")

-- 协程
local producer = coroutine.wrap(function()
    for step = 1, 3 do
        coroutine.yield("第 " .. step .. " 步")
    end
end)
print("协程依次产生：" .. producer() .. "、" .. producer() .. "、" .. producer())

-- 定时任务：卸载脚本会提前结束它
local ticks = 0
local timer = shad.Timer.new()
timer.interval = 1000
timer:on("timeout", function()
    ticks = ticks + 1
    print("定时任务第 " .. ticks .. " 次执行")
    if ticks >= 5 then
        timer:stop()
        print("定时任务结束")
    end
end)
timer:start()
