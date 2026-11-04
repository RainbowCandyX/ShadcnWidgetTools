#include "ShadChartView.h"

#include <QByteArray>
#include <QFile>
#include <QIODevice>
#include <QImage>
#include <QMatrix4x4>
#include <QPoint>
#include <QSize>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <rhi/qrhi.h>
#include <rhi/qshader.h>
// QT_CONFIG 需要 QtGui 的配置头文件，放在 rhi/qrhi.h 之后
#if QT_CONFIG(opengl)
#include <QOffscreenSurface>
#endif
// 与 QRhiVulkanInitParams 的条件一致
#if QT_CONFIG(vulkan) && __has_include(<vulkan/vulkan.h>)
#define SHAD_CHART_HAS_VULKAN 1
#include <QVulkanInstance>
#else
#define SHAD_CHART_HAS_VULKAN 0
#endif

#include <cstddef>
#include <cstring>
#include <memory>
#include <utility>

namespace
{
    // 顶点格式：vec2 坐标（2 个 float）+ 颜色（4 个归一化字节）
    static_assert(sizeof(ShadChartVertex) == 2 * sizeof(float) + 4, "ShadChartVertex 必须是紧密排列的 2 个 float 与 4 个字节");
    constexpr quint32 kVertexStride = sizeof(ShadChartVertex);
    constexpr quint32 kColorOffset = 2 * sizeof(float);
    // 单位四边形，按三角形带顺序排列
    constexpr float kCorners[] = {-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f};
    constexpr quint32 kCornerStride = 2 * sizeof(float);
    constexpr quint32 kCornerCount = 4;
    // 统一缓冲：三角形带为一个 mat4；点与图像为 mat4 + 两个 vec4
    constexpr quint32 kStripUniformSize = 16 * sizeof(float);
    constexpr quint32 kExtendedUniformSize = 24 * sizeof(float);
    // MSAA 采样数
    constexpr int kSampleCount = 4;
    // 待上传区间超过这个数量时整组上传
    constexpr int kMaxDirtyRanges = 32;
    constexpr const char* kStripVertexShaderPath = ":/ShadcnWidgetTools/Shader/chart.vert.qsb";
    constexpr const char* kStripFragmentShaderPath = ":/ShadcnWidgetTools/Shader/chart.frag.qsb";
    constexpr const char* kPointVertexShaderPath = ":/ShadcnWidgetTools/Shader/chartpoint.vert.qsb";
    constexpr const char* kPointFragmentShaderPath = ":/ShadcnWidgetTools/Shader/chartpoint.frag.qsb";
    constexpr const char* kImageVertexShaderPath = ":/ShadcnWidgetTools/Shader/chartimage.vert.qsb";
    constexpr const char* kImageFragmentShaderPath = ":/ShadcnWidgetTools/Shader/chartimage.frag.qsb";
} // namespace

