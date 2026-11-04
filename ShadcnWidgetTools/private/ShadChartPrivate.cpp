#include "ShadChartPrivate.h"

#include <QList>
#include <QMap>
#include <QMetaEnum>
#include <QMetaObject>
#include <QObject>
#include <QRegion>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#if defined(SHADCN_CHART_GPU)
#include <QRhiWidget>
#endif

#include "ShadChart.h"
#include "ShadChartOverlay.h"
#include "ShadChartSoftwareView.h"
#if defined(SHADCN_CHART_GPU)
#include "ShadChartView.h"
#endif
#include "ShadTheme.h"

ShadChartPrivate::ShadChartPrivate()
{
}

ShadChartPrivate::~ShadChartPrivate()
{
}

void ShadChartPrivate::createPlotView(ShadChartType::RenderApi api)
{
    Q_Q(ShadChart);
    _renderApi = api;
#if defined(SHADCN_CHART_GPU)
    if (api != ShadChartType::Software)
    {
        _view = new ShadChartView(api, q);
        // 切换绘图层时覆盖层需要整块重绘，盖住新的 GPU 层
        _isPlotReady = false;
        if (_overlay != nullptr)
        {
            _overlay->update();
        }
        ShadChartView* view = _view;
        QObject::connect(_view, &QRhiWidget::frameSubmitted, q, &ShadChart::frameRendered);
        QObject::connect(_view, &QRhiWidget::frameSubmitted, q, [this, view]()
        {
            if (_view != view || _isPlotReady)
            {
                return;
            }
            _isPlotReady = true;
            if (_overlay != nullptr)
            {
                _overlay->update();
            }
        });
        // 排队执行，避免在绘制流程中替换绘图层；只处理发出信号的那一层
        QObject::connect(_view, &QRhiWidget::renderFailed, q, [this, view]()
        {
            if (_view == view)
            {
                fallBackToSoftware();
            }
        }, Qt::QueuedConnection);
        plotViewCreated();
        return;
    }
#endif
    _softwareView = new ShadChartSoftwareView(this, q);
    _isPlotReady = true;
    QObject::connect(_softwareView, &ShadChartSoftwareView::framePainted, q, &ShadChart::frameRendered);
    plotViewCreated();
}

void ShadChartPrivate::replacePlotView(ShadChartType::RenderApi api)
{
    if (api == _renderApi)
    {
        return;
    }
#if defined(SHADCN_CHART_GPU)
    if (_view != nullptr)
    {
        Q_Q(ShadChart);
        QObject::disconnect(_view, nullptr, q, nullptr);
        _view->hide();
        _view->deleteLater();
        _view = nullptr;
    }
#endif
    delete _softwareView;
    _softwareView = nullptr;
    createPlotView(api);
    updateChart();
}

ShadChartType::RenderApi ShadChartPrivate::gpuRenderApi()
{
#if !defined(SHADCN_CHART_GPU)
    return ShadChartType::Software;
#else
    const QMetaEnum apiEnum = QMetaEnum::fromType<ShadChartType::RenderApi>();
    // 环境变量只读一次，未设置或无效时为 Auto
    static const ShadChartType::RenderApi environmentApi = [apiEnum]() -> ShadChartType::RenderApi
    {
        const QString value = qEnvironmentVariable("SHADCN_CHART_API").trimmed().toLower();
        if (value.isEmpty())
        {
            return ShadChartType::Auto;
        }
        const QMap<QString, ShadChartType::RenderApi> names{{QStringLiteral("software"), ShadChartType::Software}, {QStringLiteral("opengl"), ShadChartType::OpenGL}, {QStringLiteral("d3d11"), ShadChartType::Direct3D11}, {QStringLiteral("d3d12"), ShadChartType::Direct3D12}, {QStringLiteral("vulkan"), ShadChartType::Vulkan}, {QStringLiteral("metal"), ShadChartType::Metal}};
        const auto it = names.constFind(value);
        if (it == names.constEnd())
        {
            qWarning("SHADCN_CHART_API=%s 无效，已忽略", qPrintable(value));
            return ShadChartType::Auto;
        }
        if (it.value() != ShadChartType::Software && !ShadChartView::availableApis().contains(it.value()))
        {
            qWarning("当前平台不支持 SHADCN_CHART_API=%s，已忽略", qPrintable(value));
            return ShadChartType::Auto;
        }
        qInfo("图表使用 %s（SHADCN_CHART_API）", apiEnum.valueToKey(it.value()));
        return it.value();
    }();
    ShadChartType::RenderApi api = environmentApi != ShadChartType::Auto ? environmentApi : _defaultRenderApi;
    if (api == ShadChartType::Software)
    {
        return ShadChartType::Software;
    }
    if (api == ShadChartType::Auto)
    {
        // 第一项为平台默认
        const QList<ShadChartType::RenderApi> apis = ShadChartView::availableApis();
        if (apis.isEmpty())
        {
            return ShadChartType::Software;
        }
        api = apis.first();
    }
    // 检测结果按 API 缓存；图表只在 GUI 线程创建，无需加锁
    static QMap<ShadChartType::RenderApi, bool> supportedApis;
    auto it = supportedApis.find(api);
    if (it == supportedApis.end())
    {
        const bool isSupported = ShadChartView::isSupported(api);
        if (!isSupported)
        {
            qWarning("%s 不可用，图表改用 CPU 绘制", apiEnum.valueToKey(api));
        }
        it = supportedApis.insert(api, isSupported);
    }
    return it.value() ? api : ShadChartType::Software;
#endif
}

