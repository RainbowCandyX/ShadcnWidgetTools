#include "ShadScatterChartPrivate.h"

#include <QFont>
#include <QList>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <cmath>
#include <utility>

#include "ShadChart.h"
#include "ShadChartHelper.h"
#include "ShadChartOverlay.h"
#if defined(SHADCN_CHART_GPU)
#include "ShadChartView.h"
#endif
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 不限数量时缓冲的最小容量
    constexpr int kMinGrowCapacity = 64;
    // 悬停点半径比普通点大 2，外圈为表面颜色
    constexpr qreal kHoverRadiusExtra = 2.0;
    constexpr qreal kHoverRingWidth = 2.0;
} // namespace

ShadScatterChartPrivate::ShadScatterChartPrivate()
{
}

ShadScatterChartPrivate::~ShadScatterChartPrivate()
{
}

void ShadScatterChartPrivate::updateChart()
{
    _isUpdatePending = false;
    computeLayout();
    const QRect plot = physicalPlotRect();
    placePlotView(plot, !plot.isEmpty());
#if defined(SHADCN_CHART_GPU)
    if (_view != nullptr)
    {
        QList<ShadChartVertex> vertices;
        // 网格线不随窗口特效变透明
        const QColor gridColor = ShadStyleHelper::withAlpha(shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Border), 0.5);
        for (const QRectF& line : gridLines())
        {
            ShadChartHelper::appendRect(vertices, line, gridColor);
        }
        _view->setVertices(vertices);
        syncPoints();
    }
#endif
    // CPU 层每次绘制全部点
    if (_softwareView != nullptr)
    {
        for (Series& item : _series)
        {
            item.pendingAppend = 0;
        }
    }
    // 数据变化后按鼠标位置重新查找
    const bool hadHover = _hoverSeries >= 0;
    if (hadHover && !findPoint(_hoverPosition, _hoverSeries, _hoverIndex))
    {
        _hoverSeries = -1;
        _hoverIndex = -1;
    }
    updateOverlay(plot, hadHover || _hoverSeries >= 0);
}

void ShadScatterChartPrivate::paintPlot(QPainter* painter) const
{
    const QColor gridColor = ShadStyleHelper::withAlpha(shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Border), 0.5);
    for (const QRectF& line : gridLines())
    {
        painter->fillRect(line, gridColor);
    }
    painter->setRenderHint(QPainter::Antialiasing, true);
    qreal scaleX = 0.0;
    qreal offsetX = 0.0;
    qreal scaleY = 0.0;
    qreal offsetY = 0.0;
    plotTransform(scaleX, offsetX, scaleY, offsetY);
    for (int series = 0; series < _series.size(); ++series)
    {
        const QList<QPointF>& points = _series.at(series).points;
        QPolygonF polygon;
        polygon.reserve(points.size());
        for (const QPointF& point : points)
        {
            polygon.append(QPointF((point.x() - _origin.x()) * scaleX + offsetX, (point.y() - _origin.y()) * scaleY + offsetY));
        }
        // 圆头画笔的点即直径为线宽的圆
        painter->setPen(QPen(seriesColor(series), 2.0 * _pointRadius, Qt::SolidLine, Qt::RoundCap));
        painter->drawPoints(polygon);
    }
}

void ShadScatterChartPrivate::paintOverlay(QPainter* painter)
{
    paintAxes(painter);
    if (_isLegendVisible)
    {
        QList<ShadChartHelper::LegendItem> items;
        for (int series = 0; series < _series.size(); ++series)
        {
            items.append({_series.at(series).label, seriesColor(series), ShadIconType::None});
        }
        ShadChartHelper::paintLegend(painter, q_ptr, items);
    }
    if (_hoverSeries < 0 || _hoverSeries >= _series.size() || _hoverIndex < 0 || _hoverIndex >= _series.at(_hoverSeries).points.size())
    {
        return;
    }
    qreal scaleX = 0.0;
    qreal offsetX = 0.0;
    qreal scaleY = 0.0;
    qreal offsetY = 0.0;
    plotTransform(scaleX, offsetX, scaleY, offsetY);
    const QPointF point = _series.at(_hoverSeries).points.at(_hoverIndex);
    const QRect plot = physicalPlotRect();
    const QPointF center(plot.left() + (point.x() - _origin.x()) * scaleX + offsetX, plot.top() + (point.y() - _origin.y()) * scaleY + offsetY);
    const qreal radius = qMax(ShadChartHelper::kActiveDotRadius, _pointRadius + kHoverRadiusExtra);
    painter->save();
    painter->setPen(QPen(surfaceColor(), kHoverRingWidth));
    painter->setBrush(seriesColor(_hoverSeries));
    painter->drawEllipse(center, radius, radius);
    painter->restore();
    const QList<ShadChartHelper::TooltipRow> rows{{QStringLiteral("X"), ShadChartHelper::formatValue(point.x()), QColor(), ShadIconType::None}, {QStringLiteral("Y"), ShadChartHelper::formatValue(point.y()), QColor(), ShadIconType::None}};
    ShadChartHelper::paintTooltip(painter, q_ptr, center.x(), _hoverPosition.y(), _series.at(_hoverSeries).label, rows, ShadChartType::Dot);
}

