#ifndef SHADCHART_H
#define SHADCHART_H

#include <QList>
#include <QSize>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QPaintEvent;
class QResizeEvent;

// 图表基类，不能直接创建：图形默认由 GPU（QRhi）绘制，不可用时改用 CPU；文字与提示用 QPainter 绘制
class ShadChartPrivate;

class SHADCN_EXPORT ShadChart : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadChart)
    Q_PROPERTY(ShadThemeType::ThemeColor surfaceColor READ getSurfaceColor WRITE setSurfaceColor)

    public:
        ~ShadChart() override;

        // 之后新建的图表使用的绘制 API，默认 Auto，不支持的值按 Auto 处理；环境变量 SHADCN_CHART_API 优先
        // 同一窗口内的 GPU 图表只能用同一种 API
        static void setDefaultRenderApi(ShadChartType::RenderApi api);
        static ShadChartType::RenderApi getDefaultRenderApi();
        // 当前平台可选的 API，含 Auto 与 Software
        static QList<ShadChartType::RenderApi> availableRenderApis();
        // 切换本图表的绘图层：Software 为 CPU，Auto 或窗口使用的 GPU API 为 GPU，其他值输出警告并忽略
        void setRenderApi(ShadChartType::RenderApi api);
        // 实际使用的 API，回退后为 Software
        ShadChartType::RenderApi getRenderApi() const;

        // 绘图层铺底的颜色，应与图表所在表面相同，默认 card
        void setSurfaceColor(ShadThemeType::ThemeColor colorRole);
        ShadThemeType::ThemeColor getSurfaceColor() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 绘图层完成一帧后发出；帧的呈现受屏幕垂直同步限制，可据此按屏幕刷新率推送实时数据
        // 图表不可见（例如所在页面被切走）时不绘制，也不发出
        void frameRendered();

    protected:
        ShadChart(ShadChartPrivate& dd, QWidget* parent);

        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        // 窗口特效开启时铺不透明底色
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADCHART_H