ShadChartView::ShadChartView(ShadChartType::RenderApi api, QWidget* parent)
    : QRhiWidget(parent)
{
    // 必须在显示前设置；同一窗口内的 QRhiWidget 需使用相同的图形 API
    switch (api)
    {
        case ShadChartType::OpenGL:
            setApi(QRhiWidget::Api::OpenGL);
            break;
        case ShadChartType::Direct3D11:
            setApi(QRhiWidget::Api::Direct3D11);
            break;
        case ShadChartType::Direct3D12:
            setApi(QRhiWidget::Api::Direct3D12);
            break;
        case ShadChartType::Vulkan:
            setApi(QRhiWidget::Api::Vulkan);
            break;
        case ShadChartType::Metal:
            setApi(QRhiWidget::Api::Metal);
            break;
        case ShadChartType::Auto:
        case ShadChartType::Software:
            qWarning("ShadChartView 收到非 GPU API %d，使用默认 API", static_cast<int>(api));
            break;
    }
    setSampleCount(kSampleCount);
    // 鼠标交互由上方的覆盖层处理
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

ShadChartView::~ShadChartView()
{
}

QList<ShadChartType::RenderApi> ShadChartView::availableApis()
{
    // 第一项为平台默认
    QList<ShadChartType::RenderApi> apis;
#if defined(Q_OS_WIN)
    apis.append(ShadChartType::Direct3D11);
    apis.append(ShadChartType::Direct3D12);
#endif
#if QT_CONFIG(metal)
    apis.append(ShadChartType::Metal);
#endif
#if QT_CONFIG(opengl)
    apis.append(ShadChartType::OpenGL);
#endif
#if SHAD_CHART_HAS_VULKAN
    apis.append(ShadChartType::Vulkan);
#endif
    return apis;
}

bool ShadChartView::isSupported(ShadChartType::RenderApi api)
{
    switch (api)
    {
#if defined(Q_OS_WIN)
        case ShadChartType::Direct3D11:
            {
                QRhiD3D11InitParams params;
                return QRhi::probe(QRhi::D3D11, &params);
            }
        case ShadChartType::Direct3D12:
            {
                QRhiD3D12InitParams params;
                return QRhi::probe(QRhi::D3D12, &params);
            }
#endif
#if QT_CONFIG(metal)
        case ShadChartType::Metal:
            {
                QRhiMetalInitParams params;
                return QRhi::probe(QRhi::Metal, &params);
            }
#endif
#if QT_CONFIG(opengl)
        case ShadChartType::OpenGL:
            {
                // 创建上下文需要临时的离屏表面
                QRhiGles2InitParams params;
                std::unique_ptr<QOffscreenSurface> surface(QRhiGles2InitParams::newFallbackSurface());
                params.fallbackSurface = surface.get();
                return QRhi::probe(QRhi::OpenGLES2, &params);
            }
#endif
#if SHAD_CHART_HAS_VULKAN
        case ShadChartType::Vulkan:
            {
                // 临时实例只用于检测
                QVulkanInstance instance;
                if (!instance.create())
                {
                    return false;
                }
                QRhiVulkanInitParams params;
                params.inst = &instance;
                return QRhi::probe(QRhi::Vulkan, &params);
            }
#endif
        default:
            return false;
    }
}

void ShadChartView::setVertices(const QList<ShadChartVertex>& vertices)
{
    _vertices = vertices;
    _isVertexDirty = true;
    update();
}

void ShadChartView::setClearColor(const QColor& color)
{
    if (_clearColor == color)
    {
        return;
    }
    _clearColor = color;
    update();
}

void ShadChartView::setPointTransform(qreal scaleX, qreal offsetX, qreal scaleY, qreal offsetY, qreal radius)
{
    _pointTransform[0] = static_cast<float>(scaleX);
    _pointTransform[1] = static_cast<float>(offsetX);
    _pointTransform[2] = static_cast<float>(scaleY);
    _pointTransform[3] = static_cast<float>(offsetY);
    _pointRadius = static_cast<float>(radius);
    update();
}

void ShadChartView::setPointGroupCount(int count)
{
    const std::size_t size = static_cast<std::size_t>(qMax(0, count));
    if (_pointGroups.size() == size)
    {
        return;
    }
    _pointGroups.resize(size);
    update();
}

void ShadChartView::resetPointGroup(int group, int capacity)
{
    if (group < 0 || static_cast<std::size_t>(group) >= _pointGroups.size())
    {
        return;
    }
    PointGroup& target = _pointGroups[static_cast<std::size_t>(group)];
    target.points = QList<ShadChartVertex>(qMax(0, capacity));
    target.dirtyRanges.clear();
    target.isFullDirty = true;
    target.first = 0;
    target.count = 0;
    update();
}

void ShadChartView::writePoints(int group, int slot, const ShadChartVertex* points, int count)
{
    if (group < 0 || static_cast<std::size_t>(group) >= _pointGroups.size() || count <= 0)
    {
        return;
    }
    PointGroup& target = _pointGroups[static_cast<std::size_t>(group)];
    if (slot < 0 || slot + count > target.points.size())
    {
        qWarning("图表写入点越界：位置 %d，数量 %d，容量 %lld", slot, count, static_cast<long long>(target.points.size()));
        return;
    }
    std::memcpy(target.points.data() + slot, points, static_cast<std::size_t>(count) * sizeof(ShadChartVertex));
    if (!target.isFullDirty)
    {
        target.dirtyRanges.append(qMakePair(slot, count));
        if (target.dirtyRanges.size() > kMaxDirtyRanges)
        {
            target.isFullDirty = true;
            target.dirtyRanges.clear();
        }
    }
    update();
}

void ShadChartView::setPointRange(int group, int first, int count)
{
    if (group < 0 || static_cast<std::size_t>(group) >= _pointGroups.size())
    {
        return;
    }
    PointGroup& target = _pointGroups[static_cast<std::size_t>(group)];
    const int capacity = static_cast<int>(target.points.size());
    target.first = capacity > 0 ? qBound(0, first, capacity - 1) : 0;
    target.count = qBound(0, count, capacity);
    update();
}

void ShadChartView::replacePoints(int group, const QList<ShadChartVertex>& points)
{
    if (group < 0 || static_cast<std::size_t>(group) >= _pointGroups.size())
    {
        return;
    }
    PointGroup& target = _pointGroups[static_cast<std::size_t>(group)];
    target.points = points;
    target.dirtyRanges.clear();
    target.isFullDirty = true;
    target.first = 0;
    target.count = static_cast<int>(points.size());
    update();
}

void ShadChartView::resetImage(int width, int capacity)
{
    if (width > 0 && capacity > 0)
    {
        _image = QImage(width, capacity, QImage::Format_RGBA8888_Premultiplied);
        _image.fill(Qt::transparent);
    }
    else
    {
        _image = QImage();
    }
    _imageDirtyRows.clear();
    _isImageFullDirty = true;
    _imageFirst = 0;
    _imageCount = 0;
    _isImageTooLarge = false;
    update();
}

void ShadChartView::writeImageRows(int slot, const QImage& rows)
{
    if (_image.isNull() || rows.isNull())
    {
        return;
    }
    if (rows.width() != _image.width() || slot < 0 || slot + rows.height() > _image.height())
    {
        qWarning("图表写入图像越界：位置 %d，%d x %d，图像 %d x %d", slot, rows.width(), rows.height(), _image.width(), _image.height());
        return;
    }
    const QImage source = rows.format() == QImage::Format_RGBA8888_Premultiplied ? rows : rows.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    const qsizetype rowBytes = static_cast<qsizetype>(_image.width()) * 4;
    for (int row = 0; row < source.height(); ++row)
    {
        std::memcpy(_image.scanLine(slot + row), source.constScanLine(row), static_cast<std::size_t>(rowBytes));
    }
    if (!_isImageFullDirty)
    {
        _imageDirtyRows.append(qMakePair(slot, source.height()));
        if (_imageDirtyRows.size() > kMaxDirtyRanges)
        {
            _isImageFullDirty = true;
            _imageDirtyRows.clear();
        }
    }
    update();
}

void ShadChartView::setImageRange(int first, int count, bool isMirrored)
{
    const int capacity = _image.isNull() ? 0 : _image.height();
    _imageFirst = capacity > 0 ? qBound(0, first, capacity - 1) : 0;
    _imageCount = qBound(0, count, capacity);
    _isImageMirrored = isMirrored;
    update();
}

int ShadChartView::maxTextureSize() const
{
    return _rhi != nullptr ? _rhi->resourceLimit(QRhi::TextureSizeMax) : 0;
}

void ShadChartView::initialize(QRhiCommandBuffer* cb)
{
    Q_UNUSED(cb);
    // 图形设备重建（例如窗口移到另一块显卡的屏幕）后旧资源全部失效
    if (_rhi != rhi())
    {
        releaseResources();
        _rhi = rhi();
    }
    // 采样数变化后渲染目标的格式随之变化，管线必须重建
    if (_pipelineSampleCount != 0 && _pipelineSampleCount != renderTarget()->sampleCount())
    {
        _stripPipeline.reset();
        _pointPipeline.reset();
        _imagePipeline.reset();
    }
    _pipelineSampleCount = renderTarget()->sampleCount();
    if (_stripPipeline == nullptr)
    {
        createStripPipeline();
    }
    if (_pointPipeline == nullptr)
    {
        createPointPipeline();
    }
}

void ShadChartView::render(QRhiCommandBuffer* cb)
{
    QRhiResourceUpdateBatch* updates = _rhi->nextResourceUpdateBatch();
    // 逻辑像素坐标，原点在左上角；clipSpaceCorrMatrix 修正各图形 API 的裁剪空间差异
    QMatrix4x4 mvp = _rhi->clipSpaceCorrMatrix();
    mvp.ortho(0.0f, static_cast<float>(width()), static_cast<float>(height()), 0.0f, -1.0f, 1.0f);

    const bool hasImage = uploadImage(updates);
    const bool canDrawImage = hasImage && (_imagePipeline != nullptr || createImagePipeline());
    if (canDrawImage)
    {
        const float capacity = static_cast<float>(_image.height());
        const float size[4] = {static_cast<float>(width()), static_cast<float>(height()), 0.0f, 0.0f};
        const float range[4] = {_isImageMirrored ? -1.0f : 1.0f, _isImageMirrored ? 1.0f : 0.0f, static_cast<float>(_imageFirst) / capacity, static_cast<float>(_imageCount) / capacity};
        updates->updateDynamicBuffer(_imageUniformBuffer.get(), 0, 16 * sizeof(float), mvp.constData());
        updates->updateDynamicBuffer(_imageUniformBuffer.get(), 16 * sizeof(float), 4 * sizeof(float), size);
        updates->updateDynamicBuffer(_imageUniformBuffer.get(), 20 * sizeof(float), 4 * sizeof(float), range);
    }
    const bool canDrawStrip = _stripPipeline != nullptr && uploadVertices(updates) && _uploadedVertexCount > 0;
    if (canDrawStrip)
    {
        updates->updateDynamicBuffer(_stripUniformBuffer.get(), 0, kStripUniformSize, mvp.constData());
    }
    bool hasPoints = false;
    for (const PointGroup& group : _pointGroups)
    {
        hasPoints = hasPoints || (group.count > 0 && !group.points.isEmpty());
    }
    const bool canDrawPoints = _pointPipeline != nullptr && uploadPoints(updates);
    if (canDrawPoints)
    {
        const float params[4] = {_pointRadius, 0.0f, 0.0f, 0.0f};
        updates->updateDynamicBuffer(_pointUniformBuffer.get(), 0, 16 * sizeof(float), mvp.constData());
        updates->updateDynamicBuffer(_pointUniformBuffer.get(), 16 * sizeof(float), 4 * sizeof(float), _pointTransform);
        updates->updateDynamicBuffer(_pointUniformBuffer.get(), 20 * sizeof(float), 4 * sizeof(float), params);
    }
    if (_cornerBuffer != nullptr && !_isCornerUploaded)
    {
        updates->uploadStaticBuffer(_cornerBuffer.get(), kCorners);
        _isCornerUploaded = true;
    }
    // 要画的内容缺少管线时改用 CPU 绘制
    const bool isStripMissing = !_vertices.isEmpty() && _stripPipeline == nullptr;
    const bool isPointMissing = hasPoints && _pointPipeline == nullptr;
    const bool isImageMissing = hasImage && !canDrawImage;
    if ((isStripMissing || isPointMissing || isImageMissing) && !_isFailureReported)
    {
        qWarning("图表缺少可用的渲染管线（三角形带 %d、点 %d、图像 %d）", isStripMissing ? 0 : 1, isPointMissing ? 0 : 1, isImageMissing ? 0 : 1);
        _isFailureReported = true;
        Q_EMIT renderFailed();
    }

    cb->beginPass(renderTarget(), _clearColor, QRhiDepthStencilClearValue(1.0f, 0), updates);
    const QSize outputSize = renderTarget()->pixelSize();
    const QRhiViewport viewport(0.0f, 0.0f, static_cast<float>(outputSize.width()), static_cast<float>(outputSize.height()));
    if (canDrawImage)
    {
        cb->setGraphicsPipeline(_imagePipeline.get());
        cb->setViewport(viewport);
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput cornerInput(_cornerBuffer.get(), 0);
        cb->setVertexInput(0, 1, &cornerInput);
        cb->draw(kCornerCount);
    }
    if (canDrawStrip)
    {
        cb->setGraphicsPipeline(_stripPipeline.get());
        cb->setViewport(viewport);
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput vertexInput(_vertexBuffer.get(), 0);
        cb->setVertexInput(0, 1, &vertexInput);
        cb->draw(_uploadedVertexCount);
    }
    if (canDrawPoints)
    {
        cb->setGraphicsPipeline(_pointPipeline.get());
        cb->setViewport(viewport);
        cb->setShaderResources();
        for (const PointGroup& group : _pointGroups)
        {
            const int capacity = static_cast<int>(group.points.size());
            if (group.buffer == nullptr || group.count <= 0 || capacity <= 0)
            {
                continue;
            }
            // 环形区间最多两段；用缓冲偏移代替 firstInstance，不依赖 BaseInstance
            const int firstLength = qMin(group.count, capacity - group.first);
            const QPair<int, int> segments[2] = {qMakePair(group.first, firstLength), qMakePair(0, group.count - firstLength)};
            for (const QPair<int, int>& segment : segments)
            {
                if (segment.second <= 0)
                {
                    continue;
                }
                const QRhiCommandBuffer::VertexInput inputs[2] = {QRhiCommandBuffer::VertexInput(_cornerBuffer.get(), 0), QRhiCommandBuffer::VertexInput(group.buffer.get(), static_cast<quint32>(segment.first) * kVertexStride)};
                cb->setVertexInput(0, 2, inputs);
                cb->draw(kCornerCount, static_cast<quint32>(segment.second));
            }
        }
    }
    cb->endPass();
}

void ShadChartView::releaseResources()
{
    _stripPipeline.reset();
    _stripBindings.reset();
    _stripUniformBuffer.reset();
    _vertexBuffer.reset();
    _pointPipeline.reset();
    _pointBindings.reset();
    _pointUniformBuffer.reset();
    _imagePipeline.reset();
    _imageBindings.reset();
    _imageUniformBuffer.reset();
    _imageTexture.reset();
    _imageSampler.reset();
    _cornerBuffer.reset();
    _pipelineSampleCount = 0;
    _uploadedVertexCount = 0;
    // 新的图形设备上需要重新上传全部数据
    _isVertexDirty = true;
    _isCornerUploaded = false;
    for (PointGroup& group : _pointGroups)
    {
        group.buffer.reset();
        group.dirtyRanges.clear();
        group.isFullDirty = true;
    }
    _imageDirtyRows.clear();
    _isImageFullDirty = true;
}

bool ShadChartView::createStripPipeline()
{
    const QShader vertexShader = loadShader(QString::fromLatin1(kStripVertexShaderPath));
    const QShader fragmentShader = loadShader(QString::fromLatin1(kStripFragmentShaderPath));
    if (!vertexShader.isValid() || !fragmentShader.isValid())
    {
        return false;
    }
    if (_stripUniformBuffer == nullptr)
    {
        _stripUniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, kStripUniformSize));
        if (!_stripUniformBuffer->create())
        {
            qWarning("图表创建三角形带统一缓冲失败");
            _stripUniformBuffer.reset();
            return false;
        }
    }
    _stripBindings.reset(_rhi->newShaderResourceBindings());
    _stripBindings->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, _stripUniformBuffer.get())});
    if (!_stripBindings->create())
    {
        qWarning("图表创建三角形带资源绑定失败");
        _stripBindings.reset();
        return false;
    }
    QRhiVertexInputLayout layout;
    layout.setBindings({QRhiVertexInputBinding(kVertexStride)});
    layout.setAttributes({QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0), QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::UNormByte4, kColorOffset)});
    _stripPipeline.reset(createPipeline(vertexShader, fragmentShader, layout, _stripBindings.get()));
    return _stripPipeline != nullptr;
}

