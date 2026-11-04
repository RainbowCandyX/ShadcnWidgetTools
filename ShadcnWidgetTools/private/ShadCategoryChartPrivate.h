#ifndef SHADCATEGORYCHARTPRIVATE_H
#define SHADCATEGORYCHARTPRIVATE_H

#include <QColor>
#include <QFont>
#include <QList>
#include <QPointF>
#include <QPolygonF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "ShadChartHelper.h"
#include "ShadChartPrivate.h"
#include "ShadDef.h"
#include "ShadIconType.h"

class QPainter;

// 面积图、折线图与柱状图共用的私有数据，按 _mark 生成图形
class ShadCategoryChartPrivate : public ShadChartPrivate
{
    public:
        enum Mark
        {
            Area = 0,
            Line,
            Bar,
        };

        explicit ShadCategoryChartPrivate(Mark mark);
        ~ShadCategoryChartPrivate() override;

        struct Series
        {
            QString label;
            QList<qreal> values;
            ShadThemeType::ThemeColor colorRole{ShadThemeType::Chart1};
            QColor customColor;
            ShadIconType::IconName icon{ShadIconType::None};
        };

        // 绘图层的内容，GPU 与 CPU 共用；坐标相对绘图区左上角
        struct PlotArea
        {
            // 上下边界的采样点一一对应
            QList<QPointF> top;
            QList<QPointF> bottom;
            QColor color;
            // 包围盒，用作渐变范围
            qreal boxTop{0.0};
            qreal boxHeight{0.0};
        };

        struct PlotLine
        {
            QList<QPointF> points;
            QColor color;
        };

        struct PlotBar
        {
            // 圆角已展开为凸多边形
            QPolygonF polygon;
            QColor color;
        };

        struct PlotDot
        {
            QPointF center;
            QColor color;
        };

        struct PlotGeometry
        {
            QList<QRectF> gridLines;
            QColor gridColor;
            QList<PlotArea> areas;
            QList<PlotLine> lines;
            QList<PlotBar> bars;
            QList<PlotDot> dots;
            qreal dotRadius{0.0};
        };

        // 面积描边宽度 1
        static constexpr qreal kStrokeWidth = 1.0;
        // 填充不透明度 fillOpacity={0.4}；渐变在面积包围盒 5% 处为 0.8、95% 处为 0.1
        static constexpr qreal kFillOpacity = 0.4;
        static constexpr qreal kGradientTopOpacity = 0.8;
        static constexpr qreal kGradientBottomOpacity = 0.1;
        // 折线默认宽度 2；数据点半径为 3 加线宽的一半
        static constexpr qreal kDefaultLineWidth = 2.0;
        static constexpr qreal kDotRadius = 3.0;
        // 柱子：分类两侧各留 10%（barCategoryGap），柱间距 4（barGap），默认圆角 4
        static constexpr qreal kBarCategoryGap = 0.1;
        static constexpr qreal kBarGap = 4.0;
        static constexpr qreal kDefaultBarRadius = 4.0;
        // 平均每个像素超过 2 个分类时开始抽稀
        static constexpr int kDecimationPointsPerPixel = 2;
        // 缩放：最多放大到可见 10 个分类间隔；滚轮每转一格（120）可见范围缩小为 80%
        static constexpr qreal kMinVisibleSpan = 10.0;
        static constexpr qreal kZoomStepFactor = 0.8;
        // 可见范围两侧多取的分类，曲线在绘图区边缘保持连续（超出部分由绘图层裁掉）
        static constexpr int kEdgePadding = 2;

        void updateChart() override;
        void paintPlot(QPainter* painter) const override;
        void paintOverlay(QPainter* painter) override;
        void updateHover(const QPointF& position) override;
        void clearHover() override;
        // 以鼠标所在分类为中心缩放
        bool zoomAt(const QPointF& position, int angleDelta) override;
        bool zoomByFactor(const QPointF& position, qreal factor) override;
        bool beginPan(const QPointF& position) override;
        void panTo(const QPointF& position) override;
        void endPan() override;
        bool handleDoubleClick(const QPointF& position) override;
        // 恢复显示全部分类
        void resetZoom();
        // 开头移除 count 个分类后调用：放大且未贴住末尾时可见范围前移，保持显示同一批数据
        void shiftView(qreal count);
        int categoryCount() const;

