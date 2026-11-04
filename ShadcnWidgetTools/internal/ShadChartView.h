#ifndef SHADCHARTVIEW_H
#define SHADCHARTVIEW_H

#include <QColor>
#include <QImage>
#include <QList>
#include <QPair>
#include <QRhiWidget>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <memory>
#include <vector>

#include "ShadChartHelper.h"
#include "ShadDef.h"

class QRhi;
class QRhiBuffer;
class QRhiCommandBuffer;
class QRhiGraphicsPipeline;
class QRhiResourceUpdateBatch;
class QRhiSampler;
class QRhiShaderResourceBindings;
class QRhiTexture;
class QRhiVertexInputLayout;
class QShader;

// 图表的 GPU 绘制层，4 倍 MSAA，每帧依次绘制图像（热力图）、三角形带（网格、面积、线、柱）与点（实例绘制）
// 点与图像在 CPU 上保留副本，设备重建后整份上传，平时只上传修改的部分
class ShadChartView : public QRhiWidget
{
    Q_OBJECT

    public:
        // api 不能是 Auto 或 Software
        ShadChartView(ShadChartType::RenderApi api, QWidget* parent);
        ~ShadChartView() override;

        // 当前平台可用的 GPU API，第一项为平台默认
        static QList<ShadChartType::RenderApi> availableApis();
        // 用 QRhi::probe 检测 api 能否创建图形设备
        static bool isSupported(ShadChartType::RenderApi api);

        void setVertices(const QList<ShadChartVertex>& vertices);
        // 清屏颜色，应与图表所在表面相同
        void setClearColor(const QColor& color);

        // 逻辑像素 = 数据坐标 * scale + offset
        void setPointTransform(qreal scaleX, qreal offsetX, qreal scaleY, qreal offsetY, qreal radius);
        void setPointGroupCount(int count);
        // 清空一组并设置容量
        void resetPointGroup(int group, int capacity);
        // 不能超出容量
        void writePoints(int group, int slot, const ShadChartVertex* points, int count);
        // 超出容量的部分从开头继续
        void setPointRange(int group, int first, int count);
        // 容量与点数相同
        void replacePoints(int group, const QList<ShadChartVertex>& points);

        // 任一为 0 时不绘制图像
        void resetImage(int width, int capacity);
        // rows 为 RGBA8888 预乘，宽度与图像相同，不能超出容量
        void writeImageRows(int slot, const QImage& rows);
        // 超出容量的行从开头继续
        void setImageRange(int first, int count, bool isMirrored);
        // 图形设备创建前为 0
        int maxTextureSize() const;

    protected:
        void initialize(QRhiCommandBuffer* cb) override;
        void render(QRhiCommandBuffer* cb) override;
        void releaseResources() override;

    private:
        struct PointGroup
        {
            // 容量大小的副本
            QList<ShadChartVertex> points;
            // 待上传的区间（起点、数量）
            QList<QPair<int, int>> dirtyRanges;
            bool isFullDirty{true};
            int first{0};
            int count{0};
            std::unique_ptr<QRhiBuffer> buffer;
        };

        // 失败时保持为空，下次 initialize 时重试
        bool createStripPipeline();
        // 需要实例化绘制
        bool createPointPipeline();
        // 纹理重建后随之重建
        bool createImagePipeline();
        bool createCornerBuffer();
        QRhiGraphicsPipeline* createPipeline(const QShader& vertexShader, const QShader& fragmentShader, const QRhiVertexInputLayout& inputLayout, QRhiShaderResourceBindings* bindings);
        bool uploadVertices(QRhiResourceUpdateBatch* updates);
        // 返回是否有点要画
        bool uploadPoints(QRhiResourceUpdateBatch* updates);
        // 返回是否要画图像
        bool uploadImage(QRhiResourceUpdateBatch* updates);
        static QShader loadShader(const QString& path);

        QRhi* _rhi{nullptr};
        int _pipelineSampleCount{0};
        QColor _clearColor{Qt::white};
        // 缺少管线只报告一次
        bool _isFailureReported{false};

        // 三角形带
        std::unique_ptr<QRhiBuffer> _vertexBuffer;
        std::unique_ptr<QRhiBuffer> _stripUniformBuffer;
        std::unique_ptr<QRhiShaderResourceBindings> _stripBindings;
        std::unique_ptr<QRhiGraphicsPipeline> _stripPipeline;
        QList<ShadChartVertex> _vertices;
        quint32 _uploadedVertexCount{0};
        bool _isVertexDirty{true};

        // 点与图像共用的单位四边形
        std::unique_ptr<QRhiBuffer> _cornerBuffer;
        bool _isCornerUploaded{false};

        // 点
        std::unique_ptr<QRhiBuffer> _pointUniformBuffer;
        std::unique_ptr<QRhiShaderResourceBindings> _pointBindings;
        std::unique_ptr<QRhiGraphicsPipeline> _pointPipeline;
        std::vector<PointGroup> _pointGroups;
        float _pointTransform[4]{1.0f, 0.0f, 1.0f, 0.0f};
        float _pointRadius{3.0f};

        // 图像
        std::unique_ptr<QRhiBuffer> _imageUniformBuffer;
        std::unique_ptr<QRhiShaderResourceBindings> _imageBindings;
        std::unique_ptr<QRhiGraphicsPipeline> _imagePipeline;
        std::unique_ptr<QRhiTexture> _imageTexture;
        std::unique_ptr<QRhiSampler> _imageSampler;
        QImage _image;
        QList<QPair<int, int>> _imageDirtyRows;
        bool _isImageFullDirty{true};
        int _imageFirst{0};
        int _imageCount{0};
        bool _isImageMirrored{false};
        // 超过纹理尺寸上限后不再绘制
        bool _isImageTooLarge{false};
};

#endif // SHADCHARTVIEW_H
