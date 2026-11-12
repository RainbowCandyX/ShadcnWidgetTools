#ifndef SHADCANDLESTICKCHARTPRIVATE_H
#define SHADCANDLESTICKCHARTPRIVATE_H

#include <QColor>
#include <QFont>
#include <QList>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QtGlobal>

#include "ShadCandlestickChart.h"
#include "ShadChartHelper.h"
#include "ShadChartPrivate.h"
#include "ShadDef.h"

class QPainter;

// 价格区与成交量区共用一个绘图层，X 方向每根 K 线占一个单位
class ShadCandlestickChartPrivate : public ShadChartPrivate
{
    public:
        ShadCandlestickChartPrivate();
        ~ShadCandlestickChartPrivate() override;

        // 坐标相对绘图层左上角
        struct PlotRect
        {
            QRectF rect;
            QColor color;
        };

        struct PlotLine
        {
            QList<QPointF> points;
            QColor color;
        };

        // GPU 与 CPU 共用；矩形依次为成交量柱、影线、实体
        struct PlotGeometry
        {
            QList<QRectF> gridLines;
            QColor gridColor;
            QList<PlotRect> rects;
            QList<PlotLine> lines;
        };

        // 实体占间隔的 70%
        static constexpr qreal kCandleGap = 0.15;
        static constexpr qreal kVolumeRatio = 0.25;
        static constexpr int kPaneGap = 8;
        static constexpr int kVolumeTickCount = 3;
        static constexpr qreal kVolumeOpacity = 0.5;
        static constexpr qreal kMovingAverageWidth = 1.5;
        static constexpr qreal kMinVisibleSpan = 10.0;
        // 滚轮每格（120）的缩放比例
        static constexpr qreal kZoomStepFactor = 0.8;
        // 可见范围两侧多取的 K 线，均线在边缘保持连续
        static constexpr int kEdgePadding = 2;

        void updateChart() override;
        void paintPlot(QPainter* painter) const override;
        void paintOverlay(QPainter* painter) override;
        void updateHover(const QPointF& position) override;
        void clearHover() override;
        bool zoomAt(const QPointF& position, int angleDelta) override;
        bool zoomByFactor(const QPointF& position, qreal factor) override;
        bool beginPan(const QPointF& position) override;
        void panTo(const QPointF& position) override;
        void endPan() override;
        bool handleDoubleClick(const QPointF& position) override;
        void resetZoom();
        // 开头移除 count 根后调用，保持显示同一批数据
        void shiftView(qreal count);

        QList<ShadCandle> _candles;
        ShadChartType::CandleColorScheme _colorScheme{ShadChartType::RedRising};
        QList<int> _movingAverages;
        bool _isVolumeVisible{false};
        int _pricePrecision{2};
        bool _isGridVisible{true};
        bool _isXAxisVisible{true};
        bool _isYAxisVisible{true};
        bool _isLegendVisible{false};
        bool _isTooltipVisible{true};
        int _yTickCount{5};
        bool _isZoomEnabled{false};
        bool _isZoomed{false};
        // 贴住最后一根时，追加数据后继续显示最新的数据
        bool _isAnchoredToEnd{false};
        bool _isPanning{false};

    private:
        // 像素列合并后的 K 线，first 为 -1 表示空列
        struct Column
        {
            int first{-1};
            int last{-1};
            qreal open{0.0};
            qreal high{0.0};
            qreal low{0.0};
            qreal close{0.0};
            qreal volume{0.0};
        };

        void computeLayout();
        void computeMovingAverages();
        void updateVisibleRange();
        // 不在绘图区内或未开启提示时为 -1
        int candleAt(const QPointF& position) const;
        qreal minVisibleSpan() const;
        qreal visibleUnits() const;
        PlotGeometry buildGeometry() const;
        void buildCandles(PlotGeometry& geometry, qreal originX, qreal originY) const;
        // K 线多于像素宽度时按像素列合并
        void buildDecimatedCandles(PlotGeometry& geometry, qreal originX, qreal originY) const;
        void buildMovingAverages(PlotGeometry& geometry, qreal originX, qreal originY, const QList<int>& indices) const;
#if defined(SHADCN_CHART_GPU)
        QList<ShadChartVertex> buildVertices(const PlotGeometry& geometry) const;
#endif
        void paintAxes(QPainter* painter) const;
        void paintCrosshair(QPainter* painter) const;
        void paintTooltip(QPainter* painter) const;

        bool isRising(qreal open, qreal close) const;
        QColor candleColor(qreal open, qreal close) const;
        QColor movingAverageColor(int index) const;
        QString formatPrice(qreal value) const;
        // 时间文字为空时为从 1 开始的序号
        QString candleLabel(int index) const;
        QFont textFont() const;
        // RTL 时水平镜像
        qreal physicalX(qreal logicalX) const;
        qreal candleX(int index) const;
        qreal priceY(qreal price) const;
        qreal volumeY(qreal volume) const;
        // RTL 已镜像
        QRect physicalPlotRect() const;

        // 以下为 computeLayout 的结果
        int _count{0};
        // 周期内数据不足时为 NaN
        QList<QList<qreal>> _averageValues;
        qreal _priceMin{0.0};
        qreal _priceMax{1.0};
        QList<qreal> _priceTicks;
        qreal _volumeMax{1.0};
        QList<qreal> _volumeTicks;
        // 逻辑位置（LTR）；_plotRect 包含价格区、间隔与成交量区
        QRect _plotRect;
        QRect _priceRect;
        QRect _volumeRect;
        QList<int> _xTickIndices;
        QList<qreal> _xTickCenters;
        // 未悬停时为 -1
        int _hoverIndex{-1};
        QPointF _hoverPosition;
        // 从 _viewStart 开始跨 _viewSpan 个间隔，可为小数
        qreal _viewStart{0.0};
        qreal _viewSpan{0.0};
        // 含两侧的 kEdgePadding
        int _firstVisible{0};
        int _lastVisible{-1};
        qreal _panStartX{0.0};
        qreal _panStartView{0.0};
        // CPU 层绘制的内容
        PlotGeometry _plotGeometry;
};

#endif // SHADCANDLESTICKCHARTPRIVATE_H