bool ShadChartView::createPointPipeline()
{
    // 不支持实例化时（例如 OpenGL ES 2.0）实例属性会按顶点步进
    if (!_rhi->isFeatureSupported(QRhi::Instancing))
    {
        return false;
    }
    const QShader vertexShader = loadShader(QString::fromLatin1(kPointVertexShaderPath));
    const QShader fragmentShader = loadShader(QString::fromLatin1(kPointFragmentShaderPath));
    if (!vertexShader.isValid() || !fragmentShader.isValid() || !createCornerBuffer())
    {
        return false;
    }
    if (_pointUniformBuffer == nullptr)
    {
        _pointUniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, kExtendedUniformSize));
        if (!_pointUniformBuffer->create())
        {
            qWarning("图表创建点统一缓冲失败");
            _pointUniformBuffer.reset();
            return false;
        }
    }
    _pointBindings.reset(_rhi->newShaderResourceBindings());
    _pointBindings->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, _pointUniformBuffer.get())});
    if (!_pointBindings->create())
    {
        qWarning("图表创建点资源绑定失败");
        _pointBindings.reset();
        return false;
    }
    // 绑定 0 为四边形顶点，绑定 1 为每个点的实例数据
    QRhiVertexInputLayout layout;
    layout.setBindings({QRhiVertexInputBinding(kCornerStride), QRhiVertexInputBinding(kVertexStride, QRhiVertexInputBinding::PerInstance)});
    layout.setAttributes({QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0), QRhiVertexInputAttribute(1, 1, QRhiVertexInputAttribute::Float2, 0), QRhiVertexInputAttribute(1, 2, QRhiVertexInputAttribute::UNormByte4, kColorOffset)});
    _pointPipeline.reset(createPipeline(vertexShader, fragmentShader, layout, _pointBindings.get()));
    return _pointPipeline != nullptr;
}

