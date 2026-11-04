#include "ImageViewerPage.h"

#include <QByteArray>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QIODevice>
#include <QImageReader>
#include <QLatin1Char>
#include <QLinearGradient>
#include <QList>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ExampleI18n.h"
#include "ShadAttachment.h"
#include "ShadAttachmentGroup.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadImageViewer.h"

namespace
{
    // 长图上每隔一段画一条刻度，方便观察拖动位置
    constexpr int kRulerStep = 400;
    constexpr int kLabelFontSize = 48;
    constexpr int kRulerFontSize = 28;
    constexpr int kCircleCount = 6;
    constexpr int kThumbnailSize = 192;

    struct SampleImage
    {
        ExampleI18n::TextFunction title;
        QSize size;
        QColor top;
        QColor bottom;
    };

    // 生成不依赖本地文件的示例图片
    QPixmap createSample(const SampleImage& sample)
    {
        const QSize size = sample.size;
        QPixmap pixmap(size);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient gradient(0, 0, size.width(), size.height());
        gradient.setColorAt(0.0, sample.top);
        gradient.setColorAt(1.0, sample.bottom);
        painter.fillRect(pixmap.rect(), gradient);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 46));
        const qreal radius = qMin(size.width(), size.height()) * 0.18;
        for (int index = 0; index < kCircleCount; ++index)
        {
            const qreal x = size.width() * (0.12 + 0.16 * index);
            const qreal y = size.height() * (index % 2 == 0 ? 0.3 : 0.7);
            painter.drawEllipse(QPointF(x, y), radius, radius);
        }
        if (size.height() > size.width() * 2)
        {
            QFont rulerFont = painter.font();
            rulerFont.setPixelSize(kRulerFontSize);
            painter.setFont(rulerFont);
            for (int y = kRulerStep; y < size.height(); y += kRulerStep)
            {
                painter.fillRect(QRect(0, y - 1, size.width(), 2), QColor(255, 255, 255, 120));
                painter.setPen(QColor(255, 255, 255));
                painter.drawText(QRect(16, y - kRulerFontSize - 12, size.width() - 32, kRulerFontSize + 8), Qt::AlignLeft | Qt::AlignVCenter, QString::number(y));
            }
        }
        QFont labelFont = painter.font();
        labelFont.setPixelSize(qMin(kLabelFontSize, size.width() / 8));
        labelFont.setWeight(QFont::DemiBold);
        painter.setFont(labelFont);
        painter.setPen(QColor(255, 255, 255));
        painter.drawText(pixmap.rect(), Qt::AlignCenter, QStringLiteral("%1 × %2").arg(size.width()).arg(size.height()));
        return pixmap;
    }

    // 包含 Qt 支持的全部图片格式的文件过滤条件
    QString imageFileFilter()
    {
        QStringList patterns;
        const QList<QByteArray> formats = QImageReader::supportedImageFormats();
        for (const QByteArray& format : formats)
        {
            patterns.append(QStringLiteral("*.%1").arg(QString::fromLatin1(format)));
        }
        return QCoreApplication::translate("ImageViewerPage", "图片文件 (%1)").arg(patterns.join(QLatin1Char(' ')));
    }
} // namespace

