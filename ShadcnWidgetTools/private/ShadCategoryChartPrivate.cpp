#include "ShadCategoryChartPrivate.h"

#include <QFont>
#include <QFontMetricsF>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <utility>

#include "ShadChart.h"
#include "ShadChartOverlay.h"
#if defined(SHADCN_CHART_GPU)
#include "ShadChartView.h"
#endif
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 曲线每段最多采样的点数；按段宽每 2px 采样一次
    constexpr int kMaxSegmentSamples = 32;
    constexpr qreal kSamplePixelStep = 2.0;
    // 圆角每 1px 半径一段，2 到 8 段
    constexpr int kMinCornerSegments = 2;
    constexpr int kMaxCornerSegments = 8;

    // 渐变不透明度：t 为包围盒内的相对位置，色标在 5% 与 95%，两端线性外推
    qreal gradientOpacity(qreal t)
    {
        const qreal opacity = ShadCategoryChartPrivate::kGradientTopOpacity + (t - 0.05) / 0.9 * (ShadCategoryChartPrivate::kGradientBottomOpacity - ShadCategoryChartPrivate::kGradientTopOpacity);
        return qBound(0.0, opacity, 1.0) * ShadCategoryChartPrivate::kFillOpacity;
    }

    // 自然三次样条的贝塞尔控制点（与 d3 curveNatural 相同），values 至少 3 个
    void naturalControlPoints(const QList<qreal>& values, QList<qreal>& first, QList<qreal>& second)
    {
        const int n = values.size() - 1;
        QList<qreal> a(n, 0.0);
        QList<qreal> b(n, 0.0);
        QList<qreal> r(n, 0.0);
        a[0] = 0.0;
        b[0] = 2.0;
        r[0] = values.at(0) + 2.0 * values.at(1);
        for (int i = 1; i < n - 1; ++i)
        {
            a[i] = 1.0;
            b[i] = 4.0;
            r[i] = 4.0 * values.at(i) + 2.0 * values.at(i + 1);
        }
        a[n - 1] = 2.0;
        b[n - 1] = 7.0;
        r[n - 1] = 8.0 * values.at(n - 1) + values.at(n);
        for (int i = 1; i < n; ++i)
        {
            const qreal m = a.at(i) / b.at(i - 1);
            b[i] -= m;
            r[i] -= m * r.at(i - 1);
        }
        a[n - 1] = r.at(n - 1) / b.at(n - 1);
        for (int i = n - 2; i >= 0; --i)
        {
            a[i] = (r.at(i) - a.at(i + 1)) / b.at(i);
        }
        b[n - 1] = (values.at(n) + a.at(n - 1)) / 2.0;
        for (int i = 0; i < n - 1; ++i)
        {
            b[i] = 2.0 * values.at(i + 1) - a.at(i + 1);
        }
        first = a;
        second = b;
    }

    QPointF cubicBezier(const QPointF& p0, const QPointF& c1, const QPointF& c2, const QPointF& p1, qreal t)
    {
        const qreal u = 1.0 - t;
        return p0 * (u * u * u) + c1 * (3.0 * u * u * t) + c2 * (3.0 * u * t * t) + p1 * (t * t * t);
    }

    // 按曲线类型把数据点展开为折线；采样点数只取决于 X 坐标，上下边界的采样点一一对应
    QList<QPointF> sampleCurve(const QList<QPointF>& points, ShadChartType::CurveType curveType)
    {
        const int count = points.size();
        if (count < 2 || curveType == ShadChartType::Linear)
        {
            return points;
        }
        QList<QPointF> result;
        if (curveType == ShadChartType::Step)
        {
            // 与 d3 curveStep 相同：在相邻两点 X 的中点处跳变
            result.append(points.first());
            for (int i = 1; i < count; ++i)
            {
                const qreal middleX = (points.at(i - 1).x() + points.at(i).x()) / 2.0;
                result.append(QPointF(middleX, points.at(i - 1).y()));
                result.append(QPointF(middleX, points.at(i).y()));
            }
            result.append(points.last());
            return result;
        }
        if (count == 2)
        {
            return points;
        }
        QList<qreal> xs;
        QList<qreal> ys;
        xs.reserve(count);
        ys.reserve(count);
        for (const QPointF& point : points)
        {
            xs.append(point.x());
            ys.append(point.y());
        }
        QList<qreal> firstX;
        QList<qreal> secondX;
        QList<qreal> firstY;
        QList<qreal> secondY;
        naturalControlPoints(xs, firstX, secondX);
        naturalControlPoints(ys, firstY, secondY);
        result.append(points.first());
        for (int i = 0; i < count - 1; ++i)
        {
            const QPointF c1(firstX.at(i), firstY.at(i));
            const QPointF c2(secondX.at(i), secondY.at(i));
            const qreal segmentWidth = qAbs(points.at(i + 1).x() - points.at(i).x());
            const int samples = qBound(1, static_cast<int>(std::ceil(segmentWidth / kSamplePixelStep)), kMaxSegmentSamples);
            for (int step = 1; step <= samples; ++step)
            {
                result.append(cubicBezier(points.at(i), c1, c2, points.at(i + 1), static_cast<qreal>(step) / samples));
            }
        }
        return result;
    }

    // 柱子轮廓：上方圆角 topRadius、下方圆角 bottomRadius 的凸多边形
    QPolygonF roundedBar(const QRectF& rect, qreal topRadius, qreal bottomRadius)
    {
        // 半径不超过宽度的一半，上下半径之和不超过高度
        qreal top = qBound(0.0, topRadius, rect.width() / 2.0);
        qreal bottom = qBound(0.0, bottomRadius, rect.width() / 2.0);
        if (top + bottom > rect.height() && top + bottom > 0.0)
        {
            const qreal scale = rect.height() / (top + bottom);
            top *= scale;
            bottom *= scale;
        }
        QPolygonF polygon;
        // 从 startDegrees 顺时针转 90 度（Y 轴向下）
        const auto appendCorner = [&polygon](const QPointF& center, qreal radius, qreal startDegrees)
        {
            if (radius <= 1e-6)
            {
                polygon.append(center);
                return;
            }
            const int segments = qBound(kMinCornerSegments, static_cast<int>(std::ceil(radius)), kMaxCornerSegments);
            for (int i = 0; i <= segments; ++i)
            {
                const qreal angle = qDegreesToRadians(startDegrees + 90.0 * i / segments);
                polygon.append(center + QPointF(std::cos(angle), std::sin(angle)) * radius);
            }
        };
        appendCorner(QPointF(rect.left() + top, rect.top() + top), top, 180.0);
        appendCorner(QPointF(rect.right() - top, rect.top() + top), top, 270.0);
        appendCorner(QPointF(rect.right() - bottom, rect.bottom() - bottom), bottom, 0.0);
        appendCorner(QPointF(rect.left() + bottom, rect.bottom() - bottom), bottom, 90.0);
        return polygon;
    }
} // namespace