bool ShadChartView::createCornerBuffer()
{
    if (_cornerBuffer != nullptr)
    {
        return true;
    }
    _cornerBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(kCorners)));
    if (!_cornerBuffer->create())
    {
        qWarning("图表创建顶点缓冲失败");
        _cornerBuffer.reset();
        return false;
    }
    _isCornerUploaded = false;
    return true;
}

bool ShadChartView::createImagePipeline()
{
    if (_imageTexture == nullptr || !createCornerBuffer())
    {
        return false;
    }
    const QShader vertexShader = loadShader(QString::fromLatin1(kImageVertexShaderPath));
    const QShader fragmentShader = loadShader(QString::fromLatin1(kImageFragmentShaderPath));
    if (!vertexShader.isValid() || !fragmentShader.isValid())
    {
        return false;
    }
    if (_imageUniformBuffer == nullptr)
    {
        _imageUniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, kExtendedUniformSize));
        if (!_imageUniformBuffer->create())
        {
            qWarning("图表创建图像统一缓冲失败");
            _imageUniformBuffer.reset();
            return false;
        }
    }
    if (_imageSampler == nullptr)
    {
        // 一个单元格一个纹素，不插值
        _imageSampler.reset(_rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        if (!_imageSampler->create())
        {
            qWarning("图表创建采样器失败");
            _imageSampler.reset();
            return false;
        }
    }
    _imageBindings.reset(_rhi->newShaderResourceBindings());
    _imageBindings->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, _imageUniformBuffer.get()), QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, _imageTexture.get(), _imageSampler.get())});
    if (!_imageBindings->create())
    {
        qWarning("图表创建图像资源绑定失败");
        _imageBindings.reset();
        return false;
    }
    QRhiVertexInputLayout layout;
    layout.setBindings({QRhiVertexInputBinding(kCornerStride)});
    layout.setAttributes({QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0)});
    _imagePipeline.reset(createPipeline(vertexShader, fragmentShader, layout, _imageBindings.get()));
    return _imagePipeline != nullptr;
}