ImageViewerPage::ImageViewerPage(QWidget* parent)
    : ExampleBasePage([] { return tr("图片预览"); }, [] { return tr("ShadImageViewer：覆盖窗口预览图片，支持缩放、拖动、旋转、翻转与多图切换。"); }, parent)
{
    const QList<SampleImage> samples{
        {[] { return tr("横图"); }, QSize(1280, 800), QColor(0x60, 0xa5, 0xfa), QColor(0x1e, 0x3a, 0x8a)},
        {[] { return tr("竖图"); }, QSize(720, 1080), QColor(0xf4, 0x72, 0xb6), QColor(0x83, 0x18, 0x43)},
        {[] { return tr("方图"); }, QSize(960, 960), QColor(0x34, 0xd3, 0x99), QColor(0x06, 0x5f, 0x46)},
        {[] { return tr("长图"); }, QSize(480, 3200), QColor(0xfb, 0xbf, 0x24), QColor(0x9a, 0x34, 0x12)},
        {[] { return tr("小图"); }, QSize(240, 160), QColor(0xa7, 0x8b, 0xfa), QColor(0x4c, 0x1d, 0x95)},
    };
    QList<QPixmap> pixmaps;
    for (const SampleImage& sample : samples)
    {
        pixmaps.append(createSample(sample));
    }

    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("点击缩略图，从对应的图片开始预览；滚轮缩放，拖动平移，双击切换适应窗口与实际大小"); });
    ShadImageViewer* viewer = new ShadImageViewer(this);
    viewer->setPixmaps(pixmaps);
    ShadAttachmentGroup* thumbnailGroup = new ShadAttachmentGroup(this);
    for (int index = 0; index < samples.size(); ++index)
    {
        const SampleImage& sample = samples.at(index);
        ShadAttachment* attachment = new ShadAttachment(thumbnailGroup);
        attachment->setOrientation(Qt::Vertical);
        // 缩略图先缩小，避免附件每次绘制都缩放大图
        attachment->setImage(pixmaps.at(index).scaled(kThumbnailSize, kThumbnailSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        attachment->setDescription(QStringLiteral("%1 × %2").arg(sample.size.width()).arg(sample.size.height()));
        attachment->setClickable(true);
        const ExampleI18n::TextFunction title = sample.title;
        ExampleI18n::bind(attachment, [attachment, title]()
        {
            attachment->setTitle(title());
            attachment->setToolTip(tr("预览%1").arg(title()));
        });
        connect(attachment, &ShadAttachment::clicked, viewer, [viewer, index]() { viewer->open(index); });
        thumbnailGroup->addAttachment(attachment);
    }
    addStretchItem(basicCard, thumbnailGroup);

    ShadCard* localCard = addSection([] { return tr("本地图片"); }, [] { return tr("setImagePaths：在线程池中异步解码，加载时显示加载指示，按 EXIF 方向自动旋转"); });
    ShadButton* localButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    localButton->setShadIcon(ShadIconType::FolderOpen);
    ExampleI18n::bind(localButton, [localButton]() { localButton->setText(tr("选择图片...")); });
    connect(localButton, &ShadButton::clicked, this, [this]()
    {
#ifdef Q_OS_WASM
        const QPointer<ImageViewerPage> guard(this);
        QFileDialog::getOpenFileContent(imageFileFilter(), [guard](const QString& fileName, const QByteArray& content)
        {
            if (guard.isNull() || fileName.isEmpty())
            {
                return;
            }
            const QString path = QDir(QDir::tempPath()).filePath(QFileInfo(fileName).fileName());
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(content) != content.size())
            {
                qWarning("写入临时文件失败: %s", qUtf8Printable(path));
                return;
            }
            file.close();
            ShadImageViewer::preview(guard.data(), QStringList{path}, 0);
        });
        return;
#endif
        const QStringList paths = QFileDialog::getOpenFileNames(this, tr("选择图片"), QString(), imageFileFilter());
        if (paths.isEmpty())
        {
            return;
        }
        ShadImageViewer::preview(this, paths, 0);
    });
    addItem(localCard, localButton);

    ShadCard* errorCard = addSection([] { return tr("加载失败"); }, [] { return tr("文件不存在或无法解码时显示 ImageOff 图标与提示文字"); });
    ShadButton* errorButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    errorButton->setShadIcon(ShadIconType::ImageOff);
    ExampleI18n::bind(errorButton, [errorButton]() { errorButton->setText(tr("打开不存在的图片")); });
    connect(errorButton, &ShadButton::clicked, this, [this]()
    {
        const QStringList paths{QStringLiteral("missing/not-found.png"), QStringLiteral("missing/also-missing.jpg")};
        ShadImageViewer::preview(this, paths, 0);
    });
    addItem(errorCard, errorButton);

    ShadCard* toolbarCard = addSection([] { return tr("隐藏工具栏"); }, [] { return QStringLiteral("setToolbarVisible(false)"); });
    ShadImageViewer* plainViewer = new ShadImageViewer(this);
    plainViewer->setPixmaps(pixmaps.mid(0, 3));
    plainViewer->setToolbarVisible(false);
    ShadButton* plainButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    plainButton->setShadIcon(ShadIconType::Images);
    ExampleI18n::bind(plainButton, [plainButton]() { plainButton->setText(tr("打开预览")); });
    connect(plainButton, &ShadButton::clicked, plainViewer, [plainViewer]() { plainViewer->open(0); });
    addItem(toolbarCard, plainButton);
}

ImageViewerPage::~ImageViewerPage()
{
}