void ShadScatterChartPrivate::updateHover(const QPointF& position)
{
    int series = -1;
    int index = -1;
    if (!findPoint(position, series, index))
    {
        clearHover();
        return;
    }
    _hoverSeries = series;
    _hoverIndex = index;
    _hoverPosition = position;
    _overlay->update();
}

void ShadScatterChartPrivate::clearHover()
{
    if (_hoverSeries < 0)
    {
        return;
    }
    _hoverSeries = -1;
    _hoverIndex = -1;
    _overlay->update();
}

QColor ShadScatterChartPrivate::seriesColor(int series) const
{
    const Series& item = _series.at(series);
    return item.customColor.isValid() ? item.customColor : shadTheme->getColor(item.colorRole);
}

void ShadScatterChartPrivate::plotViewCreated()
{
    for (Series& item : _series)
    {
        item.isFullDirty = true;
    }
}

void ShadScatterChartPrivate::computeLayout()
{
    const ShadChart* q = q_ptr;
    bool hasPoint = false;
    qreal minX = 0.0;
    qreal maxX = 1.0;
    qreal minY = 0.0;
    qreal maxY = 1.0;
    for (const Series& item : std::as_const(_series))
    {
        for (const QPointF& point : item.points)
        {
            if (!hasPoint)
            {
                minX = maxX = point.x();
                minY = maxY = point.y();
                hasPoint = true;
                continue;
            }
            minX = qMin(minX, point.x());
            maxX = qMax(maxX, point.x());
            minY = qMin(minY, point.y());
            maxY = qMax(maxY, point.y());
        }
    }
    // 原点取数据范围中心，远离后重选并整组重写
    if (hasPoint)
    {
        const QPointF center((minX + maxX) / 2.0, (minY + maxY) / 2.0);
        const bool isFarX = qMax(qAbs(minX - _origin.x()), qAbs(maxX - _origin.x())) > kRebaseRatio * (maxX - minX);
        const bool isFarY = qMax(qAbs(minY - _origin.y()), qAbs(maxY - _origin.y())) > kRebaseRatio * (maxY - minY);
        if (!_hasOrigin || isFarX || isFarY)
        {
            _origin = center;
            _hasOrigin = true;
            for (Series& item : _series)
            {
                item.isFullDirty = true;
            }
        }
    }
    _xTicks = ShadChartHelper::niceTicks(minX, maxX, _xTickCount);
    _yTicks = ShadChartHelper::niceTicks(minY, maxY, _yTickCount);
    _xMin = _xTicks.first();
    _xMax = _xTicks.last();
    _yMin = _yTicks.first();
    _yMax = _yTicks.last();

    // 绘图区：去掉外边距、图例、X 轴与 Y 轴占用的区域
    const QFont font = textFont();
    int left = ShadChartHelper::kHorizontalMargin;
    const int right = q->width() - ShadChartHelper::kHorizontalMargin;
    const int top = ShadChartHelper::kVerticalMargin;
    int bottom = q->height() - ShadChartHelper::kVerticalMargin;
    if (_isLegendVisible && !_series.isEmpty())
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
        for (const qreal tick : std::as_const(_yTicks))
        {
            labelWidth = qMax(labelWidth, ShadStyleHelper::textWidth(font, ShadChartHelper::formatValue(tick)));
        }
        left += labelWidth + ShadChartHelper::kTickMargin;
    }
    _plotRect = QRect(left, top, qMax(0, right - left), qMax(0, bottom - top));

    _xTickIndices.clear();
    if (_isXAxisVisible)
    {
        QList<qreal> centers;
        QList<qreal> widths;
        for (const qreal tick : std::as_const(_xTicks))
        {
            centers.append(valueX(tick));
            widths.append(ShadStyleHelper::textWidth(font, ShadChartHelper::formatValue(tick)));
        }
        _xTickIndices = ShadChartHelper::spacedLabels(centers, widths, ShadChartHelper::kMinTickGap);
    }
}