QRhiGraphicsPipeline* ShadChartView::createPipeline(const QShader& vertexShader, const QShader& fragmentShader, const QRhiVertexInputLayout& inputLayout, QRhiShaderResourceBindings* bindings)
{
    std::unique_ptr<QRhiGraphicsPipeline> pipeline(_rhi->newGraphicsPipeline());
    // 预乘透明度混合
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::One;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    pipeline->setTargetBlends({blend});
    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    pipeline->setSampleCount(renderTarget()->sampleCount());
    pipeline->setShaderStages({QRhiShaderStage(QRhiShaderStage::Vertex, vertexShader), QRhiShaderStage(QRhiShaderStage::Fragment, fragmentShader)});
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(bindings);
    pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    if (!pipeline->create())
    {
        qWarning("图表创建渲染管线失败");
        return nullptr;
    }
    return pipeline.release();
}

bool ShadChartView::uploadVertices(QRhiResourceUpdateBatch* updates)
{
    if (!_isVertexDirty)
    {
        return true;
    }
    _isVertexDirty = false;
    _uploadedVertexCount = 0;
    if (_vertices.isEmpty())
    {
        return true;
    }
    const quint32 byteSize = static_cast<quint32>(_vertices.size()) * kVertexStride;
    if (_vertexBuffer == nullptr || _vertexBuffer->size() < byteSize)
    {
        // 预留一半余量，数据小幅增长时不必重建缓冲
        // Dynamic：CPU 可直接写入，适合实时数据每帧都更新顶点，省去 Static 上传时经过暂存缓冲的一次拷贝
        _vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, byteSize + byteSize / 2));
        if (!_vertexBuffer->create())
        {
            qWarning("图表创建顶点缓冲失败，大小 %u 字节", byteSize + byteSize / 2);
            _vertexBuffer.reset();
            return false;
        }
    }
    updates->updateDynamicBuffer(_vertexBuffer.get(), 0, byteSize, _vertices.constData());
    _uploadedVertexCount = static_cast<quint32>(_vertices.size());
    return true;
}

