#include "ShadCandlestickChartPrivate.h"

#include <QColor>
#include <QCoreApplication>
#include <QFont>
#include <QFontMetricsF>
#include <QList>
#include <QLocale>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <cmath>
#include <limits>
#include <utility>

#include "ShadChart.h"
#include "ShadChartOverlay.h"
#if defined(SHADCN_CHART_GPU)
#include "ShadChartView.h"
#endif
#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kChartColorCount = 5;
    // 涨跌颜色固定，不随主题变化
    constexpr QRgb kRedRgb = qRgb(0xef, 0x44, 0x44);
    constexpr QRgb kGreenRgb = qRgb(0x22, 0xc5, 0x5e);
    constexpr int kPriceTagPadding = 4;
    constexpr qreal kPriceTagRadius = 4.0;
    constexpr qreal kCrosshairDash = 4.0;
    constexpr qreal kNaN = std::numeric_limits<qreal>::quiet_NaN();
} // namespace

ShadCandlestickChartPrivate::ShadCandlestickChartPrivate()
{
}

ShadCandlestickChartPrivate::~ShadCandlestickChartPrivate()
{
}

void ShadCandlestickChartPrivate::updateChart()
{
    _isUpdatePending = false;
    computeLayout();
    const QRect plot = physicalPlotRect();
    placePlotView(plot, !plot.isEmpty() && _count > 0);
    PlotGeometry geometry = buildGeometry();
#if defined(SHADCN_CHART_GPU)
    if (_view != nullptr)
    {
        _view->setVertices(buildVertices(geometry));
    }
#endif
    if (_softwareView != nullptr)
    {
        _plotGeometry = std::move(geometry);
    }
    // 数据或可见范围变化后，提示仍对应鼠标下的 K 线
    const bool hadHover = _hoverIndex >= 0;
    if (hadHover)
    {
        _hoverIndex = candleAt(_hoverPosition);
    }
    updateOverlay(plot, hadHover || _hoverIndex >= 0);
}