QList<QRectF> ShadScatterChartPrivate::gridLines() const
{
    QList<QRectF> lines;
    if (!_isGridVisible || _plotRect.isEmpty())
    {
        return lines;
    }
    // stroke-border/50，1px，对齐到像素中心
    const qreal lineWidth = ShadChartHelper::kGridLineWidth;
    const qreal maxCenter = _plotRect.height() - lineWidth / 2.0;
    for (const qreal tick : std::as_const(_yTicks))
    {
        const qreal center = qBound(lineWidth / 2.0, std::floor(valueY(tick) - _plotRect.top()) + lineWidth / 2.0, maxCenter);
        lines.append(QRectF(0.0, center - lineWidth / 2.0, _plotRect.width(), lineWidth));
    }
    return lines;
}

#if defined(SHADCN_CHART_GPU)
void ShadScatterChartPrivate::syncPoints()
{
    qreal scaleX = 0.0;
    qreal offsetX = 0.0;
    qreal scaleY = 0.0;
    qreal offsetY = 0.0;
    plotTransform(scaleX, offsetX, scaleY, offsetY);
    _view->setPointTransform(scaleX, offsetX, scaleY, offsetY, _pointRadius);
    _view->setPointGroupCount(static_cast<int>(_series.size()));
    for (int series = 0; series < _series.size(); ++series)
    {
        Series& item = _series[series];
        const QColor color = seriesColor(series);
        const int size = static_cast<int>(item.points.size());
        // points[first, first + count) 写到缓冲的 slot 处
        const auto writeRange = [this, series, &item, &color](int first, int count, int slot)
        {
            QList<ShadChartVertex> vertices;
            vertices.reserve(count);
            for (int i = first; i < first + count; ++i)
            {
                const QPointF& point = item.points.at(i);
                vertices.append(ShadChartHelper::makeVertex(QPointF(point.x() - _origin.x(), point.y() - _origin.y()), color, 1.0));
            }
            _view->writePoints(series, slot, vertices.constData(), count);
        };
        bool isFull = item.isFullDirty || color != item.gpuColor;
        if (!isFull && item.pendingAppend > 0)
        {
            const int appended = item.pendingAppend;
            const bool isRing = item.ringMaxCount > 0 && item.gpuCapacity == item.ringMaxCount;
            if (isRing && appended < item.gpuCapacity && appended <= size)
            {
                // 接在最后一个点之后，超出容量从开头继续，覆盖的正是被移除的点
                const int capacity = item.gpuCapacity;
                const int writeStart = (item.gpuHead + item.gpuCount) % capacity;
                const int firstLength = qMin(appended, capacity - writeStart);
                writeRange(size - appended, firstLength, writeStart);
                if (appended > firstLength)
                {
                    writeRange(size - appended + firstLength, appended - firstLength, 0);
                }
                const int removed = item.gpuCount + appended - size;
                item.gpuHead = (item.gpuHead + removed) % capacity;
                item.gpuCount = size;
            }
            else if (!isRing && item.gpuHead == 0 && item.gpuCount + appended == size && size <= item.gpuCapacity)
            {
                // 不限数量且容量足够时接在末尾
                writeRange(item.gpuCount, appended, item.gpuCount);
                item.gpuCount = size;
            }
            else
            {
                isFull = true;
            }
        }
        if (isFull)
        {
            // 环形缓冲容量为 maxCount；不限数量时预留一半余量
            const int capacity = item.ringMaxCount > 0 ? qMax(item.ringMaxCount, size) : qMax(kMinGrowCapacity, size + size / 2);
            _view->resetPointGroup(series, capacity);
            if (size > 0)
            {
                writeRange(0, size, 0);
            }
            item.gpuHead = 0;
            item.gpuCount = size;
            item.gpuCapacity = capacity;
        }
        _view->setPointRange(series, item.gpuHead, item.gpuCount);
        item.pendingAppend = 0;
        item.isFullDirty = false;
        item.gpuColor = color;
    }
}
#endif

