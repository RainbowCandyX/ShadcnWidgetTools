#include "ShadImageLoadTask.h"

#include <QColorSpace>
#include <QCoreApplication>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QList>
#include <QMetaObject>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

namespace
{
    constexpr int kMinLevelSize = 64;
    constexpr int kMaxLevelCount = 10;
    constexpr float kToneMapKnee = 0.75f;
    constexpr float kPqReferenceWhite = 1.2992f;
    constexpr float kHlgReferenceWhite = 3.1796f;

    bool isFloatFormat(QImage::Format format)
    {
        switch (format)
        {
            case QImage::Format_RGBX16FPx4:
            case QImage::Format_RGBA16FPx4:
            case QImage::Format_RGBA16FPx4_Premultiplied:
            case QImage::Format_RGBX32FPx4:
            case QImage::Format_RGBA32FPx4:
            case QImage::Format_RGBA32FPx4_Premultiplied:
                return true;
            default:
                return false;
        }
    }

    float referenceWhite(const QColorSpace& colorSpace)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        if (colorSpace.transferFunction() == QColorSpace::TransferFunction::St2084)
        {
            return kPqReferenceWhite;
        }
        if (colorSpace.transferFunction() == QColorSpace::TransferFunction::Hlg)
        {
            return kHlgReferenceWhite;
        }
#else
        Q_UNUSED(colorSpace);
#endif
        return 1.0f;
    }

    float toneMap(float value, float peak)
    {
        if (value <= kToneMapKnee)
        {
            return value;
        }
        const float range = 1.0f - kToneMapKnee;
        const float t = (value - kToneMapKnee) / range;
        const float w = (peak - kToneMapKnee) / range;
        return kToneMapKnee + range * t * (1.0f + t / (w * w)) / (1.0f + t);
    }

    QImage toSdrImage(const QImage& image)
    {
        const QColorSpace source = image.colorSpace();
        const float white = referenceWhite(source);
        if (!isFloatFormat(image.format()) && white == 1.0f)
        {
            if (!source.isValid() || source == QColorSpace(QColorSpace::SRgb))
            {
                return image;
            }
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            return image.convertedToColorSpace(QColorSpace::SRgb, image.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32);
#else
            return image.convertedToColorSpace(QColorSpace::SRgb);
#endif
        }
        QImage linear = image.convertToFormat(image.hasAlphaChannel() ? QImage::Format_RGBA32FPx4 : QImage::Format_RGBX32FPx4);
        if (source.isValid())
        {
            linear.convertToColorSpace(QColorSpace::SRgbLinear);
        }
        float peak = 0.0f;
        for (int y = 0; y < linear.height(); ++y)
        {
            float* pixel = reinterpret_cast<float*>(linear.scanLine(y));
            for (int x = 0; x < linear.width(); ++x, pixel += 4)
            {
                pixel[0] = qMax(pixel[0] / white, 0.0f);
                pixel[1] = qMax(pixel[1] / white, 0.0f);
                pixel[2] = qMax(pixel[2] / white, 0.0f);
                peak = qMax(peak, qMax(pixel[0], qMax(pixel[1], pixel[2])));
            }
        }
        if (peak > 1.0f)
        {
            for (int y = 0; y < linear.height(); ++y)
            {
                float* pixel = reinterpret_cast<float*>(linear.scanLine(y));
                for (int x = 0; x < linear.width(); ++x, pixel += 4)
                {
                    const float maximum = qMax(pixel[0], qMax(pixel[1], pixel[2]));
                    if (maximum > kToneMapKnee)
                    {
                        const float scale = toneMap(maximum, peak) / maximum;
                        pixel[0] *= scale;
                        pixel[1] *= scale;
                        pixel[2] *= scale;
                    }
                }
            }
        }
        linear.setColorSpace(QColorSpace::SRgbLinear);
        linear.convertToColorSpace(QColorSpace::SRgb);
        return linear;
    }
} // namespace

ShadImageLoadTask::ShadImageLoadTask(const QString& path, Callback callback)
    : _path(path), _callback(std::move(callback))
{
    setAutoDelete(true);
}

ShadImageLoadTask::~ShadImageLoadTask()
{
}

void ShadImageLoadTask::run()
{
    QList<QImage> levels;
    QString errorString;
    const QFileInfo fileInfo(_path);
    if (!fileInfo.isFile() || !fileInfo.isReadable())
    {
        errorString = QStringLiteral("文件不存在或不可读");
    }
    else
    {
        // 解码后过大的图片会被 Qt 默认的分配上限拒绝，按加载失败处理
        QImageReader reader(_path);
        reader.setAutoTransform(true);
        const QImage image = reader.read();
        if (image.isNull())
        {
            errorString = reader.errorString();
        }
        else
        {
            levels = buildLevels(image);
        }
    }
    QCoreApplication* application = QCoreApplication::instance();
    if (application == nullptr)
    {
        return;
    }
    // QCoreApplication 析构时会等待全局线程池结束，这里投递时应用对象一定存在
    QMetaObject::invokeMethod(application, [callback = _callback, levels = std::move(levels), errorString = std::move(errorString)]() { callback(levels, errorString); }, Qt::QueuedConnection);
}

QList<QImage> ShadImageLoadTask::buildLevels(const QImage& image)
{
    QList<QImage> levels;
    if (image.isNull())
    {
        return levels;
    }
    QImage current = toSdrImage(image).convertToFormat(image.hasAlphaChannel() ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);
    current.setDevicePixelRatio(1.0);
    levels.append(current);
    while (levels.size() < kMaxLevelCount && qMin(current.width(), current.height()) / 2 >= kMinLevelSize)
    {
        current = current.scaled(current.width() / 2, current.height() / 2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        levels.append(current);
    }
    return levels;
}
