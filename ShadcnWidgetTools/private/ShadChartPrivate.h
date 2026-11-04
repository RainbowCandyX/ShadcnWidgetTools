#ifndef SHADCHARTPRIVATE_H
#define SHADCHARTPRIVATE_H

#include <QColor>
#include <QPointF>
#include <QRect>

#include "ShadDef.h"
#include "ShadProperty.h"

class QPainter;
class ShadChart;
class ShadChartOverlay;
class ShadChartSoftwareView;
class ShadChartView;

// 图表私有数据基类：管理绘图层与重建请求，布局、绘制与交互由子类实现
class ShadChartPrivate
{
    SHAD_D_CREATE(ShadChart)

    public:
        ShadChartPrivate();
        virtual ~ShadChartPrivate();

        // 创建绘图层：api 为 Software 时为 CPU 层，否则为 QRhi 层
        void createPlotView(ShadChartType::RenderApi api);
        // 删除当前绘图层并按 api 重建
        void replacePlotView(ShadChartType::RenderApi api);
        // 窗口使用的 GPU API：环境变量或默认值，Auto 换成平台默认；不可用时为 Software，每种 API 只检测一次
        static ShadChartType::RenderApi gpuRenderApi();
        // 数据与样式变化时调用：合并同一轮事件循环内的多次修改，只在事件循环空闲时重建一次
        void requestUpdate();
        // GPU 层无法显示半透明，不随窗口特效变透明
        QColor surfaceColor() const;

        // 重新布局并更新绘图层与覆盖层；实现时先把 _isUpdatePending 置为 false
        virtual void updateChart() = 0;
        // CPU 层的绘制，坐标相对绘图层
        virtual void paintPlot(QPainter* painter) const = 0;
        virtual void paintOverlay(QPainter* painter) = 0;
        virtual void updateHover(const QPointF& position);
        virtual void clearHover();
        // 返回 false 时事件交给父组件
        virtual bool zoomAt(const QPointF& position, int angleDelta);
        virtual bool zoomByFactor(const QPointF& position, qreal factor);
        // 返回是否开始平移
        virtual bool beginPan(const QPointF& position);
        virtual void panTo(const QPointF& position);
        virtual void endPan();
        // 返回是否已处理
        virtual bool handleDoubleClick(const QPointF& position);

        // setDefaultRenderApi 的值
        static inline ShadChartType::RenderApi _defaultRenderApi{ShadChartType::Auto};

        ShadChartView* _view{nullptr};
        ShadChartSoftwareView* _softwareView{nullptr};
        ShadChartOverlay* _overlay{nullptr};
        // 实际使用的绘制 API
        ShadChartType::RenderApi _renderApi{ShadChartType::Software};
        ShadThemeType::ThemeColor _surfaceColor{ShadThemeType::Card};
        // 已请求、尚未执行的重建
        bool _isUpdatePending{false};
        // GPU 层画出第一帧之前内容未定义，覆盖层先用表面颜色盖住绘图区
        bool _isPlotReady{false};

    protected:
        // 新建绘图层后调用，GPU 缓冲需要整份重新上传
        virtual void plotViewCreated();
        void placePlotView(const QRect& rect, bool isVisible);
        // 未悬停且绘图区不动时只重绘绘图区以外的部分
        void updateOverlay(const QRect& plotRect, bool isHoverChanged);

    private:
#if defined(SHADCN_CHART_GPU)
        // GPU 绘制失败后切到 CPU
        void fallBackToSoftware();
#endif

        // 上次重绘覆盖层时绘图区的实际位置
        QRect _overlayPlotRect;
};

#endif // SHADCHARTPRIVATE_H