ShadCategoryChartPrivate::ShadCategoryChartPrivate(Mark mark)
    : _mark(mark)
{
}

ShadCategoryChartPrivate::~ShadCategoryChartPrivate()
{
}

void ShadCategoryChartPrivate::updateChart()
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
        // 数据点走点管线
        if (geometry.dots.isEmpty())
        {
            _view->setPointGroupCount(0);
        }
        else
        {
            QList<ShadChartVertex> dots;
            dots.reserve(geometry.dots.size());
            for (const PlotDot& dot : std::as_const(geometry.dots))
            {
                dots.append(ShadChartHelper::makeVertex(dot.center, dot.color, 1.0));
            }
            _view->setPointGroupCount(1);
            _view->setPointTransform(1.0, 0.0, 1.0, 0.0, geometry.dotRadius);
            _view->replacePoints(0, dots);
        }
    }
#endif
    if (_softwareView != nullptr)
    {
        _plotGeometry = std::move(geometry);
    }
    // 悬停中按鼠标位置重新计算分类：数据或可见范围变化后，提示仍对应鼠标下的数据
    const bool hadHover = _hoverIndex >= 0;
    if (hadHover)
    {
        _hoverIndex = categoryAt(_hoverPosition);
    }
    updateOverlay(plot, hadHover || _hoverIndex >= 0);
}

