#include "ChartCandlestickPage.h"

#include <QDate>
#include <QList>
#include <QRandomGenerator>
#include <QString>
#include <QTime>
#include <QTimer>
#include <QWidget>
#include <QtGlobal>

#include <cmath>
#include <functional>
#include <memory>

#include "ExampleI18n.h"
#include "ShadCandlestickChart.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kDailyCount = 120;
    constexpr int kHistoryCount = 365;
    constexpr int kRealtimeHistoryCount = 60;
    constexpr int kRealtimeMaxCount = 120;
    constexpr int kIndicatorHeight = 360;
    constexpr int kRealtimeIntervalMs = 200;
    constexpr int kTicksPerCandle = 15;
    constexpr quint32 kDailySeed = 20250102;
    constexpr quint32 kHistorySeed = 20240101;
    constexpr quint32 kRealtimeSeed = 20261112;

    // 随机游走生成 K 线
    QList<ShadCandle> generateCandles(int count, qreal startPrice, quint32 seed, const std::function<QString(int)>& labelAt)
    {
        QRandomGenerator random(seed);
        QList<ShadCandle> candles;
        candles.reserve(count);
        qreal close = startPrice;
        for (int index = 0; index < count; ++index)
        {
            ShadCandle candle;
            candle.label = labelAt(index);
            candle.open = close;
            close = qMax(1.0, close * (1.0 + (random.generateDouble() - 0.48) * 0.04));
            candle.close = close;
            candle.high = qMax(candle.open, candle.close) * (1.0 + random.generateDouble() * 0.015);
            candle.low = qMin(candle.open, candle.close) * (1.0 - random.generateDouble() * 0.015);
            candle.volume = std::round(50000.0 + random.generateDouble() * 150000.0);
            candles.append(candle);
        }
        return candles;
    }

    QList<ShadCandle> generateDailyCandles(int count, qreal startPrice, quint32 seed)
    {
        const QDate firstDate(2025, 1, 2);
        return generateCandles(count, startPrice, seed, [firstDate](int index) { return firstDate.addDays(index).toString(QStringLiteral("MM-dd")); });
    }

    struct RealtimeData
    {
        QRandomGenerator random{kRealtimeSeed};
        QTime time;
        int tickCount{0};
        ShadCandle current;
    };

    void tickRealtime(RealtimeData& data, ShadCandlestickChart* chart)
    {
        ShadCandle& current = data.current;
        current.close = qMax(1.0, current.close * (1.0 + (data.random.generateDouble() - 0.5) * 0.004));
        current.high = qMax(current.high, current.close);
        current.low = qMin(current.low, current.close);
        current.volume += std::round(data.random.generateDouble() * 2000.0);
        chart->updateLastCandle(current);
        if (++data.tickCount < kTicksPerCandle)
        {
            return;
        }
        data.tickCount = 0;
        data.time = data.time.addSecs(60);
        ShadCandle next;
        next.label = data.time.toString(QStringLiteral("HH:mm"));
        next.open = current.close;
        next.high = current.close;
        next.low = current.close;
        next.close = current.close;
        current = next;
        chart->appendCandles({next}, kRealtimeMaxCount);
    }
} // namespace