bool ShadChartView::uploadPoints(QRhiResourceUpdateBatch* updates)
{
    bool hasPoints = false;
    for (PointGroup& group : _pointGroups)
    {
        const quint32 byteSize = static_cast<quint32>(group.points.size()) * kVertexStride;
        if (byteSize == 0)
        {
            group.buffer.reset();
            group.dirtyRanges.clear();
            group.isFullDirty = false;
            continue;
        }
        // Static：只在追加时上传新增部分
        if (group.buffer == nullptr || group.buffer->size() != byteSize)
        {
            group.buffer.reset(_rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::VertexBuffer, byteSize));
            if (!group.buffer->create())
            {
                qWarning("图表创建点缓冲失败，大小 %u 字节", byteSize);
                group.buffer.reset();
                continue;
            }
            group.isFullDirty = true;
        }
        if (group.isFullDirty)
        {
            updates->uploadStaticBuffer(group.buffer.get(), 0, byteSize, group.points.constData());
        }
        else
        {
            for (const QPair<int, int>& range : std::as_const(group.dirtyRanges))
            {
                updates->uploadStaticBuffer(group.buffer.get(), static_cast<quint32>(range.first) * kVertexStride, static_cast<quint32>(range.second) * kVertexStride, group.points.constData() + range.first);
            }
        }
        group.dirtyRanges.clear();
        group.isFullDirty = false;
        hasPoints = hasPoints || group.count > 0;
    }
    return hasPoints;
}