void ShadCategoryChartPrivate::paintPlot(QPainter* painter) const
{
    // 网格线已对齐像素，不开抗锯齿
    for (const QRectF& line : _plotGeometry.gridLines)
    {
        painter->fillRect(line, _plotGeometry.gridColor);
    }
    painter->setRenderHint(QPainter::Antialiasing, true);
    for (const PlotArea& area : _plotGeometry.areas)
    {
        // 上边界去、下边界回；用非零环绕，边界交叉处也填充
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        path.moveTo(area.top.first());
        for (qsizetype i = 1; i < area.top.size(); ++i)
        {
            path.lineTo(area.top.at(i));
        }
        for (qsizetype i = area.bottom.size() - 1; i >= 0; --i)
        {
            path.lineTo(area.bottom.at(i));
        }
        path.closeSubpath();
        if (_fillStyle == ShadChartType::Gradient && area.boxHeight > 0.0)
        {
            // 两端取外推值，与 GPU 层逐顶点插值一致
            QLinearGradient gradient(QPointF(0.0, area.boxTop), QPointF(0.0, area.boxTop + area.boxHeight));
            gradient.setColorAt(0.0, ShadStyleHelper::withAlpha(area.color, gradientOpacity(0.0)));
            gradient.setColorAt(1.0, ShadStyleHelper::withAlpha(area.color, gradientOpacity(1.0)));
            painter->fillPath(path, gradient);
        }
        else
        {
            painter->fillPath(path, ShadStyleHelper::withAlpha(area.color, fillOpacity(area, area.boxTop)));
        }
        // 描边只画上边界
        painter->setPen(QPen(area.color, kStrokeWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawPolyline(QPolygonF(area.top));
    }
    for (const PlotLine& line : _plotGeometry.lines)
    {
        painter->setPen(QPen(line.color, _lineWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawPolyline(QPolygonF(line.points));
    }
    painter->setPen(Qt::NoPen);
    for (const PlotBar& bar : _plotGeometry.bars)
    {
        painter->setBrush(bar.color);
        painter->drawPolygon(bar.polygon);
    }
    for (const PlotDot& dot : _plotGeometry.dots)
    {
        painter->setBrush(dot.color);
        painter->drawEllipse(dot.center, _plotGeometry.dotRadius, _plotGeometry.dotRadius);
    }
}

void ShadCategoryChartPrivate::paintOverlay(QPainter* painter)
{
    paintAxes(painter);
    if (_isLegendVisible)
    {
        QList<ShadChartHelper::LegendItem> items;
        for (int series = 0; series < _series.size(); ++series)
        {
            items.append({_series.at(series).label, seriesColor(series), _series.at(series).icon});
        }
        ShadChartHelper::paintLegend(painter, q_ptr, items);
    }
    paintHover(painter);
    paintTooltip(painter);
}

void ShadCategoryChartPrivate::updateHover(const QPointF& position)
{
    const int index = categoryAt(position);
    if (index < 0)
    {
        clearHover();
        return;
    }
    _hoverIndex = index;
    _hoverPosition = position;
    _overlay->update();
}

void ShadCategoryChartPrivate::clearHover()
{
    if (_hoverIndex < 0)
    {
        return;
    }
    _hoverIndex = -1;
    _overlay->update();
}

bool ShadCategoryChartPrivate::zoomAt(const QPointF& position, int angleDelta)
{
    if (angleDelta == 0)
    {
        return false;
    }
    return zoomByFactor(position, std::pow(kZoomStepFactor, -angleDelta / 120.0));
}

bool ShadCategoryChartPrivate::zoomByFactor(const QPointF& position, qreal factor)
{
    const QRect plot = physicalPlotRect();
    if (!_isZoomEnabled || _count < 2 || factor <= 0.0 || qFuzzyCompare(factor, 1.0) || plot.isEmpty() || !QRectF(plot).contains(position))
    {
        return false;
    }
    const qreal fullSpan = _count - 1;
    const qreal minSpan = minVisibleSpan();
    // 分类太少无法放大，或已完全缩小时继续缩小：交给父组件滚动页面
    if (fullSpan <= minSpan || (factor < 1.0 && !_isZoomed))
    {
        return false;
    }
    // 鼠标位置在缩放前后不动；band 刻度偏移半个单位、宽度多一个单位
    const qreal bandOffset = isBandScale() ? 0.5 : 0.0;
    const qreal bandExtra = isBandScale() ? 1.0 : 0.0;
    const qreal fraction = qBound(0.0, (physicalX(position.x()) - _plotRect.left()) / _plotRect.width(), 1.0);
    const qreal anchor = _viewStart - bandOffset + fraction * (_viewSpan + bandExtra);
    const qreal span = qBound(minSpan, _viewSpan / factor, fullSpan);
    if (span >= fullSpan)
    {
        resetZoom();
    }
    else
    {
        _isZoomed = true;
        _viewSpan = span;
        _viewStart = qBound(0.0, anchor + bandOffset - fraction * (span + bandExtra), fullSpan - span);
        _isAnchoredToEnd = _viewStart + span >= fullSpan - 1e-6;
        updateChart();
    }
    updateHover(position);
    return true;
}

bool ShadCategoryChartPrivate::beginPan(const QPointF& position)
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

void ShadCategoryChartPrivate::panTo(const QPointF& position)
{
    if (!_isPanning || !_isZoomed || _plotRect.width() <= 0)
    {
        return;
    }
    // 内容跟随鼠标移动：向右拖动显示更早的分类，RTL 时方向相反
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

void ShadCategoryChartPrivate::endPan()
{
    _isPanning = false;
}

bool ShadCategoryChartPrivate::handleDoubleClick(const QPointF& position)
{
    if (!_isZoomEnabled || !_isZoomed)
    {
        return false;
    }
    resetZoom();
    updateHover(position);
    return true;
}

void ShadCategoryChartPrivate::resetZoom()
{
    _isZoomed = false;
    _isAnchoredToEnd = false;
    _isPanning = false;
    updateChart();
}

void ShadCategoryChartPrivate::shiftView(qreal count)
{
    if (!_isZoomed || _isAnchoredToEnd)
    {
        return;
    }
    // 可见范围移出开头后由 updateVisibleRange 限制在 0 以上；拖动中的起点同步前移，拖动不跳动
    // 悬停的分类在重建时按鼠标位置重新计算，仍指向同一条数据
    _viewStart -= count;
    _panStartView -= count;
}

int ShadCategoryChartPrivate::categoryCount() const
{
    if (!_categories.isEmpty())
    {
        return _categories.size();
    }
    int count = 0;
    for (const Series& series : _series)
    {
        count = qMax(count, static_cast<int>(series.values.size()));
    }
    return count;
}

int ShadCategoryChartPrivate::categoryAt(const QPointF& position) const
{
    const QRect plot = physicalPlotRect();
    if (!_isTooltipVisible || _count == 0 || plot.isEmpty() || !QRectF(plot).contains(position))
    {
        return -1;
    }
    if (_count == 1)
    {
        return 0;
    }
    // 镜像是对合变换，同一函数可把实际 X 转回逻辑 X
    const qreal logicalX = physicalX(position.x());
    const qreal bandOffset = isBandScale() ? 0.5 : 0.0;
    return qBound(0, qRound(_viewStart - bandOffset + (logicalX - _plotRect.left()) / _plotRect.width() * visibleUnits()), _count - 1);
}

void ShadCategoryChartPrivate::computeLayout()
{
    const ShadChart* q = q_ptr;
    _count = categoryCount();
    const int seriesCount = _series.size();

    // 堆叠：Stacked 依次累加；Expanded 再除以该分类的总和
    // 逐个系列连续写入，复用上次的数组，数据量大且每帧重建时不反复分配内存
    _tops.resize(seriesCount);
    _bottoms.resize(seriesCount);
    QList<qreal> cumulative(_count, 0.0);
    QList<qreal> totals;
    if (_stackMode == ShadChartType::Expanded)
    {
        totals.fill(0.0, _count);
        qreal* total = totals.data();
        for (int series = 0; series < seriesCount; ++series)
        {
            const QList<qreal>& values = _series.at(series).values;
            const qreal* source = values.constData();
            const int valueCount = static_cast<int>(qMin(values.size(), static_cast<qsizetype>(_count)));
            for (int category = 0; category < valueCount; ++category)
            {
                total[category] += source[category];
            }
        }
    }
    qreal* sum = cumulative.data();
    const qreal* total = totals.constData();
    for (int series = 0; series < seriesCount; ++series)
    {
        _tops[series].resize(_count);
        _bottoms[series].resize(_count);
        qreal* top = _tops[series].data();
        qreal* bottom = _bottoms[series].data();
        const QList<qreal>& values = _series.at(series).values;
        const qreal* source = values.constData();
        const int valueCount = static_cast<int>(qMin(values.size(), static_cast<qsizetype>(_count)));
        for (int category = 0; category < _count; ++category)
        {
            const qreal value = category < valueCount ? source[category] : 0.0;
            switch (_stackMode)
            {
                case ShadChartType::NoStack:
                    bottom[category] = 0.0;
                    top[category] = value;
                    break;
                case ShadChartType::Stacked:
                    bottom[category] = sum[category];
                    top[category] = sum[category] + value;
                    break;
                case ShadChartType::Expanded:
                    bottom[category] = qFuzzyIsNull(total[category]) ? 0.0 : sum[category] / total[category];
                    top[category] = qFuzzyIsNull(total[category]) ? 0.0 : (sum[category] + value) / total[category];
                    break;
            }
            sum[category] += value;
        }
    }
    updateVisibleRange();

    // Y 轴范围：百分比堆叠固定为 0 到 1，其余按可见范围内的数据从 0（或更小的负值）到最大值取整齐刻度
    _yTicks.clear();
    if (_stackMode == ShadChartType::Expanded)
    {
        for (int i = 0; i < _yTickCount; ++i)
        {
            _yTicks.append(ShadChartHelper::roundTick(static_cast<qreal>(i) / (_yTickCount - 1)));
        }
    }
    else
    {
        qreal minValue = 0.0;
        qreal maxValue = 0.0;
        const int firstCategory = qMax(0, static_cast<int>(std::floor(_viewStart)));
        const int lastCategory = qMin(_count - 1, static_cast<int>(std::ceil(_viewStart + _viewSpan)));
        for (int series = 0; series < seriesCount; ++series)
        {
            for (int category = firstCategory; category <= lastCategory; ++category)
            {
                minValue = qMin(minValue, qMin(_tops.at(series).at(category), _bottoms.at(series).at(category)));
                maxValue = qMax(maxValue, qMax(_tops.at(series).at(category), _bottoms.at(series).at(category)));
            }
        }
        _yTicks = ShadChartHelper::niceTicks(minValue, maxValue, _yTickCount);
    }
    _domainMin = _yTicks.first();
    _domainMax = _yTicks.last();

    // 绘图区：去掉外边距、图例、X 轴与 Y 轴占用的区域
    const QFont font = textFont();
    int left = ShadChartHelper::kHorizontalMargin;
    const int right = q->width() - ShadChartHelper::kHorizontalMargin;
    const int top = ShadChartHelper::kVerticalMargin;
    int bottom = q->height() - ShadChartHelper::kVerticalMargin;
    if (_isLegendVisible && seriesCount > 0)
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
            labelWidth = qMax(labelWidth, ShadStyleHelper::textWidth(font, formatTick(tick)));
        }
        left += labelWidth + ShadChartHelper::kTickMargin;
    }
    _plotRect = QRect(left, top, qMax(0, right - left), qMax(0, bottom - top));

    // X 轴刻度：相邻刻度文字至少间隔 kMinTickGap，放不下时每隔 step 个分类显示一个
    _xTickIndices.clear();
    _xTickCenters.clear();
    if (_isXAxisVisible && _count == 1)
    {
        _xTickIndices.append(0);
        _xTickCenters.append(categoryX(0));
    }
    else if (_isXAxisVisible && _count > 1 && _plotRect.width() > 0)
    {
        const QFontMetricsF metrics(font);
        // 可见范围内的整数分类，band 刻度两侧各多半个单位
        const qreal bandOffset = isBandScale() ? 0.5 : 0.0;
        const int firstTick = qMax(0, static_cast<int>(std::ceil(_viewStart - bandOffset - 1e-9)));
        const int lastTick = qMin(_count - 1, static_cast<int>(std::floor(_viewStart + _viewSpan + bandOffset + 1e-9)));
        const qreal spacing = _plotRect.width() / visibleUnits();
        const auto requiredStep = [spacing](qreal labelWidth) { return qMax(1, static_cast<int>(std::ceil((std::ceil(labelWidth) + ShadChartHelper::kMinTickGap) / spacing))); };
        // 未放大时与 Recharts 的 preserveEnd 相同，从最后一个分类开始每隔 step 个取一个；放大后取 step 的整数倍，平移时刻度不跳动
        const auto collectTicks = [this, firstTick, lastTick](int step)
        {
            QList<int> ticks;
            if (_isZoomed)
            {
                for (int category = (firstTick + step - 1) / step * step; category <= lastTick; category += step)
                {
                    ticks.append(category);
                }
            }
            else
            {
                for (int category = lastTick; category >= firstTick; category -= step)
                {
                    ticks.prepend(category);
                }
            }
            return ticks;
        };
        // 只测量会显示的刻度：先按最后一个刻度估算间隔，再检查选中刻度中最宽的文字，放不下时加大间隔，直到不再变化
        // 分类很多时（例如数万个）不必逐个测量全部文字
        int step = requiredStep(metrics.horizontalAdvance(tickLabelAt(lastTick)));
        QList<int> ticks;
        for (;;)
        {
            ticks = collectTicks(step);
            qreal maxLabelWidth = 0.0;
            for (const int category : std::as_const(ticks))
            {
                maxLabelWidth = qMax(maxLabelWidth, metrics.horizontalAdvance(tickLabelAt(category)));
            }
            const int required = requiredStep(maxLabelWidth);
            if (required <= step)
            {
                break;
            }
            step = required;
        }
        for (const int category : std::as_const(ticks))
        {
            const qreal center = categoryX(category);
            // 放大后刻度位置任意，文字超出组件两侧时不显示
            const qreal halfWidth = std::ceil(metrics.horizontalAdvance(tickLabelAt(category))) / 2.0;
            if (_isZoomed && (center - halfWidth < 0.0 || center + halfWidth > q->width()))
            {
                continue;
            }
            _xTickIndices.append(category);
            _xTickCenters.append(center);
        }
        // 与 Recharts 的 preserveEnd 相同：最后一个刻度的文字超出绘图区结束边时向内移动，移动后与前一个刻度的间距不足 kMinTickGap 时去掉前一个
        const qreal plotEnd = _plotRect.left() + _plotRect.width();
        const qreal tailWidth = _xTickIndices.isEmpty() ? 0.0 : std::ceil(metrics.horizontalAdvance(tickLabelAt(_xTickIndices.last())));
        if (!_isZoomed && !_xTickIndices.isEmpty() && _xTickCenters.last() + tailWidth / 2.0 > plotEnd && tailWidth < _plotRect.width())
        {
            _xTickCenters.last() = plotEnd - tailWidth / 2.0;
            if (_xTickIndices.size() > 1)
            {
                const qsizetype previous = _xTickIndices.size() - 2;
                const qreal previousWidth = std::ceil(metrics.horizontalAdvance(tickLabelAt(_xTickIndices.at(previous))));
                if (_xTickCenters.at(previous) + previousWidth / 2.0 + ShadChartHelper::kMinTickGap > _xTickCenters.last() - tailWidth / 2.0)
                {
                    _xTickIndices.removeAt(previous);
                    _xTickCenters.removeAt(previous);
                }
            }
        }
    }
}

ShadCategoryChartPrivate::PlotGeometry ShadCategoryChartPrivate::buildGeometry() const
{
    PlotGeometry geometry;
    const QRect plot = physicalPlotRect();
    if (plot.isEmpty() || _count == 0)
    {
        return geometry;
    }
    // 坐标相对绘图层的左上角
    const qreal originX = plot.left();
    const qreal originY = plot.top();

    // 水平网格线：stroke-border/50，1px，对齐到像素中心
    if (_isGridVisible)
    {
        const qreal lineWidth = ShadChartHelper::kGridLineWidth;
        // 网格线不随窗口特效变透明
        geometry.gridColor = ShadStyleHelper::withAlpha(shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Border), 0.5);
        const qreal maxCenter = plot.height() - lineWidth / 2.0;
        for (const qreal tick : std::as_const(_yTicks))
        {
            const qreal center = qBound(lineWidth / 2.0, std::floor(valueY(tick) - originY) + lineWidth / 2.0, maxCenter);
            geometry.gridLines.append(QRectF(0.0, center - lineWidth / 2.0, plot.width(), lineWidth));
        }
    }
    if (_mark == Bar)
    {
        buildBars(geometry, originX, originY);
    }
    else
    {
        buildCurves(geometry, originX, originY);
    }
    return geometry;
}

void ShadCategoryChartPrivate::buildCurves(PlotGeometry& geometry, qreal originX, qreal originY) const
{
    if (_count < 2)
    {
        return;
    }
    // 数据点过密时按像素列抽稀；抽稀后的点在像素内锯齿起伏，样条会过冲，改用折线连接
    // 只处理可见范围内的分类（含两侧各 kEdgePadding 个），超出绘图区的部分由绘图层裁掉
    const QList<int> indices = decimatedIndices();
    const bool isDecimated = !indices.isEmpty();
    const int pointCount = isDecimated ? static_cast<int>(indices.size()) : _lastVisible - _firstVisible + 1;
    if (pointCount < 2)
    {
        return;
    }
    const ShadChartType::CurveType curveType = isDecimated ? ShadChartType::Linear : _curveType;
    const bool hasDots = _mark == Line && _isDotsVisible && !isDecimated;
    geometry.dotRadius = kDotRadius + _lineWidth / 2.0;
    for (int series = 0; series < _series.size(); ++series)
    {
        QList<QPointF> topPoints;
        QList<QPointF> bottomPoints;
        topPoints.reserve(pointCount);
        bottomPoints.reserve(pointCount);
        for (int point = 0; point < pointCount; ++point)
        {
            const int category = isDecimated ? indices.at(point) : _firstVisible + point;
            const qreal x = physicalX(categoryX(category)) - originX;
            topPoints.append(QPointF(x, valueY(_tops.at(series).at(category)) - originY));
            bottomPoints.append(QPointF(x, valueY(_bottoms.at(series).at(category)) - originY));
        }
        const QColor color = seriesColor(series);
        if (_mark == Line)
        {
            geometry.lines.append({sampleCurve(topPoints, curveType), color});
            if (hasDots)
            {
                for (const QPointF& point : std::as_const(topPoints))
                {
                    geometry.dots.append({point, color});
                }
            }
            continue;
        }
        PlotArea area;
        area.top = sampleCurve(topPoints, curveType);
        area.bottom = sampleCurve(bottomPoints, curveType);
        if (area.top.size() != area.bottom.size())
        {
            continue;
        }
        area.color = color;
        // 渐变的范围为面积（上边界与下边界）的包围盒
        qreal boxTop = area.top.first().y();
        qreal boxBottom = boxTop;
        for (qsizetype i = 0; i < area.top.size(); ++i)
        {
            boxTop = qMin(boxTop, qMin(area.top.at(i).y(), area.bottom.at(i).y()));
            boxBottom = qMax(boxBottom, qMax(area.top.at(i).y(), area.bottom.at(i).y()));
        }
        area.boxTop = boxTop;
        area.boxHeight = boxBottom - boxTop;
        geometry.areas.append(area);
    }
}

void ShadCategoryChartPrivate::buildBars(PlotGeometry& geometry, qreal originX, qreal originY) const
{
    const int seriesCount = _series.size();
    if (seriesCount == 0 || _plotRect.width() <= 0)
    {
        return;
    }
    if (_lastVisible - _firstVisible + 1 > _plotRect.width())
    {
        buildDecimatedBars(geometry, originX, originY);
        return;
    }
    // 去掉两侧间隔后平分给各系列；堆叠时一个分类一根柱子
    const bool isStacked = _stackMode != ShadChartType::NoStack;
    const qreal bandSize = _plotRect.width() / visibleUnits();
    const qreal available = qMax(0.0, bandSize * (1.0 - 2.0 * kBarCategoryGap));
    const int barCount = isStacked ? 1 : seriesCount;
    qreal gap = barCount > 1 ? kBarGap : 0.0;
    qreal barSize = (available - (barCount - 1) * gap) / barCount;
    // 分类过窄时去掉柱间距；柱宽至少 1 物理像素，否则 MSAA 采样不到
    if (barSize < 1.0)
    {
        gap = 0.0;
        barSize = qMax(available / barCount, 1.0 / q_ptr->devicePixelRatioF());
    }
    for (int category = _firstVisible; category <= _lastVisible; ++category)
    {
        const qreal start = categoryX(category) - available / 2.0;
        for (int series = 0; series < seriesCount; ++series)
        {
            const qreal logicalLeft = start + (isStacked ? 0 : series) * (barSize + gap);
            const qreal x0 = physicalX(logicalLeft) - originX;
            const qreal x1 = physicalX(logicalLeft + barSize) - originX;
            const qreal y0 = valueY(_tops.at(series).at(category)) - originY;
            const qreal y1 = valueY(_bottoms.at(series).at(category)) - originY;
            const QRectF rect(qMin(x0, x1), qMin(y0, y1), qAbs(x1 - x0), qAbs(y1 - y0));
            if (rect.height() < 1e-6)
            {
                continue;
            }
            // 不堆叠时四角圆角；堆叠时第一个系列圆下方、最后一个系列圆上方
            const qreal topRadius = !isStacked || series == seriesCount - 1 ? _barRadius : 0.0;
            const qreal bottomRadius = !isStacked || series == 0 ? _barRadius : 0.0;
            geometry.bars.append({roundedBar(rect, topRadius, bottomRadius), seriesColor(series)});
        }
    }
}

void ShadCategoryChartPrivate::buildDecimatedBars(PlotGeometry& geometry, qreal originX, qreal originY) const
{
    const int seriesCount = _series.size();
    const int columns = _plotRect.width();
    const bool isStacked = _stackMode != ShadChartType::NoStack;
    // 每列保留的分类：堆叠时为整根柱子最高的分类，不堆叠时为每个系列各自最高的分类
    const int slotCount = isStacked ? 1 : seriesCount;
    QList<int> kept(columns * slotCount, -1);
    const auto barHeight = [this](int series, int category) { return qAbs(_tops.at(series).at(category) - _bottoms.at(series).at(category)); };
    const auto stackHeight = [this, seriesCount](int category) { return qAbs(_tops.at(seriesCount - 1).at(category) - _bottoms.at(0).at(category)); };
    for (int category = _firstVisible; category <= _lastVisible; ++category)
    {
        // 可见范围两侧多取的分类落在绘图区外，不参与合并
        const int column = static_cast<int>(std::floor(categoryX(category) - _plotRect.left()));
        if (column < 0 || column >= columns)
        {
            continue;
        }
        for (int slot = 0; slot < slotCount; ++slot)
        {
            int& current = kept[column * slotCount + slot];
            const qreal height = isStacked ? stackHeight(category) : barHeight(slot, category);
            if (current < 0 || height > (isStacked ? stackHeight(current) : barHeight(slot, current)))
            {
                current = category;
            }
        }
    }
    // 每列宽 1 个逻辑像素、不画圆角；不堆叠时各系列重叠，按高度从高到低画，矮的在上面
    QList<std::pair<qreal, int>> order;
    order.reserve(seriesCount);
    for (int column = 0; column < columns; ++column)
    {
        const qreal x0 = physicalX(_plotRect.left() + column) - originX;
        const qreal x1 = physicalX(_plotRect.left() + column + 1) - originX;
        const auto appendBar = [&](int series, int category)
        {
            const qreal y0 = valueY(_tops.at(series).at(category)) - originY;
            const qreal y1 = valueY(_bottoms.at(series).at(category)) - originY;
            const QRectF rect(qMin(x0, x1), qMin(y0, y1), qAbs(x1 - x0), qAbs(y1 - y0));
            if (rect.height() >= 1e-6)
            {
                geometry.bars.append({roundedBar(rect, 0.0, 0.0), seriesColor(series)});
            }
        };
        if (isStacked)
        {
            const int category = kept.at(column);
            for (int series = 0; series < seriesCount && category >= 0; ++series)
            {
                appendBar(series, category);
            }
            continue;
        }
        order.clear();
        for (int series = 0; series < seriesCount; ++series)
        {
            const int category = kept.at(column * slotCount + series);
            if (category >= 0)
            {
                order.append({barHeight(series, category), series});
            }
        }
        std::sort(order.begin(), order.end(), [](const std::pair<qreal, int>& a, const std::pair<qreal, int>& b) { return a.first > b.first; });
        for (const std::pair<qreal, int>& item : std::as_const(order))
        {
            appendBar(item.second, kept.at(column * slotCount + item.second));
        }
    }
}

qreal ShadCategoryChartPrivate::fillOpacity(const PlotArea& area, qreal y) const
{
    if (_fillStyle == ShadChartType::Solid)
    {
        return kFillOpacity;
    }
    if (area.boxHeight <= 0.0)
    {
        return kGradientTopOpacity * kFillOpacity;
    }
    return gradientOpacity((y - area.boxTop) / area.boxHeight);
}

#if defined(SHADCN_CHART_GPU)
QList<ShadChartVertex> ShadCategoryChartPrivate::buildVertices(const PlotGeometry& geometry) const
{
    QList<ShadChartVertex> vertices;
    for (const QRectF& line : geometry.gridLines)
    {
        ShadChartHelper::appendRect(vertices, line, geometry.gridColor);
    }
    for (const PlotArea& area : geometry.areas)
    {
        // 面积与描边各为每个采样点 2 个顶点，另加段间连接，预先分配避免反复扩容
        vertices.reserve(vertices.size() + area.top.size() * 4 + 4);
        // 面积：上下边界的采样点交替排列成三角形带
        ShadChartHelper::beginStrip(vertices, ShadChartHelper::makeVertex(area.top.first(), area.color, fillOpacity(area, area.top.first().y())));
        for (qsizetype i = 0; i < area.top.size(); ++i)
        {
            const QPointF& top = area.top.at(i);
            const QPointF& bottom = area.bottom.at(i);
            vertices.append(ShadChartHelper::makeVertex(top, area.color, fillOpacity(area, top.y())));
            vertices.append(ShadChartHelper::makeVertex(bottom, area.color, fillOpacity(area, bottom.y())));
        }
        // 描边只画上边界
        ShadChartHelper::appendPolyline(vertices, area.top, kStrokeWidth, area.color);
    }
    for (const PlotLine& line : geometry.lines)
    {
        ShadChartHelper::appendPolyline(vertices, line.points, _lineWidth, line.color);
    }
    for (const PlotBar& bar : geometry.bars)
    {
        ShadChartHelper::appendConvexPolygon(vertices, bar.polygon, bar.color);
    }
    return vertices;
}
#endif

QList<int> ShadCategoryChartPrivate::decimatedIndices() const
{
    QList<int> indices;
    const int columns = _plotRect.width();
    if (columns <= 0 || _lastVisible - _firstVisible + 1 <= columns * kDecimationPointsPerPixel)
    {
        return indices;
    }
    const int seriesCount = static_cast<int>(_tops.size());
    QList<int> minIndices(seriesCount, 0);
    QList<int> maxIndices(seriesCount, 0);
    QList<int> bucket;
    bucket.reserve(2 + 2 * seriesCount);
    indices.reserve(columns * (2 + 2 * seriesCount));
    // 可见范围两侧多取的分类落在绘图区外，列号为负数或不小于 columns，单独成列，不影响边缘列的取值
    int column = 0;
    bool hasColumn = false;
    int first = 0;
    int last = 0;
    // 一列内保留的分类按序号排列后追加，相邻列之间也保持递增
    const auto flushColumn = [&]()
    {
        bucket.clear();
        bucket.append(first);
        bucket.append(last);
        for (int series = 0; series < seriesCount; ++series)
        {
            bucket.append(minIndices.at(series));
            bucket.append(maxIndices.at(series));
        }
        std::sort(bucket.begin(), bucket.end());
        for (const int index : std::as_const(bucket))
        {
            if (indices.isEmpty() || indices.last() != index)
            {
                indices.append(index);
            }
        }
    };
    // 分类到像素列的比例，与 categoryX 的映射一致；预先算出，循环内不做除法
    const qreal columnScale = columns / _viewSpan;
    for (int category = _firstVisible; category <= _lastVisible; ++category)
    {
        // 分类所在的像素列；可见范围的最后一个分类恰好落在结束边上，归入最后一列
        int currentColumn = static_cast<int>(std::floor((category - _viewStart) * columnScale));
        if (currentColumn == columns && category <= _viewStart + _viewSpan + 1e-9)
        {
            currentColumn = columns - 1;
        }
        if (!hasColumn || currentColumn != column)
        {
            if (hasColumn)
            {
                flushColumn();
            }
            hasColumn = true;
            column = currentColumn;
            first = category;
            for (int series = 0; series < seriesCount; ++series)
            {
                minIndices[series] = category;
                maxIndices[series] = category;
            }
        }
        last = category;
        for (int series = 0; series < seriesCount; ++series)
        {
            const QList<qreal>& tops = _tops.at(series);
            const qreal value = tops.at(category);
            if (value < tops.at(minIndices.at(series)))
            {
                minIndices[series] = category;
            }
            if (value > tops.at(maxIndices.at(series)))
            {
                maxIndices[series] = category;
            }
        }
    }
    flushColumn();
    return indices;
}

void ShadCategoryChartPrivate::paintAxes(QPainter* painter) const
{
    const ShadChart* q = q_ptr;
    const Qt::LayoutDirection direction = q->layoutDirection();
    const QRectF bounds(q->rect());
    const QFont font = textFont();
    painter->setFont(font);
    painter->setPen(shadTheme->getColor(ShadThemeType::MutedForeground));

    // Y 轴刻度文字靠绘图区右对齐，与绘图区间隔 tickMargin
    if (_isYAxisVisible && !_plotRect.isEmpty())
    {
        const Qt::Alignment alignment = ShadStyleHelper::visualAlignment(direction, Qt::AlignRight | Qt::AlignVCenter);
        for (const qreal tick : std::as_const(_yTicks))
        {
            const QRectF logicalRect(0.0, valueY(tick) - ShadChartHelper::kTickTextHeight / 2.0, _plotRect.left() - ShadChartHelper::kTickMargin, ShadChartHelper::kTickTextHeight);
            painter->drawText(ShadStyleHelper::visualRect(direction, bounds, logicalRect), alignment | Qt::TextDontClip, formatTick(tick));
        }
    }

    // X 轴刻度文字居中于分类位置，最后一个刻度超出绘图区时已向内移动
    if (_isXAxisVisible)
    {
        const qreal textTop = _plotRect.top() + _plotRect.height() + ShadChartHelper::kTickMargin;
        for (qsizetype index = 0; index < _xTickIndices.size() && index < _xTickCenters.size(); ++index)
        {
            const QString text = tickLabelAt(_xTickIndices.at(index));
            const qreal textWidth = ShadStyleHelper::textWidth(font, text);
            const qreal x = physicalX(_xTickCenters.at(index));
            painter->drawText(QRectF(x - textWidth / 2.0, textTop, textWidth, ShadChartHelper::kTickTextHeight), Qt::AlignCenter | Qt::TextDontClip, text);
        }
    }
}

void ShadCategoryChartPrivate::paintHover(QPainter* painter) const
{
    // 柱状图不显示光标线与数据点（对应 cursor={false}）
    if (_mark == Bar || _hoverIndex < 0 || _hoverIndex >= _count || _plotRect.isEmpty())
    {
        return;
    }
    const qreal x = physicalX(categoryX(_hoverIndex));
    // 光标线：stroke-border，1px，对齐到像素中心
    painter->save();
    painter->setPen(QPen(shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Border), 1.0));
    const qreal lineX = std::floor(x) + 0.5;
    painter->drawLine(QPointF(lineX, _plotRect.top()), QPointF(lineX, _plotRect.top() + _plotRect.height()));
    // 当前分类上的数据点：半径 4，填充为系列颜色
    // 修改数据后到合并重建之前，系列数量与上次布局不同时只画两边都有的系列
    painter->setPen(Qt::NoPen);
    const int seriesCount = static_cast<int>(qMin(_series.size(), _tops.size()));
    for (int series = 0; series < seriesCount; ++series)
    {
        painter->setBrush(seriesColor(series));
        painter->drawEllipse(QPointF(x, valueY(_tops.at(series).at(_hoverIndex))), ShadChartHelper::kActiveDotRadius, ShadChartHelper::kActiveDotRadius);
    }
    painter->restore();
}

void ShadCategoryChartPrivate::paintTooltip(QPainter* painter) const
{
    if (_hoverIndex < 0 || _hoverIndex >= _count)
    {
        return;
    }
    // 与 paintHover 相同，只显示上次布局中也存在的系列
    const int seriesCount = static_cast<int>(qMin(_series.size(), _tops.size()));
    QList<ShadChartHelper::TooltipRow> rows;
    for (int series = 0; series < seriesCount; ++series)
    {
        rows.append({_series.at(series).label, ShadChartHelper::formatValue(valueAt(series, _hoverIndex)), seriesColor(series), _series.at(series).icon});
    }
    ShadChartHelper::paintTooltip(painter, q_ptr, physicalX(categoryX(_hoverIndex)), _hoverPosition.y(), categoryLabel(_hoverIndex), rows, _tooltipIndicator);
}

qreal ShadCategoryChartPrivate::valueAt(int series, int category) const
{
    return _series.at(series).values.value(category, 0.0);
}

QColor ShadCategoryChartPrivate::seriesColor(int series) const
{
    const Series& item = _series.at(series);
    return item.customColor.isValid() ? item.customColor : shadTheme->getColor(item.colorRole);
}

QString ShadCategoryChartPrivate::formatTick(qreal value) const
{
    if (_stackMode == ShadChartType::Expanded)
    {
        return QStringLiteral("%1%").arg(qRound(value * 100.0));
    }
    return ShadChartHelper::formatValue(value);
}

QString ShadCategoryChartPrivate::categoryLabel(int category) const
{
    if (category >= 0 && category < _categories.size())
    {
        return _categories.at(category);
    }
    return QString::number(category + 1);
}

QString ShadCategoryChartPrivate::tickLabelAt(int category) const
{
    // 刻度序号来自上次布局，数据修改后越界时返回空文字
    if (!_tickLabels.isEmpty() && _tickLabels.size() == _categories.size())
    {
        return _tickLabels.value(category);
    }
    return categoryLabel(category);
}

QFont ShadCategoryChartPrivate::textFont() const
{
    return q_ptr->font();
}

qreal ShadCategoryChartPrivate::physicalX(qreal logicalX) const
{
    return q_ptr->layoutDirection() == Qt::RightToLeft ? q_ptr->width() - logicalX : logicalX;
}

bool ShadCategoryChartPrivate::isBandScale() const
{
    return _mark == Bar;
}

qreal ShadCategoryChartPrivate::visibleUnits() const
{
    return isBandScale() ? _viewSpan + 1.0 : _viewSpan;
}

qreal ShadCategoryChartPrivate::categoryX(int category) const
{
    if (isBandScale())
    {
        return _plotRect.left() + static_cast<qreal>(_plotRect.width()) * (category - _viewStart + 0.5) / visibleUnits();
    }
    if (_count <= 1 || _viewSpan <= 0.0)
    {
        return _plotRect.left() + _plotRect.width() / 2.0;
    }
    return _plotRect.left() + static_cast<qreal>(_plotRect.width()) * (category - _viewStart) / _viewSpan;
}

void ShadCategoryChartPrivate::updateVisibleRange()
{
    const qreal fullSpan = qMax(0.0, static_cast<qreal>(_count - 1));
    if (_isZoomEnabled && _isZoomed && _count >= 2)
    {
        // 分类数量变化后重新限制跨度，放大范围不小于全部分类时视为未放大
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

qreal ShadCategoryChartPrivate::minVisibleSpan() const
{
    return qMin(kMinVisibleSpan, qMax(1.0, static_cast<qreal>(_count - 1)));
}

qreal ShadCategoryChartPrivate::valueY(qreal value) const
{
    const qreal range = _domainMax - _domainMin;
    if (range <= 0.0)
    {
        return _plotRect.top() + _plotRect.height();
    }
    return _plotRect.top() + (_domainMax - value) / range * _plotRect.height();
}

QRect ShadCategoryChartPrivate::physicalPlotRect() const
{
    return ShadStyleHelper::visualRect(q_ptr->layoutDirection(), q_ptr->rect(), _plotRect);
}