QColor ShadChartPrivate::surfaceColor() const
{
    return shadTheme->getColor(shadTheme->getThemeMode(), _surfaceColor);
}

void ShadChartPrivate::requestUpdate()
{
    Q_Q(ShadChart);
    if (_isUpdatePending)
    {
        return;
    }
    _isUpdatePending = true;
    // 以图表为上下文排队执行，图表销毁时未执行的调用随之丢弃；期间若已同步重建（例如尺寸变化），这里不再重复
    QMetaObject::invokeMethod(q, [this]()
    {
        if (_isUpdatePending)
        {
            updateChart();
        }
    }, Qt::QueuedConnection);
}

void ShadChartPrivate::updateHover(const QPointF& position)
{
    Q_UNUSED(position);
}

void ShadChartPrivate::clearHover()
{
}

bool ShadChartPrivate::zoomAt(const QPointF& position, int angleDelta)
{
    Q_UNUSED(position);
    Q_UNUSED(angleDelta);
    return false;
}

bool ShadChartPrivate::zoomByFactor(const QPointF& position, qreal factor)
{
    Q_UNUSED(position);
    Q_UNUSED(factor);
    return false;
}

bool ShadChartPrivate::beginPan(const QPointF& position)
{
    Q_UNUSED(position);
    return false;
}

void ShadChartPrivate::panTo(const QPointF& position)
{
    Q_UNUSED(position);
}

void ShadChartPrivate::endPan()
{
}

bool ShadChartPrivate::handleDoubleClick(const QPointF& position)
{
    Q_UNUSED(position);
    return false;
}

void ShadChartPrivate::plotViewCreated()
{
}

void ShadChartPrivate::placePlotView(const QRect& rect, bool isVisible)
{
#if defined(SHADCN_CHART_GPU)
    if (_view != nullptr)
    {
        _view->setGeometry(rect);
        _view->setVisible(isVisible);
        _view->setClearColor(surfaceColor());
    }
#endif
    if (_softwareView != nullptr)
    {
        _softwareView->setGeometry(rect);
        _softwareView->setVisible(isVisible);
        _softwareView->update();
    }
}

void ShadChartPrivate::updateOverlay(const QRect& plotRect, bool isHoverChanged)
{
    Q_Q(ShadChart);
    _overlay->setGeometry(q->rect());
    _overlay->raise();
    const bool isPlotMoved = plotRect != _overlayPlotRect;
    _overlayPlotRect = plotRect;
    if (isHoverChanged || isPlotMoved)
    {
        _overlay->update();
    }
    else
    {
        _overlay->update(QRegion(_overlay->rect()).subtracted(QRegion(plotRect)));
    }
}

#if defined(SHADCN_CHART_GPU)
void ShadChartPrivate::fallBackToSoftware()
{
    if (_view == nullptr)
    {
        return;
    }
    qWarning("图表 %s 绘制失败，改用 CPU 绘制", QMetaEnum::fromType<ShadChartType::RenderApi>().valueToKey(_renderApi));
    replacePlotView(ShadChartType::Software);
}
#endif