void ShadCandlestickChartPrivate::paintPlot(QPainter* painter) const
{
    // 已对齐像素，不开抗锯齿
    for (const QRectF& line : _plotGeometry.gridLines)
    {
        painter->fillRect(line, _plotGeometry.gridColor);
    }
    for (const PlotRect& item : _plotGeometry.rects)
    {
        painter->fillRect(item.rect, item.color);
    }
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(Qt::NoBrush);
    for (const PlotLine& line : _plotGeometry.lines)
    {
        painter->setPen(QPen(line.color, kMovingAverageWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
        painter->drawPolyline(QPolygonF(line.points));
    }
}

void ShadCandlestickChartPrivate::paintOverlay(QPainter* painter)
{
    paintAxes(painter);
    if (_isLegendVisible && !_movingAverages.isEmpty())
    {
        QList<ShadChartHelper::LegendItem> items;
        for (int index = 0; index < _movingAverages.size(); ++index)
        {
            items.append({QStringLiteral("MA%1").arg(_movingAverages.at(index)), movingAverageColor(index)});
        }
        ShadChartHelper::paintLegend(painter, q_ptr, items);
    }
    paintCrosshair(painter);
    paintTooltip(painter);
}

void ShadCandlestickChartPrivate::updateHover(const QPointF& position)
{
    const int index = candleAt(position);
    if (index < 0)
    {
        clearHover();
        return;
    }
    // 水平线跟随鼠标，每次移动都重绘
    _hoverIndex = index;
    _hoverPosition = position;
    _overlay->update();
}

void ShadCandlestickChartPrivate::clearHover()
{
    if (_hoverIndex < 0)
    {
        return;
    }
    _hoverIndex = -1;
    _overlay->update();
}

bool ShadCandlestickChartPrivate::zoomAt(const QPointF& position, int angleDelta)
{
    if (angleDelta == 0)
    {
        return false;
    }
    return zoomByFactor(position, std::pow(kZoomStepFactor, -angleDelta / 120.0));
}

bool ShadCandlestickChartPrivate::zoomByFactor(const QPointF& position, qreal factor)
{
    const QRect plot = physicalPlotRect();
    if (!_isZoomEnabled || _count < 2 || factor <= 0.0 || qFuzzyCompare(factor, 1.0) || plot.isEmpty() || !QRectF(plot).contains(position))
    {
        return false;
    }
    const qreal fullSpan = _count - 1;
    const qreal minSpan = minVisibleSpan();
    // 无法放大或已完全缩小时交给父组件滚动
    if (fullSpan <= minSpan || (factor < 1.0 && !_isZoomed))
    {
        return false;
    }
    // 鼠标位置在缩放前后不动
    const qreal fraction = qBound(0.0, (physicalX(position.x()) - _plotRect.left()) / _plotRect.width(), 1.0);
    const qreal anchor = _viewStart - 0.5 + fraction * (_viewSpan + 1.0);
    const qreal span = qBound(minSpan, _viewSpan / factor, fullSpan);
    if (span >= fullSpan)
    {
        resetZoom();
    }
    else
    {
        _isZoomed = true;
        _viewSpan = span;
        _viewStart = qBound(0.0, anchor + 0.5 - fraction * (span + 1.0), fullSpan - span);
        _isAnchoredToEnd = _viewStart + span >= fullSpan - 1e-6;
        updateChart();
    }
    updateHover(position);
    return true;
}

bool ShadCandlestickChartPrivate::beginPan(const QPointF& position)
{
    const QRect plot = physicalPlotRect();
    if (!_isZoomEnabled || !_isZoomed || plot.isEmpty() || !QRectF(plot).contains(position))
    {
        return false;
    }
    _isPanning = true;
    _panStartX = position.x();
    _panStartView = _viewStart;
    return true;
}

void ShadCandlestickChartPrivate::panTo(const QPointF& position)
{
    if (!_isPanning || !_isZoomed || _plotRect.width() <= 0)
    {
        return;
    }
    // 向右拖动显示更早的 K 线，RTL 时相反
    qreal deltaX = position.x() - _panStartX;
    if (q_ptr->layoutDirection() == Qt::RightToLeft)
    {
        deltaX = -deltaX;
    }
    const qreal fullSpan = _count - 1;
    _viewStart = qBound(0.0, _panStartView - deltaX / _plotRect.width() * visibleUnits(), qMax(0.0, fullSpan - _viewSpan));
    _isAnchoredToEnd = _viewStart + _viewSpan >= fullSpan - 1e-6;
    updateChart();
}

void ShadCandlestickChartPrivate::endPan()
{
    _isPanning = false;
}

bool ShadCandlestickChartPrivate::handleDoubleClick(const QPointF& position)
{
    if (!_isZoomEnabled || !_isZoomed)
    {
        return false;
    }
    resetZoom();
    updateHover(position);
    return true;
}

void ShadCandlestickChartPrivate::resetZoom()
{
    _isZoomed = false;
    _isAnchoredToEnd = false;
    _isPanning = false;
    updateChart();
}

void ShadCandlestickChartPrivate::shiftView(qreal count)
{
    if (!_isZoomed || _isAnchoredToEnd)
    {
        return;
    }
    // 拖动中的起点同步前移，拖动不跳动
    _viewStart -= count;
    _panStartView -= count;
}

int ShadCandlestickChartPrivate::candleAt(const QPointF& position) const
{
    const QRect plot = physicalPlotRect();
    if (!_isTooltipVisible || _count == 0 || plot.isEmpty() || !QRectF(plot).contains(position))
    {
        return -1;
    }
    // physicalX 也可把实际 X 转回逻辑 X
    const qreal logicalX = physicalX(position.x());
    return qBound(0, qRound(_viewStart - 0.5 + (logicalX - _plotRect.left()) / _plotRect.width() * visibleUnits()), _count - 1);
}

void ShadCandlestickChartPrivate::computeMovingAverages()
{
    const int averageCount = static_cast<int>(_movingAverages.size());
    _averageValues.resize(averageCount);
    for (int average = 0; average < averageCount; ++average)
    {
        const int period = _movingAverages.at(average);
        QList<qreal>& values = _averageValues[average];
        values.resize(_count);
        qreal* target = values.data();
        qreal sum = 0.0;
        for (int index = 0; index < _count; ++index)
        {
            sum += _candles.at(index).close;
            if (index >= period)
            {
                sum -= _candles.at(index - period).close;
            }
            target[index] = index >= period - 1 ? sum / period : kNaN;
        }
    }
}

void ShadCandlestickChartPrivate::computeLayout()
{
    const ShadChart* q = q_ptr;
    _count = static_cast<int>(_candles.size());
    computeMovingAverages();
    updateVisibleRange();

    // 价格范围包含可见范围内的均线
    const int firstIndex = qMax(0, static_cast<int>(std::floor(_viewStart)));
    const int lastIndex = qMin(_count - 1, static_cast<int>(std::ceil(_viewStart + _viewSpan)));
    qreal minPrice = std::numeric_limits<qreal>::max();
    qreal maxPrice = std::numeric_limits<qreal>::lowest();
    qreal maxVolume = 0.0;
    for (int index = firstIndex; index <= lastIndex; ++index)
    {
        const ShadCandle& candle = _candles.at(index);
        minPrice = qMin(minPrice, qMin(candle.low, qMin(candle.open, candle.close)));
        maxPrice = qMax(maxPrice, qMax(candle.high, qMax(candle.open, candle.close)));
        maxVolume = qMax(maxVolume, candle.volume);
        for (const QList<qreal>& values : std::as_const(_averageValues))
        {
            const qreal value = values.at(index);
            if (!std::isnan(value))
            {
                minPrice = qMin(minPrice, value);
                maxPrice = qMax(maxPrice, value);
            }
        }
    }
    if (minPrice > maxPrice)
    {
        minPrice = 0.0;
        maxPrice = 1.0;
    }
    _priceTicks = ShadChartHelper::niceTicks(minPrice, maxPrice, _yTickCount);
    _priceMin = _priceTicks.first();
    _priceMax = _priceTicks.last();
    _volumeTicks = ShadChartHelper::niceTicks(0.0, maxVolume, kVolumeTickCount);
    _volumeMax = _volumeTicks.last();

    const QFont font = textFont();
    const int left = ShadChartHelper::kHorizontalMargin;
    int right = q->width() - ShadChartHelper::kHorizontalMargin;
    const int top = ShadChartHelper::kVerticalMargin;
    int bottom = q->height() - ShadChartHelper::kVerticalMargin;
    if (_isLegendVisible && !_movingAverages.isEmpty())
    {
        bottom -= ShadChartHelper::kLegendTopPadding + ShadChartHelper::kLegendLineHeight;
    }
    if (_isXAxisVisible)
    {
        bottom -= ShadChartHelper::kXAxisHeight;
    }
    if (_isYAxisVisible)
    {
        int labelWidth = 0;
        for (const qreal tick : std::as_const(_priceTicks))
        {
            labelWidth = qMax(labelWidth, ShadStyleHelper::textWidth(font, ShadChartHelper::formatValue(tick)));
        }
        if (_isVolumeVisible)
        {
            labelWidth = qMax(labelWidth, ShadStyleHelper::textWidth(font, ShadChartHelper::formatValue(_volumeMax)));
        }
        right -= labelWidth + ShadChartHelper::kTickMargin;
    }
    _plotRect = QRect(left, top, qMax(0, right - left), qMax(0, bottom - top));
    _priceRect = _plotRect;
    _volumeRect = QRect();
    if (_isVolumeVisible)
    {
        const int volumeHeight = qRound(_plotRect.height() * kVolumeRatio);
        const int priceHeight = qMax(0, _plotRect.height() - volumeHeight - kPaneGap);
        _priceRect = QRect(left, top, _plotRect.width(), priceHeight);
        _volumeRect = QRect(left, top + priceHeight + kPaneGap, _plotRect.width(), qMax(0, _plotRect.height() - priceHeight - kPaneGap));
    }

    // X 轴刻度文字至少间隔 kMinTickGap
    _xTickIndices.clear();
    _xTickCenters.clear();
    if (!_isXAxisVisible || _count == 0 || _plotRect.width() <= 0)
    {
        return;
    }
    if (_count == 1)
    {
        _xTickIndices.append(0);
        _xTickCenters.append(candleX(0));
        return;
    }
    const QFontMetricsF metrics(font);
    const int firstTick = qMax(0, static_cast<int>(std::ceil(_viewStart - 0.5 - 1e-9)));
    const int lastTick = qMin(_count - 1, static_cast<int>(std::floor(_viewStart + _viewSpan + 0.5 + 1e-9)));
    const qreal spacing = _plotRect.width() / visibleUnits();
    const auto requiredStep = [spacing](qreal labelWidth) { return qMax(1, static_cast<int>(std::ceil((std::ceil(labelWidth) + ShadChartHelper::kMinTickGap) / spacing))); };
    // 放大后取 step 的整数倍，平移时刻度不跳动
    const auto collectTicks = [this, firstTick, lastTick](int step)
    {
        QList<int> ticks;
        if (_isZoomed)
        {
            for (int index = (firstTick + step - 1) / step * step; index <= lastTick; index += step)
            {
                ticks.append(index);
            }
        }
        else
        {
            for (int index = lastTick; index >= firstTick; index -= step)
            {
                ticks.prepend(index);
            }
        }
        return ticks;
    };
    // 只测量选中刻度的文字，K 线很多时不必逐个测量
    int step = requiredStep(metrics.horizontalAdvance(candleLabel(lastTick)));
    QList<int> ticks;
    for (;;)
    {
        ticks = collectTicks(step);
        qreal maxLabelWidth = 0.0;
        for (const int index : std::as_const(ticks))
        {
            maxLabelWidth = qMax(maxLabelWidth, metrics.horizontalAdvance(candleLabel(index)));
        }
        const int required = requiredStep(maxLabelWidth);
        if (required <= step)
        {
            break;
        }
        step = required;
    }
    for (const int index : std::as_const(ticks))
    {
        const qreal center = candleX(index);
        // 放大后文字超出组件两侧时不显示
        const qreal halfWidth = std::ceil(metrics.horizontalAdvance(candleLabel(index))) / 2.0;
        if (_isZoomed && (center - halfWidth < 0.0 || center + halfWidth > q->width()))
        {
            continue;
        }
        _xTickIndices.append(index);
        _xTickCenters.append(center);
    }
    // 最后一个刻度超出绘图区时向内移动，太挤时去掉前一个
    const qreal plotEnd = _plotRect.left() + _plotRect.width();
    const qreal tailWidth = _xTickIndices.isEmpty() ? 0.0 : std::ceil(metrics.horizontalAdvance(candleLabel(_xTickIndices.last())));
    if (!_isZoomed && !_xTickIndices.isEmpty() && _xTickCenters.last() + tailWidth / 2.0 > plotEnd && tailWidth < _plotRect.width())
    {
        _xTickCenters.last() = plotEnd - tailWidth / 2.0;
        if (_xTickIndices.size() > 1)
        {
            const qsizetype previous = _xTickIndices.size() - 2;
            const qreal previousWidth = std::ceil(metrics.horizontalAdvance(candleLabel(_xTickIndices.at(previous))));
            if (_xTickCenters.at(previous) + previousWidth / 2.0 + ShadChartHelper::kMinTickGap > _xTickCenters.last() - tailWidth / 2.0)
            {
                _xTickIndices.removeAt(previous);
                _xTickCenters.removeAt(previous);
            }
        }
    }
}

void ShadCandlestickChartPrivate::updateVisibleRange()
{
    const qreal fullSpan = qMax(0.0, static_cast<qreal>(_count - 1));
    if (_isZoomEnabled && _isZoomed && _count >= 2)
    {
        _viewSpan = qBound(minVisibleSpan(), _viewSpan, fullSpan);
        _isZoomed = _viewSpan < fullSpan;
    }
    else
    {
        _isZoomed = false;
    }
    if (!_isZoomed)
    {
        _viewStart = 0.0;
        _viewSpan = fullSpan;
        _isAnchoredToEnd = false;
        _isPanning = false;
    }
    else
    {
        if (_isAnchoredToEnd)
        {
            _viewStart = fullSpan - _viewSpan;
        }
        _viewStart = qBound(0.0, _viewStart, fullSpan - _viewSpan);
    }
    _firstVisible = qMax(0, static_cast<int>(std::floor(_viewStart)) - kEdgePadding);
    _lastVisible = qMin(_count - 1, static_cast<int>(std::ceil(_viewStart + _viewSpan)) + kEdgePadding);
}

qreal ShadCandlestickChartPrivate::minVisibleSpan() const
{
    return qMin(kMinVisibleSpan, qMax(1.0, static_cast<qreal>(_count - 1)));
}

qreal ShadCandlestickChartPrivate::visibleUnits() const
{
    return _viewSpan + 1.0;
}

ShadCandlestickChartPrivate::PlotGeometry ShadCandlestickChartPrivate::buildGeometry() const
{
    PlotGeometry geometry;
    const QRect plot = physicalPlotRect();
    if (plot.isEmpty() || _count == 0)
    {
        return geometry;
    }
    // 坐标相对绘图层左上角
    const qreal originX = plot.left();
    const qreal originY = plot.top();

    // 网格线限制在所在区域内
    if (_isGridVisible)
    {
        const qreal lineWidth = ShadChartHelper::kGridLineWidth;
        // 网格线不随窗口特效变透明
        geometry.gridColor = ShadStyleHelper::withAlpha(shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Border), 0.5);
        const auto appendGridLine = [&geometry, &plot, lineWidth, originY](qreal y, const QRect& pane)
        {
            const qreal paneTop = pane.top() - originY;
            const qreal center = qBound(paneTop + lineWidth / 2.0, std::floor(y - originY) + lineWidth / 2.0, paneTop + pane.height() - lineWidth / 2.0);
            geometry.gridLines.append(QRectF(0.0, center - lineWidth / 2.0, plot.width(), lineWidth));
        };
        for (const qreal tick : std::as_const(_priceTicks))
        {
            appendGridLine(priceY(tick), _priceRect);
        }
        if (_isVolumeVisible && !_volumeRect.isEmpty())
        {
            for (const qreal tick : std::as_const(_volumeTicks))
            {
                appendGridLine(volumeY(tick), _volumeRect);
            }
        }
    }
    if (_lastVisible - _firstVisible + 1 > _plotRect.width())
    {
        buildDecimatedCandles(geometry, originX, originY);
    }
    else
    {
        buildCandles(geometry, originX, originY);
    }
    return geometry;
}

void ShadCandlestickChartPrivate::buildCandles(PlotGeometry& geometry, qreal originX, qreal originY) const
{
    if (_plotRect.width() <= 0)
    {
        return;
    }
    // 实体宽度取奇数像素，影线居中
    const qreal bandSize = _plotRect.width() / visibleUnits();
    int bodyWidth = qMax(1, static_cast<int>(std::floor(bandSize * (1.0 - 2.0 * kCandleGap))));
    if (bodyWidth > 1 && bodyWidth % 2 == 0)
    {
        --bodyWidth;
    }
    const qreal halfBody = (bodyWidth - 1) / 2;
    const bool hasVolume = _isVolumeVisible && !_volumeRect.isEmpty();
    const qreal volumeBase = volumeY(0.0) - originY;
    QList<PlotRect> volumes;
    QList<PlotRect> wicks;
    QList<PlotRect> bodies;
    const qsizetype candleCount = _lastVisible - _firstVisible + 1;
    wicks.reserve(candleCount);
    bodies.reserve(candleCount);
    if (hasVolume)
    {
        volumes.reserve(candleCount);
    }
    for (int index = _firstVisible; index <= _lastVisible; ++index)
    {
        const ShadCandle& candle = _candles.at(index);
        const QColor color = candleColor(candle.open, candle.close);
        // RTL 时先镜像再取整，实体左右对称
        const qreal wickLeft = std::floor(physicalX(candleX(index)) - originX);
        const qreal bodyLeft = wickLeft - halfBody;
        const qreal highY = priceY(candle.high) - originY;
        const qreal lowY = priceY(candle.low) - originY;
        wicks.append({QRectF(wickLeft, qMin(highY, lowY), 1.0, qMax(1.0, qAbs(lowY - highY))), color});
        // 开盘等于收盘时画高 1 的横线
        const qreal openY = priceY(candle.open) - originY;
        const qreal closeY = priceY(candle.close) - originY;
        bodies.append({QRectF(bodyLeft, qMin(openY, closeY), bodyWidth, qMax(1.0, qAbs(closeY - openY))), color});
        if (hasVolume)
        {
            const qreal volumeTop = volumeY(candle.volume) - originY;
            if (volumeBase - volumeTop >= 1e-6)
            {
                volumes.append({QRectF(bodyLeft, volumeTop, bodyWidth, volumeBase - volumeTop), ShadStyleHelper::withAlpha(color, kVolumeOpacity)});
            }
        }
    }
    geometry.rects.reserve(volumes.size() + wicks.size() + bodies.size());
    geometry.rects.append(volumes);
    geometry.rects.append(wicks);
    geometry.rects.append(bodies);
    buildMovingAverages(geometry, originX, originY, QList<int>());
}

void ShadCandlestickChartPrivate::buildDecimatedCandles(PlotGeometry& geometry, qreal originX, qreal originY) const
{
    const int columns = _plotRect.width();
    QList<Column> merged(columns);
    for (int index = _firstVisible; index <= _lastVisible; ++index)
    {
        const int column = static_cast<int>(std::floor(candleX(index) - _plotRect.left()));
        if (column < 0 || column >= columns)
        {
            continue;
        }
        const ShadCandle& candle = _candles.at(index);
        Column& item = merged[column];
        if (item.first < 0)
        {
            item.first = index;
            item.open = candle.open;
            item.high = candle.high;
            item.low = candle.low;
            item.volume = candle.volume;
        }
        else
        {
            item.high = qMax(item.high, candle.high);
            item.low = qMin(item.low, candle.low);
            item.volume = qMax(item.volume, candle.volume);
        }
        item.last = index;
        item.close = candle.close;
    }
    // 每列宽 1，只画最高到最低
    const bool hasVolume = _isVolumeVisible && !_volumeRect.isEmpty();
    const qreal volumeBase = volumeY(0.0) - originY;
    QList<PlotRect> volumes;
    QList<PlotRect> wicks;
    QList<int> lastIndices;
    wicks.reserve(columns);
    lastIndices.reserve(columns);
    for (int column = 0; column < columns; ++column)
    {
        const Column& item = merged.at(column);
        if (item.first < 0)
        {
            continue;
        }
        const qreal x0 = physicalX(_plotRect.left() + column) - originX;
        const qreal x1 = physicalX(_plotRect.left() + column + 1) - originX;
        const qreal left = qMin(x0, x1);
        const QColor color = candleColor(item.open, item.close);
        const qreal highY = priceY(item.high) - originY;
        const qreal lowY = priceY(item.low) - originY;
        wicks.append({QRectF(left, qMin(highY, lowY), 1.0, qMax(1.0, qAbs(lowY - highY))), color});
        if (hasVolume)
        {
            const qreal volumeTop = volumeY(item.volume) - originY;
            if (volumeBase - volumeTop >= 1e-6)
            {
                volumes.append({QRectF(left, volumeTop, 1.0, volumeBase - volumeTop), ShadStyleHelper::withAlpha(color, kVolumeOpacity)});
            }
        }
        lastIndices.append(item.last);
    }
    geometry.rects.reserve(volumes.size() + wicks.size());
    geometry.rects.append(volumes);
    geometry.rects.append(wicks);
    // 均线每列取最后一根 K 线的值
    buildMovingAverages(geometry, originX, originY, lastIndices);
}

void ShadCandlestickChartPrivate::buildMovingAverages(PlotGeometry& geometry, qreal originX, qreal originY, const QList<int>& indices) const
{
    // 两侧多取的点可能超出价格范围，限制在价格区内
    const qreal priceTop = _priceRect.top() - originY;
    const qreal priceBottom = priceTop + _priceRect.height();
    for (int average = 0; average < _averageValues.size(); ++average)
    {
        const QList<qreal>& values = _averageValues.at(average);
        PlotLine line;
        line.color = movingAverageColor(average);
        const auto appendPoint = [&line, &values, this, originX, originY, priceTop, priceBottom](int index)
        {
            const qreal value = values.value(index, kNaN);
            if (std::isnan(value))
            {
                return;
            }
            line.points.append(QPointF(physicalX(candleX(index)) - originX, qBound(priceTop, priceY(value) - originY, priceBottom)));
        };
        if (indices.isEmpty())
        {
            for (int index = _firstVisible; index <= _lastVisible; ++index)
            {
                appendPoint(index);
            }
        }
        else
        {
            for (const int index : indices)
            {
                appendPoint(index);
            }
        }
        if (line.points.size() >= 2)
        {
            geometry.lines.append(line);
        }
    }
}

#if defined(SHADCN_CHART_GPU)
QList<ShadChartVertex> ShadCandlestickChartPrivate::buildVertices(const PlotGeometry& geometry) const
{
    QList<ShadChartVertex> vertices;
    vertices.reserve((geometry.gridLines.size() + geometry.rects.size()) * 6);
    for (const QRectF& line : geometry.gridLines)
    {
        ShadChartHelper::appendRect(vertices, line, geometry.gridColor);
    }
    for (const PlotRect& item : geometry.rects)
    {
        ShadChartHelper::appendRect(vertices, item.rect, item.color);
    }
    for (const PlotLine& line : geometry.lines)
    {
        ShadChartHelper::appendPolyline(vertices, line.points, kMovingAverageWidth, line.color);
    }
    return vertices;
}
#endif

void ShadCandlestickChartPrivate::paintAxes(QPainter* painter) const
{
    const ShadChart* q = q_ptr;
    const Qt::LayoutDirection direction = q->layoutDirection();
    const QRectF bounds(q->rect());
    const QFont font = textFont();
    painter->setFont(font);
    painter->setPen(shadTheme->getColor(ShadThemeType::MutedForeground));

    if (_isYAxisVisible && !_plotRect.isEmpty())
    {
        const qreal labelLeft = _plotRect.left() + _plotRect.width() + ShadChartHelper::kTickMargin;
        const qreal labelWidth = qMax(0.0, q->width() - ShadChartHelper::kHorizontalMargin - labelLeft);
        const Qt::Alignment alignment = ShadStyleHelper::visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter);
        for (const qreal tick : std::as_const(_priceTicks))
        {
            const QRectF logicalRect(labelLeft, priceY(tick) - ShadChartHelper::kTickTextHeight / 2.0, labelWidth, ShadChartHelper::kTickTextHeight);
            painter->drawText(ShadStyleHelper::visualRect(direction, bounds, logicalRect), alignment | Qt::TextDontClip, ShadChartHelper::formatValue(tick));
        }
        // 成交量区只标最大值，顶对齐，避免与价格刻度重叠
        if (_isVolumeVisible && !_volumeRect.isEmpty())
        {
            const QRectF logicalRect(labelLeft, _volumeRect.top(), labelWidth, ShadChartHelper::kTickTextHeight);
            painter->drawText(ShadStyleHelper::visualRect(direction, bounds, logicalRect), ShadStyleHelper::visualAlignment(direction, Qt::AlignLeft | Qt::AlignTop) | Qt::TextDontClip, ShadChartHelper::formatValue(_volumeMax));
        }
    }

    if (_isXAxisVisible)
    {
        const qreal textTop = _plotRect.top() + _plotRect.height() + ShadChartHelper::kTickMargin;
        for (qsizetype index = 0; index < _xTickIndices.size() && index < _xTickCenters.size(); ++index)
        {
            const QString text = candleLabel(_xTickIndices.at(index));
            const qreal textWidth = ShadStyleHelper::textWidth(font, text);
            const qreal x = physicalX(_xTickCenters.at(index));
            painter->drawText(QRectF(x - textWidth / 2.0, textTop, textWidth, ShadChartHelper::kTickTextHeight), Qt::AlignCenter | Qt::TextDontClip, text);
        }
    }
}

