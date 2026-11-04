#ifndef SHADSCATTERCHARTPRIVATE_H
#define SHADSCATTERCHARTPRIVATE_H

#include <QColor>
#include <QFont>
#include <QList>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QtGlobal>

#include "ShadChartPrivate.h"
#include "ShadDef.h"

class QPainter;

// 散点图私有数据；GPU 层中每个系列一个环形缓冲，坐标为相对 _origin 的数据坐标
class ShadScatterChartPrivate : public ShadChartPrivate
{
    public:
        ShadScatterChartPrivate();
        ~ShadScatterChartPrivate() override;

        struct Series
        {
            QString label;
            QList<QPointF> points;
            ShadThemeType::ThemeColor colorRole{ShadThemeType::Chart1};
            QColor customColor;
            // 以下为 GPU 缓冲的同步状态
            int pendingAppend{0};
            int ringMaxCount{0};
            bool isFullDirty{true};
            int gpuHead{0};
            int gpuCount{0};
            int gpuCapacity{0};
            QColor gpuColor;
        };

        static constexpr qreal kDefaultPointRadius = 3.0;
        // 悬停时取 8px 内最近的点
        static constexpr qreal kHoverDistance = 8.0;
        // 数据离原点超过范围跨度的 1000 倍时重选原点，保持 float 精度
        static constexpr qreal kRebaseRatio = 1000.0;

        void updateChart() override;
        void paintPlot(QPainter* painter) const override;
        void paintOverlay(QPainter* painter) override;
        void updateHover(const QPointF& position) override;
        void clearHover() override;
        QColor seriesColor(int series) const;

        QList<Series> _series;
        qreal _pointRadius{kDefaultPointRadius};
        bool _isGridVisible{true};
        bool _isXAxisVisible{true};
        bool _isYAxisVisible{true};
        bool _isLegendVisible{false};
        bool _isTooltipVisible{true};
        int _xTickCount{5};
        int _yTickCount{5};

    protected:
        void plotViewCreated() override;

    private:
        // 计算数据范围、刻度与绘图区，必要时重新选取原点
        void computeLayout();
        // 坐标相对绘图层
        QList<QRectF> gridLines() const;
#if defined(SHADCN_CHART_GPU)
        void syncPoints();
#endif
        // 绘图层内坐标 = (数据坐标 - 原点) * scale + offset，RTL 已镜像
        void plotTransform(qreal& scaleX, qreal& offsetX, qreal& scaleY, qreal& offsetY) const;
        // 返回是否找到
        bool findPoint(const QPointF& position, int& series, int& index) const;
        void paintAxes(QPainter* painter) const;
        QFont textFont() const;
        qreal physicalX(qreal logicalX) const;
        // 数值在组件内的逻辑 X 与 Y
        qreal valueX(qreal value) const;
        qreal valueY(qreal value) const;
        QRect physicalPlotRect() const;

        QPointF _origin;
        bool _hasOrigin{false};
        // 以下为 computeLayout 的结果
        QList<qreal> _xTicks;
        QList<qreal> _yTicks;
        // 显示的 X 轴刻度序号
        QList<int> _xTickIndices;
        qreal _xMin{0.0};
        qreal _xMax{1.0};
        qreal _yMin{0.0};
        qreal _yMax{1.0};
        // 绘图区的逻辑位置（LTR）
        QRect _plotRect;
        // 悬停的点与鼠标位置，未悬停时为 -1
        int _hoverSeries{-1};
        int _hoverIndex{-1};
        QPointF _hoverPosition;
};

#endif // SHADSCATTERCHARTPRIVATE_H
