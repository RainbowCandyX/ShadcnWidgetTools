#include "ShadChart.h"

#include <QEvent>
#include <QFont>
#include <QList>
#include <QMetaEnum>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadChartHelper.h"
#include "ShadChartOverlay.h"
#include "ShadChartPrivate.h"
#include "ShadChartSoftwareView.h"
#if defined(SHADCN_CHART_GPU)
#include "ShadChartView.h"
#endif
#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadChart::ShadChart(ShadChartPrivate& dd, QWidget* parent)
    : QWidget(parent), d_ptr(&dd)
{
    Q_D(ShadChart);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    // 绘图层在下，覆盖层在上
    d->createPlotView(ShadChartPrivate::gpuRenderApi());
    d->_overlay = new ShadChartOverlay(d, this);
    d->_overlay->raise();
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->requestUpdate(); });
    // 字体变化会触发 changeEvent 重新布局，必须在子组件创建之后设置；图表容器为 text-xs
    ShadStyleHelper::applyFont(this, 12, QFont::Normal);
    d->updateChart();
}

ShadChart::~ShadChart()
{
    Q_D(ShadChart);
    // 覆盖层与 CPU 绘制层持有私有数据的指针，先于私有数据删除
    delete d->_overlay;
    d->_overlay = nullptr;
    delete d->_softwareView;
    d->_softwareView = nullptr;
}

void ShadChart::setDefaultRenderApi(ShadChartType::RenderApi api)
{
    if (!availableRenderApis().contains(api))
    {
        qWarning("不支持绘制 API %s，按 Auto 处理", QMetaEnum::fromType<ShadChartType::RenderApi>().valueToKey(api));
        api = ShadChartType::Auto;
    }
    ShadChartPrivate::_defaultRenderApi = api;
}

ShadChartType::RenderApi ShadChart::getDefaultRenderApi()
{
    return ShadChartPrivate::_defaultRenderApi;
}

QList<ShadChartType::RenderApi> ShadChart::availableRenderApis()
{
    QList<ShadChartType::RenderApi> apis{ShadChartType::Auto, ShadChartType::Software};
#if defined(SHADCN_CHART_GPU)
    apis.append(ShadChartView::availableApis());
#endif
    return apis;
}

void ShadChart::setRenderApi(ShadChartType::RenderApi api)
{
    Q_D(ShadChart);
    const ShadChartType::RenderApi gpuApi = ShadChartPrivate::gpuRenderApi();
    const ShadChartType::RenderApi target = api == ShadChartType::Auto ? gpuApi : api;
    // 同一窗口只能用一种 GPU API
    if (target != ShadChartType::Software && target != gpuApi)
    {
        const QMetaEnum apiEnum = QMetaEnum::fromType<ShadChartType::RenderApi>();
        qWarning("图表不能切换到 %s，可选 %s 或 Software", apiEnum.valueToKey(api), apiEnum.valueToKey(gpuApi));
        return;
    }
    d->replacePlotView(target);
}

ShadChartType::RenderApi ShadChart::getRenderApi() const
{
    Q_D(const ShadChart);
    return d->_renderApi;
}

void ShadChart::setSurfaceColor(ShadThemeType::ThemeColor colorRole)
{
    Q_D(ShadChart);
    if (d->_surfaceColor == colorRole)
    {
        return;
    }
    d->_surfaceColor = colorRole;
    d->requestUpdate();
}

ShadThemeType::ThemeColor ShadChart::getSurfaceColor() const
{
    Q_D(const ShadChart);
    return d->_surfaceColor;
}

QSize ShadChart::sizeHint() const
{
    return QSize(ShadChartHelper::kDefaultWidth, ShadChartHelper::kDefaultHeight);
}

QSize ShadChart::minimumSizeHint() const
{
    return QSize(ShadChartHelper::kMinimumSize, ShadChartHelper::kMinimumSize);
}

void ShadChart::changeEvent(QEvent* event)
{
    Q_D(ShadChart);
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange)
    {
        d->updateChart();
    }
}

void ShadChart::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadChart);
    QWidget::resizeEvent(event);
    // 尺寸变化立即重建，与本次布局同步
    d->updateChart();
}

void ShadChart::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadChart);
    if (shadTheme->getColor(d->_surfaceColor).alpha() == 255)
    {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    ShadStyleHelper::fillRoundedRect(&painter, QRectF(rect()), shadTheme->getRadius(ShadThemeType::Medium), d->surfaceColor());
}