void ShadCandlestickChartPrivate::paintCrosshair(QPainter* painter) const
{
    // 数据修改后到重建前，K 线数量可能少于上次布局
    if (_hoverIndex < 0 || _hoverIndex >= _count || _hoverIndex >= _candles.size() || _plotRect.isEmpty())
    {
        return;
    }
    const ShadChart* q = q_ptr;
    const Qt::LayoutDirection direction = q->layoutDirection();
    const QRect plot = physicalPlotRect();
    painter->save();
    QPen pen(shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Border), 1.0, Qt::CustomDashLine);
    pen.setDashPattern({kCrosshairDash, kCrosshairDash});
    painter->setPen(pen);
    const qreal lineX = std::floor(physicalX(candleX(_hoverIndex))) + 0.5;
    painter->drawLine(QPointF(lineX, plot.top()), QPointF(lineX, plot.top() + plot.height()));
    // 鼠标在价格区内时显示水平线与价格标签
    const qreal mouseY = _hoverPosition.y();
    if (_priceRect.height() > 0 && mouseY >= _priceRect.top() && mouseY <= _priceRect.top() + _priceRect.height())
    {
        const qreal lineY = std::floor(mouseY) + 0.5;
        painter->drawLine(QPointF(plot.left(), lineY), QPointF(plot.left() + plot.width(), lineY));
        if (_isYAxisVisible)
        {
            const qreal price = _priceMax - (mouseY - _priceRect.top()) / _priceRect.height() * (_priceMax - _priceMin);
            const QString text = formatPrice(price);
            const QFont font = textFont();
            const qreal tagWidth = ShadStyleHelper::textWidth(font, text) + 2.0 * kPriceTagPadding;
            // 超出组件时向内移动
            const qreal tagLeft = qMin(static_cast<qreal>(_plotRect.left() + _plotRect.width() + ShadChartHelper::kTickMargin - kPriceTagPadding), q->width() - tagWidth);
            const QRectF logicalTag(tagLeft, lineY - ShadChartHelper::kTickTextHeight / 2.0, tagWidth, ShadChartHelper::kTickTextHeight);
            const QRectF tag = ShadStyleHelper::visualRect(direction, QRectF(q->rect()), logicalTag);
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->setPen(Qt::NoPen);
            painter->setBrush(shadTheme->getColor(ShadThemeType::Foreground));
            painter->drawRoundedRect(tag, kPriceTagRadius, kPriceTagRadius);
            painter->setFont(font);
            painter->setPen(shadTheme->getColor(ShadThemeType::Background));
            painter->drawText(tag, Qt::AlignCenter, text);
        }
    }
    painter->restore();
}