void ShadScatterChartPrivate::plotTransform(qreal& scaleX, qreal& offsetX, qreal& scaleY, qreal& offsetY) const
{
    const qreal width = _plotRect.width();
    const qreal height = _plotRect.height();
    // 刻度保证范围大于 0
    scaleX = width / (_xMax - _xMin);
    offsetX = (_origin.x() - _xMin) * scaleX;
    if (q_ptr->layoutDirection() == Qt::RightToLeft)
    {
        scaleX = -scaleX;
        offsetX = width - offsetX;
    }
    scaleY = -height / (_yMax - _yMin);
    offsetY = (_yMax - _origin.y()) * height / (_yMax - _yMin);
}

bool ShadScatterChartPrivate::findPoint(const QPointF& position, int& series, int& index) const
{
    const QRect plot = physicalPlotRect();
    if (!_isTooltipVisible || plot.isEmpty() || !QRectF(plot).contains(position))
    {
        return false;
    }
    qreal scaleX = 0.0;
    qreal offsetX = 0.0;
    qreal scaleY = 0.0;
    qreal offsetY = 0.0;
    plotTransform(scaleX, offsetX, scaleY, offsetY);
    const qreal localX = position.x() - plot.left();
    const qreal localY = position.y() - plot.top();
    const qreal maxDistance = qMax(kHoverDistance, _pointRadius + kHoverRadiusExtra);
    qreal bestDistance = maxDistance * maxDistance;
    bool isFound = false;
    for (int s = 0; s < _series.size(); ++s)
    {
        const QList<QPointF>& points = _series.at(s).points;
        for (qsizetype i = 0; i < points.size(); ++i)
        {
            const qreal dx = (points.at(i).x() - _origin.x()) * scaleX + offsetX - localX;
            const qreal dy = (points.at(i).y() - _origin.y()) * scaleY + offsetY - localY;
            const qreal distance = dx * dx + dy * dy;
            // 距离相同时取后画的点（显示在上面）
            if (distance <= bestDistance)
            {
                bestDistance = distance;
                series = s;
                index = static_cast<int>(i);
                isFound = true;
            }
        }
    }
    return isFound;
}

void ShadScatterChartPrivate::paintAxes(QPainter* painter) const
{
    const ShadChart* q = q_ptr;
    const Qt::LayoutDirection direction = q->layoutDirection();
    const QRectF bounds(q->rect());
    const QFont font = textFont();
    painter->setFont(font);
    painter->setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
    if (_isYAxisVisible && !_plotRect.isEmpty())
    {
        const Qt::Alignment alignment = ShadStyleHelper::visualAlignment(direction, Qt::AlignRight | Qt::AlignVCenter);
        for (const qreal tick : std::as_const(_yTicks))
        {
            const QRectF logicalRect(0.0, valueY(tick) - ShadChartHelper::kTickTextHeight / 2.0, _plotRect.left() - ShadChartHelper::kTickMargin, ShadChartHelper::kTickTextHeight);
            painter->drawText(ShadStyleHelper::visualRect(direction, bounds, logicalRect), alignment | Qt::TextDontClip, ShadChartHelper::formatValue(tick));
        }
    }
    if (_isXAxisVisible)
    {
        const qreal textTop = _plotRect.top() + _plotRect.height() + ShadChartHelper::kTickMargin;
        for (const int index : std::as_const(_xTickIndices))
        {
            const QString text = ShadChartHelper::formatValue(_xTicks.at(index));
            const qreal textWidth = ShadStyleHelper::textWidth(font, text);
            const qreal x = physicalX(valueX(_xTicks.at(index)));
            painter->drawText(QRectF(x - textWidth / 2.0, textTop, textWidth, ShadChartHelper::kTickTextHeight), Qt::AlignCenter | Qt::TextDontClip, text);
        }
    }
}

QFont ShadScatterChartPrivate::textFont() const
{
    return q_ptr->font();
}

qreal ShadScatterChartPrivate::physicalX(qreal logicalX) const
{
    return q_ptr->layoutDirection() == Qt::RightToLeft ? q_ptr->width() - logicalX : logicalX;
}

qreal ShadScatterChartPrivate::valueX(qreal value) const
{
    return _plotRect.left() + (value - _xMin) / (_xMax - _xMin) * _plotRect.width();
}

qreal ShadScatterChartPrivate::valueY(qreal value) const
{
    return _plotRect.top() + (_yMax - value) / (_yMax - _yMin) * _plotRect.height();
}

QRect ShadScatterChartPrivate::physicalPlotRect() const
{
    return ShadStyleHelper::visualRect(q_ptr->layoutDirection(), q_ptr->rect(), _plotRect);
}