ChartCandlestickPage::ChartCandlestickPage(QWidget* parent)
    : ExampleBasePage([] { return tr("K 线图"); }, [] { return tr("ShadCandlestickChart：GPU 绘制的 K 线图（蜡烛图），GPU 不可用时改用 CPU。"); }, parent)
{
    ShadCard* defaultCard = addSection([] { return tr("默认"); }, [] { return tr("120 根日 K，价格轴在右侧；悬停显示十字线与开高低收"); });
    ShadCandlestickChart* defaultChart = new ShadCandlestickChart(this);
    defaultChart->setCandles(generateDailyCandles(kDailyCount, 100.0, kDailySeed));
    addStretchItem(defaultCard, defaultChart);

    ShadCard* indicatorCard = addSection([] { return tr("均线与成交量"); }, []
    {
        return tr("setMovingAverages({5, 10, 20}) 显示均线，setVolumeVisible(true) 显示成交量副图，setZoomEnabled(true) 可滚轮缩放、拖动平移");
    });
    ShadCandlestickChart* indicatorChart = new ShadCandlestickChart(this);
    indicatorChart->setFixedHeight(kIndicatorHeight);
    indicatorChart->setCandles(generateDailyCandles(kHistoryCount, 3200.0, kHistorySeed));
    indicatorChart->setMovingAverages({5, 10, 20});
    indicatorChart->setVolumeVisible(true);
    indicatorChart->setLegendVisible(true);
    indicatorChart->setZoomEnabled(true);
    addStretchItem(indicatorCard, indicatorChart);

    ShadCard* colorCard = addSection([] { return tr("涨跌配色"); }, [] { return tr("setColorScheme(GreenRising) 切换为绿涨红跌，开关可随时切换"); });
    ShadSwitch* colorSwitch = new ShadSwitch(this);
    colorSwitch->setChecked(true);
    ExampleI18n::bind(colorSwitch, [colorSwitch]() { colorSwitch->setText(tr("绿涨红跌")); });
    ShadCandlestickChart* colorChart = new ShadCandlestickChart(this);
    colorChart->setCandles(generateDailyCandles(kDailyCount, 100.0, kDailySeed));
    colorChart->setColorScheme(ShadChartType::GreenRising);
    connect(colorSwitch, &ShadSwitch::toggled, colorChart, [colorChart](bool checked)
    {
        colorChart->setColorScheme(checked ? ShadChartType::GreenRising : ShadChartType::RedRising);
    });
    addItem(colorCard, colorSwitch);
    addStretchItem(colorCard, colorChart);

    ShadCard* realtimeCard = addSection([] { return tr("实时行情"); }, []
    {
        return tr("updateLastCandle 每 200 毫秒更新最后一根，每 3 秒 appendCandles 追加一根，只保留最近 120 根");
    });
    ShadSwitch* realtimeSwitch = new ShadSwitch(this);
    ExampleI18n::bind(realtimeSwitch, [realtimeSwitch]() { realtimeSwitch->setText(tr("开始行情")); });
    ShadCandlestickChart* realtimeChart = new ShadCandlestickChart(this);
    realtimeChart->setFixedHeight(kIndicatorHeight);
    realtimeChart->setMovingAverages({5, 10});
    realtimeChart->setVolumeVisible(true);
    realtimeChart->setZoomEnabled(true);
    std::shared_ptr<RealtimeData> realtime = std::make_shared<RealtimeData>();
    const QTime openTime(9, 30);
    const QList<ShadCandle> history = generateCandles(kRealtimeHistoryCount, 25.0, kRealtimeSeed, [openTime](int index) { return openTime.addSecs(index * 60).toString(QStringLiteral("HH:mm")); });
    realtimeChart->setCandles(history);
    realtime->current = history.last();
    realtime->time = openTime.addSecs((kRealtimeHistoryCount - 1) * 60);
    QTimer* realtimeTimer = new QTimer(realtimeChart);
    realtimeTimer->setInterval(kRealtimeIntervalMs);
    connect(realtimeTimer, &QTimer::timeout, realtimeChart, [realtime, realtimeChart]() { tickRealtime(*realtime, realtimeChart); });
    connect(realtimeSwitch, &ShadSwitch::toggled, realtimeTimer, [realtimeTimer](bool checked)
    {
        if (checked)
        {
            realtimeTimer->start();
        }
        else
        {
            realtimeTimer->stop();
        }
    });
    addItem(realtimeCard, realtimeSwitch);
    addStretchItem(realtimeCard, realtimeChart);
}

ChartCandlestickPage::~ChartCandlestickPage()
{
}