void ShadCandlestickChartPrivate::paintTooltip(QPainter* painter) const
{
    if (_hoverIndex < 0 || _hoverIndex >= _count || _hoverIndex >= _candles.size())
    {
        return;
    }
    const ShadCandle& candle = _candles.at(_hoverIndex);
    // 第一根相对自身开盘价
    const qreal previousClose = _hoverIndex > 0 ? _candles.at(_hoverIndex - 1).close : candle.open;
    QString changeText = QStringLiteral("--");
    if (!qFuzzyIsNull(previousClose))
    {
        const qreal percent = (candle.close - previousClose) / previousClose * 100.0;
        changeText = (percent > 0.0 ? QStringLiteral("+") : QString()) + QLocale().toString(percent, 'f', 2) + QStringLiteral("%");
    }
    const QColor color = candleColor(candle.open, candle.close);
    QList<ShadChartHelper::TooltipRow> rows;
    rows.append({QCoreApplication::translate("ShadCandlestickChart", "开盘"), formatPrice(candle.open), color});
    rows.append({QCoreApplication::translate("ShadCandlestickChart", "最高"), formatPrice(candle.high), color});
    rows.append({QCoreApplication::translate("ShadCandlestickChart", "最低"), formatPrice(candle.low), color});
    rows.append({QCoreApplication::translate("ShadCandlestickChart", "收盘"), formatPrice(candle.close), color});
    rows.append({QCoreApplication::translate("ShadCandlestickChart", "涨跌幅"), changeText, color});
    if (_isVolumeVisible)
    {
        rows.append({QCoreApplication::translate("ShadCandlestickChart", "成交量"), ShadChartHelper::formatValue(candle.volume), color});
    }
    for (int average = 0; average < _averageValues.size() && average < _movingAverages.size(); ++average)
    {
        const qreal value = _averageValues.at(average).value(_hoverIndex, kNaN);
        if (!std::isnan(value))
        {
            rows.append({QStringLiteral("MA%1").arg(_movingAverages.at(average)), formatPrice(value), movingAverageColor(average)});
        }
    }
    ShadChartHelper::paintTooltip(painter, q_ptr, physicalX(candleX(_hoverIndex)), _hoverPosition.y(), candleLabel(_hoverIndex), rows, ShadChartType::Dot);
}

