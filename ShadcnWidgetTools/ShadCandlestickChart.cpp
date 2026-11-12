#include "ShadCandlestickChart.h"

#include <QList>
#include <QtGlobal>

#include "ShadCandlestickChartPrivate.h"
#include "ShadDef.h"

namespace
{
    constexpr int kMaxPricePrecision = 8;
} // namespace

ShadCandlestickChart::ShadCandlestickChart(QWidget* parent)
    : ShadChart(*new ShadCandlestickChartPrivate(), parent)
{
}

ShadCandlestickChart::~ShadCandlestickChart()
{
}

ShadCandlestickChartPrivate* ShadCandlestickChart::candlestickPrivate() const
{
    return static_cast<ShadCandlestickChartPrivate*>(d_ptr.data());
}

void ShadCandlestickChart::setCandles(const QList<ShadCandle>& candles)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    d->_candles = candles;
    d->requestUpdate();
}

QList<ShadCandle> ShadCandlestickChart::getCandles() const
{
    return candlestickPrivate()->_candles;
}

int ShadCandlestickChart::getCandleCount() const
{
    return static_cast<int>(candlestickPrivate()->_candles.size());
}

void ShadCandlestickChart::appendCandles(const QList<ShadCandle>& candles, int maxCount)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    d->_candles.append(candles);
    if (maxCount > 0)
    {
        const qsizetype overflow = d->_candles.size() - maxCount;
        if (overflow > 0)
        {
            d->_candles.remove(0, overflow);
            d->shiftView(static_cast<qreal>(overflow));
        }
    }
    d->requestUpdate();
}

void ShadCandlestickChart::updateLastCandle(const ShadCandle& candle)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    if (d->_candles.isEmpty())
    {
        d->_candles.append(candle);
    }
    else
    {
        d->_candles.last() = candle;
    }
    d->requestUpdate();
}

void ShadCandlestickChart::clearCandles()
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    d->_candles.clear();
    d->requestUpdate();
}

void ShadCandlestickChart::setColorScheme(ShadChartType::CandleColorScheme scheme)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    if (d->_colorScheme == scheme)
    {
        return;
    }
    d->_colorScheme = scheme;
    d->requestUpdate();
}

ShadChartType::CandleColorScheme ShadCandlestickChart::getColorScheme() const
{
    return candlestickPrivate()->_colorScheme;
}

void ShadCandlestickChart::setMovingAverages(const QList<int>& periods)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    QList<int> validPeriods;
    for (const int period : periods)
    {
        if (period >= 1)
        {
            validPeriods.append(period);
        }
    }
    if (d->_movingAverages == validPeriods)
    {
        return;
    }
    d->_movingAverages = validPeriods;
    d->requestUpdate();
}

QList<int> ShadCandlestickChart::getMovingAverages() const
{
    return candlestickPrivate()->_movingAverages;
}

void ShadCandlestickChart::setVolumeVisible(bool visible)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    if (d->_isVolumeVisible == visible)
    {
        return;
    }
    d->_isVolumeVisible = visible;
    d->requestUpdate();
}

bool ShadCandlestickChart::isVolumeVisible() const
{
    return candlestickPrivate()->_isVolumeVisible;
}

void ShadCandlestickChart::setPricePrecision(int decimals)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    const int boundedDecimals = qBound(0, decimals, kMaxPricePrecision);
    if (d->_pricePrecision == boundedDecimals)
    {
        return;
    }
    d->_pricePrecision = boundedDecimals;
    d->requestUpdate();
}

int ShadCandlestickChart::getPricePrecision() const
{
    return candlestickPrivate()->_pricePrecision;
}

void ShadCandlestickChart::setGridVisible(bool visible)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    if (d->_isGridVisible == visible)
    {
        return;
    }
    d->_isGridVisible = visible;
    d->requestUpdate();
}

bool ShadCandlestickChart::isGridVisible() const
{
    return candlestickPrivate()->_isGridVisible;
}

void ShadCandlestickChart::setXAxisVisible(bool visible)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    if (d->_isXAxisVisible == visible)
    {
        return;
    }
    d->_isXAxisVisible = visible;
    d->requestUpdate();
}

bool ShadCandlestickChart::isXAxisVisible() const
{
    return candlestickPrivate()->_isXAxisVisible;
}

void ShadCandlestickChart::setYAxisVisible(bool visible)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    if (d->_isYAxisVisible == visible)
    {
        return;
    }
    d->_isYAxisVisible = visible;
    d->requestUpdate();
}

bool ShadCandlestickChart::isYAxisVisible() const
{
    return candlestickPrivate()->_isYAxisVisible;
}

void ShadCandlestickChart::setLegendVisible(bool visible)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    if (d->_isLegendVisible == visible)
    {
        return;
    }
    d->_isLegendVisible = visible;
    d->requestUpdate();
}

bool ShadCandlestickChart::isLegendVisible() const
{
    return candlestickPrivate()->_isLegendVisible;
}

void ShadCandlestickChart::setTooltipVisible(bool visible)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    if (d->_isTooltipVisible == visible)
    {
        return;
    }
    d->_isTooltipVisible = visible;
    d->requestUpdate();
}

bool ShadCandlestickChart::isTooltipVisible() const
{
    return candlestickPrivate()->_isTooltipVisible;
}

void ShadCandlestickChart::setYTickCount(int count)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    const int boundedCount = qMax(2, count);
    if (d->_yTickCount == boundedCount)
    {
        return;
    }
    d->_yTickCount = boundedCount;
    d->requestUpdate();
}

int ShadCandlestickChart::getYTickCount() const
{
    return candlestickPrivate()->_yTickCount;
}

void ShadCandlestickChart::setZoomEnabled(bool enabled)
{
    ShadCandlestickChartPrivate* d = candlestickPrivate();
    if (d->_isZoomEnabled == enabled)
    {
        return;
    }
    d->_isZoomEnabled = enabled;
    if (!enabled)
    {
        d->resetZoom();
    }
}

bool ShadCandlestickChart::isZoomEnabled() const
{
    return candlestickPrivate()->_isZoomEnabled;
}

bool ShadCandlestickChart::isZoomed() const
{
    return candlestickPrivate()->_isZoomed;
}

void ShadCandlestickChart::resetZoom()
{
    candlestickPrivate()->resetZoom();
}