        const Mark _mark;
        QStringList _categories;
        QStringList _tickLabels;
        QList<Series> _series;
        ShadChartType::CurveType _curveType{ShadChartType::Natural};
        ShadChartType::StackMode _stackMode{ShadChartType::NoStack};
        ShadChartType::FillStyle _fillStyle{ShadChartType::Solid};
        ShadChartType::TooltipIndicator _tooltipIndicator{ShadChartType::Dot};
        qreal _lineWidth{kDefaultLineWidth};
        bool _isDotsVisible{false};
        qreal _barRadius{kDefaultBarRadius};
        bool _isGridVisible{true};
        bool _isXAxisVisible{true};
        bool _isYAxisVisible{false};
        bool _isLegendVisible{false};
        bool _isTooltipVisible{true};
        int _yTickCount{5};
        bool _isZoomEnabled{false};
        // 是否处于放大状态；放大且可见范围贴住最后一个分类时，追加数据后继续显示最新的数据
        bool _isZoomed{false};
        bool _isAnchoredToEnd{false};
        bool _isPanning{false};

    private:
        // 计算分类数量、可见范围、堆叠后的上下边界、Y 轴范围与刻度、绘图区与 X 轴刻度
        void computeLayout();
        // 按分类数量限制可见范围，并计算参与绘制的分类区间
        void updateVisibleRange();
        // 鼠标位置对应的分类，不在绘图区内或未开启提示时为 -1
        int categoryAt(const QPointF& position) const;
        // 缩放时可见范围的最小跨度
        qreal minVisibleSpan() const;
        // 柱状图为 band 刻度（每个分类占一个单位），面积图与折线图为 point 刻度
        bool isBandScale() const;
        // 绘图区宽度对应的单位数
        qreal visibleUnits() const;
        PlotGeometry buildGeometry() const;
        void buildCurves(PlotGeometry& geometry, qreal originX, qreal originY) const;
        void buildBars(PlotGeometry& geometry, qreal originX, qreal originY) const;
        // 可见分类多于绘图区宽度（像素）时按像素列合并，每列每个系列只画一根柱子
        void buildDecimatedBars(PlotGeometry& geometry, qreal originX, qreal originY) const;
        // 面积在 y 处的填充不透明度，已乘以 kFillOpacity
        qreal fillOpacity(const PlotArea& area, qreal y) const;
#if defined(SHADCN_CHART_GPU)
        QList<ShadChartVertex> buildVertices(const PlotGeometry& geometry) const;
#endif
        // 按像素列抽稀：分类数超过绘图区宽度的 kDecimationPointsPerPixel 倍时，每列保留第一个、最后一个分类与各系列上边界最大、最小的分类
        // 所有系列使用同一组分类，堆叠的上下边界仍然吻合；无需抽稀时返回空列表
        QList<int> decimatedIndices() const;
        void paintAxes(QPainter* painter) const;
        void paintHover(QPainter* painter) const;
        void paintTooltip(QPainter* painter) const;

        qreal valueAt(int series, int category) const;
        QColor seriesColor(int series) const;
        QString formatTick(qreal value) const;
        // 提示卡片标题：分类文字，未设置分类时为从 1 开始的序号
        QString categoryLabel(int category) const;
        QString tickLabelAt(int category) const;
        QFont textFont() const;
        // 逻辑 X（LTR，组件坐标）转换为实际 X，RTL 时水平镜像
        qreal physicalX(qreal logicalX) const;
        // 分类在组件内的逻辑 X 与数值在组件内的 Y
        qreal categoryX(int category) const;
        qreal valueY(qreal value) const;
        // 绘图区的实际位置（RTL 已镜像），即绘图层的位置
        QRect physicalPlotRect() const;

        // 以下为 computeLayout 的结果
        int _count{0};
        // 每个系列在每个分类上的上边界与下边界（数值空间，百分比堆叠时为 0 到 1）
        QList<QList<qreal>> _tops;
        QList<QList<qreal>> _bottoms;
        qreal _domainMin{0.0};
        qreal _domainMax{1.0};
        QList<qreal> _yTicks;
        // 绘图区的逻辑位置（LTR）
        QRect _plotRect;
        QList<int> _xTickIndices;
        // 每个刻度文字中心的逻辑 X，通常等于分类位置，最后一个刻度超出绘图区时向内移动
        QList<qreal> _xTickCenters;
        // 悬停的分类与鼠标位置，未悬停时为 -1
        int _hoverIndex{-1};
        QPointF _hoverPosition;
        // 可见范围：从分类 _viewStart（可为小数）开始、跨 _viewSpan 个分类间隔；未放大时为全部分类
        qreal _viewStart{0.0};
        qreal _viewSpan{0.0};
        // 参与绘制的分类区间（含两侧的 kEdgePadding）
        int _firstVisible{0};
        int _lastVisible{-1};
        // 平移开始时的鼠标 X 与可见范围起点
        qreal _panStartX{0.0};
        qreal _panStartView{0.0};
        // CPU 层绘制的内容
        PlotGeometry _plotGeometry;
};

#endif // SHADCATEGORYCHARTPRIVATE_H