bool ShadCandlestickChartPrivate::isRising(qreal open, qreal close) const
{
    return close >= open;
}

QColor ShadCandlestickChartPrivate::candleColor(qreal open, qreal close) const
{
    const bool isRed = isRising(open, close) == (_colorScheme == ShadChartType::RedRising);
    return QColor::fromRgb(isRed ? kRedRgb : kGreenRgb);
}

QColor ShadCandlestickChartPrivate::movingAverageColor(int index) const
{
    return shadTheme->getColor(static_cast<ShadThemeType::ThemeColor>(ShadThemeType::Chart1 + index % kChartColorCount));
}

QString ShadCandlestickChartPrivate::formatPrice(qreal value) const
{
    return QLocale().toString(value, 'f', _pricePrecision);
}

QString ShadCandlestickChartPrivate::candleLabel(int index) const
{
    // 序号可能来自上次布局，越界时返回空文字
    if (index < 0 || index >= _candles.size())
    {
        return QString();
    }
    const QString& label = _candles.at(index).label;
    return label.isEmpty() ? QString::number(index + 1) : label;
}

QFont ShadCandlestickChartPrivate::textFont() const
{
    return q_ptr->font();
}

qreal ShadCandlestickChartPrivate::physicalX(qreal logicalX) const
{
    return q_ptr->layoutDirection() == Qt::RightToLeft ? q_ptr->width() - logicalX : logicalX;
}

qreal ShadCandlestickChartPrivate::candleX(int index) const
{
    return _plotRect.left() + static_cast<qreal>(_plotRect.width()) * (index - _viewStart + 0.5) / visibleUnits();
}

qreal ShadCandlestickChartPrivate::priceY(qreal price) const
{
    const qreal range = _priceMax - _priceMin;
    if (range <= 0.0)
    {
        return _priceRect.top() + _priceRect.height();
    }
    return _priceRect.top() + (_priceMax - price) / range * _priceRect.height();
}

qreal ShadCandlestickChartPrivate::volumeY(qreal volume) const
{
    if (_volumeMax <= 0.0)
    {
        return _volumeRect.top() + _volumeRect.height();
    }
    return _volumeRect.top() + (1.0 - volume / _volumeMax) * _volumeRect.height();
}

QRect ShadCandlestickChartPrivate::physicalPlotRect() const
{
    return ShadStyleHelper::visualRect(q_ptr->layoutDirection(), q_ptr->rect(), _plotRect);
}