bool ShadChartView::uploadImage(QRhiResourceUpdateBatch* updates)
{
    if (_image.isNull() || _isImageTooLarge)
    {
        _imageTexture.reset();
        _imagePipeline.reset();
        _imageBindings.reset();
        return false;
    }
    const QSize imageSize = _image.size();
    if (_imageTexture == nullptr || _imageTexture->pixelSize() != imageSize)
    {
        const int maxSize = _rhi->resourceLimit(QRhi::TextureSizeMax);
        if (imageSize.width() > maxSize || imageSize.height() > maxSize)
        {
            qWarning("图表图像 %d x %d 超过纹理尺寸上限 %d", imageSize.width(), imageSize.height(), maxSize);
            _isImageTooLarge = true;
            Q_EMIT renderFailed();
            return false;
        }
        _imagePipeline.reset();
        _imageBindings.reset();
        _imageTexture.reset(_rhi->newTexture(QRhiTexture::RGBA8, imageSize));
        if (!_imageTexture->create())
        {
            qWarning("图表创建纹理失败，%d x %d", imageSize.width(), imageSize.height());
            _imageTexture.reset();
            return false;
        }
        _isImageFullDirty = true;
    }
    if (_isImageFullDirty)
    {
        updates->uploadTexture(_imageTexture.get(), _image);
    }
    else if (!_imageDirtyRows.isEmpty())
    {
        QList<QRhiTextureUploadEntry> entries;
        for (const QPair<int, int>& rows : std::as_const(_imageDirtyRows))
        {
            QRhiTextureSubresourceUploadDescription description(_image);
            description.setSourceTopLeft(QPoint(0, rows.first));
            description.setSourceSize(QSize(imageSize.width(), rows.second));
            description.setDestinationTopLeft(QPoint(0, rows.first));
            entries.append(QRhiTextureUploadEntry(0, 0, description));
        }
        QRhiTextureUploadDescription upload;
        upload.setEntries(entries.cbegin(), entries.cend());
        updates->uploadTexture(_imageTexture.get(), upload);
    }
    _imageDirtyRows.clear();
    _isImageFullDirty = false;
    return _imageCount > 0;
}

QShader ShadChartView::loadShader(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning("图表加载着色器失败：%s，%s", qPrintable(path), qPrintable(file.errorString()));
        return QShader();
    }
    const QShader shader = QShader::fromSerialized(file.readAll());
    if (!shader.isValid())
    {
        qWarning("图表着色器格式无效：%s", qPrintable(path));
    }
    return shader;
}
