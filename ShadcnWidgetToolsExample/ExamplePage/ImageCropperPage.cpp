#include "ImageCropperPage.h"

#include <QByteArray>
#include <QColor>
#include <QFileDialog>
#include <QFont>
#include <QImage>
#include <QLabel>
#include <QLinearGradient>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <memory>

#include "ExampleI18n.h"
#include "ShadAvatar.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadComboBox.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadImageCropper.h"
#include "ShadLabel.h"

namespace
{
    constexpr int kCropperHeight = 360;
    constexpr int kPreviewLimit = 200;
    constexpr int kSampleCircleCount = 7;

    QImage createSampleImage()
    {
        const QSize size(1200, 800);
        QImage image(size, QImage::Format_ARGB32_Premultiplied);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient gradient(0, 0, size.width(), size.height());
        gradient.setColorAt(0.0, QColor(0x60, 0xa5, 0xfa));
        gradient.setColorAt(0.5, QColor(0xa7, 0x8b, 0xfa));
        gradient.setColorAt(1.0, QColor(0xf4, 0x72, 0xb6));
        painter.fillRect(image.rect(), gradient);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 56));
        for (int index = 0; index < kSampleCircleCount; ++index)
        {
            const qreal x = size.width() * (0.08 + 0.14 * index);
            const qreal y = size.height() * (index % 2 == 0 ? 0.3 : 0.7);
            const qreal radius = size.height() * (0.1 + 0.03 * (index % 3));
            painter.drawEllipse(QPointF(x, y), radius, radius);
        }
        QFont font = painter.font();
        font.setPixelSize(72);
        font.setWeight(QFont::DemiBold);
        painter.setFont(font);
        painter.setPen(Qt::white);
        painter.drawText(image.rect(), Qt::AlignCenter, QStringLiteral("1200 × 800"));
        return image;
    }

    QPixmap toPreviewPixmap(const QImage& image, qreal ratio)
    {
        const int limit = qRound(kPreviewLimit * ratio);
        QImage scaled = image;
        if (image.width() > limit || image.height() > limit)
        {
            scaled = image.scaled(limit, limit, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        QPixmap pixmap = QPixmap::fromImage(scaled);
        pixmap.setDevicePixelRatio(ratio);
        return pixmap;
    }
} // namespace

ImageCropperPage::ImageCropperPage(QWidget* parent)
    : ExampleBasePage([] { return tr("图片裁剪"); }, [] { return tr("ShadImageCropper：在图片上拖动裁剪框，支持固定比例、圆形裁剪、旋转与翻转。"); }, parent)
{
    const QImage sample = createSampleImage();

    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("拖动框内移动，拖动手柄缩放，聚焦后用方向键微调，按住 Shift 加快 10 倍"); });
    ShadImageCropper* cropper = new ShadImageCropper(this);
    cropper->setImage(sample);
    cropper->setFixedHeight(kCropperHeight);
    addStretchItem(basicCard, cropper);

    ShadComboBox* ratioBox = new ShadComboBox(this);
    ratioBox->addItem(QString());
    ratioBox->addItem(QStringLiteral("1:1"));
    ratioBox->addItem(QStringLiteral("4:3"));
    ratioBox->addItem(QStringLiteral("16:9"));
    ratioBox->setFixedWidth(120);
    ExampleI18n::bind(ratioBox, [ratioBox]() { ratioBox->setItemText(0, tr("自由")); });
    connect(ratioBox, &ShadComboBox::currentIndexChanged, cropper, [cropper](int index)
    {
        const QList<qreal> ratios{0.0, 1.0, 4.0 / 3.0, 16.0 / 9.0};
        cropper->setAspectRatio(ratios.value(index, 0.0));
    });

    ShadButton* rotateLeftButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(rotateLeftButton, [rotateLeftButton]() { rotateLeftButton->setText(tr("向左旋转")); });
    connect(rotateLeftButton, &ShadButton::clicked, cropper, &ShadImageCropper::rotateLeft);
    ShadButton* rotateRightButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(rotateRightButton, [rotateRightButton]() { rotateRightButton->setText(tr("向右旋转")); });
    connect(rotateRightButton, &ShadButton::clicked, cropper, &ShadImageCropper::rotateRight);
    ShadButton* flipHorizontalButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(flipHorizontalButton, [flipHorizontalButton]() { flipHorizontalButton->setText(tr("水平翻转")); });
    connect(flipHorizontalButton, &ShadButton::clicked, cropper, &ShadImageCropper::flipHorizontal);
    ShadButton* flipVerticalButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(flipVerticalButton, [flipVerticalButton]() { flipVerticalButton->setText(tr("垂直翻转")); });
    connect(flipVerticalButton, &ShadButton::clicked, cropper, &ShadImageCropper::flipVertical);
    ShadButton* resetButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(resetButton, [resetButton]() { resetButton->setText(tr("重置")); });
    connect(resetButton, &ShadButton::clicked, this, [cropper, ratioBox]()
    {
        ratioBox->setCurrentIndex(0);
        cropper->setAspectRatio(0.0);
        cropper->reset();
    });
    addRow(basicCard, {ratioBox, rotateLeftButton, rotateRightButton, flipHorizontalButton, flipVerticalButton, resetButton});

    ShadButton* cropButton = new ShadButton(QString(), this);
    ExampleI18n::bind(cropButton, [cropButton]() { cropButton->setText(tr("裁剪")); });
    QLabel* previewLabel = new QLabel(this);
    ShadLabel* sizeText = new ShadLabel(this);
    sizeText->setColorRole(ShadThemeType::MutedForeground);
    const std::shared_ptr<QSize> croppedSize = std::make_shared<QSize>();
    const auto updateSizeText = [sizeText, croppedSize]()
    {
        if (croppedSize->isValid())
        {
            sizeText->setText(tr("裁剪尺寸: %1 × %2").arg(croppedSize->width()).arg(croppedSize->height()));
        }
    };
    ExampleI18n::bind(sizeText, updateSizeText);
    connect(cropButton, &ShadButton::clicked, this, [this, cropper, previewLabel, croppedSize, updateSizeText]()
    {
        const QImage result = cropper->croppedImage();
        if (result.isNull())
        {
            return;
        }
        previewLabel->setPixmap(toPreviewPixmap(result, devicePixelRatioF()));
        *croppedSize = result.size();
        updateSizeText();
    });
    addRow(basicCard, {cropButton, previewLabel, sizeText});

    ShadCard* avatarCard = addSection([] { return tr("圆形头像"); }, [] { return tr("setShape(Circle)：固定 1:1，遮罩挖空为圆形，输出图片圆外透明"); });
    ShadImageCropper* avatarCropper = new ShadImageCropper(this);
    avatarCropper->setShape(ShadImageCropperType::Circle);
    avatarCropper->setImage(sample);
    avatarCropper->setFixedHeight(kCropperHeight);
    addStretchItem(avatarCard, avatarCropper);
    ShadAvatar* avatar = new ShadAvatar(this);
    avatar->setAvatarSize(ShadAvatarType::Large);
    ShadButton* avatarButton = new ShadButton(QString(), this);
    ExampleI18n::bind(avatarButton, [avatarButton]() { avatarButton->setText(tr("生成头像")); });
    connect(avatarButton, &ShadButton::clicked, this, [this, avatarCropper, avatar]()
    {
        const QImage result = avatarCropper->croppedImage();
        if (result.isNull())
        {
            return;
        }
        avatar->setImage(toPreviewPixmap(result, devicePixelRatioF()));
    });
    addRow(avatarCard, {avatarButton, avatar});

    ShadCard* fileCard = addSection([] { return tr("选择图片"); }, [] { return tr("从本地选择图片后进行裁剪"); });
    ShadImageCropper* fileCropper = new ShadImageCropper(this);
    fileCropper->setFixedHeight(kCropperHeight);
    addStretchItem(fileCard, fileCropper);
    ShadButton* chooseButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    chooseButton->setShadIcon(ShadIconType::FolderOpen);
    ExampleI18n::bind(chooseButton, [chooseButton]() { chooseButton->setText(tr("选择图片")); });
    ShadLabel* errorLabel = new ShadLabel(this);
    errorLabel->setColorRole(ShadThemeType::Destructive);
    errorLabel->setVisible(false);
    ExampleI18n::bind(errorLabel, [errorLabel]() { errorLabel->setText(tr("图片加载失败")); });
    const auto applyImage = [fileCropper, errorLabel](const QImage& image)
    {
        if (image.isNull())
        {
            errorLabel->setVisible(true);
            return;
        }
        errorLabel->setVisible(false);
        fileCropper->setImage(image);
    };
    connect(chooseButton, &ShadButton::clicked, this, [this, applyImage]()
    {
        const QString filter = tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)");
#ifdef Q_OS_WASM
        const QPointer<ImageCropperPage> guard(this);
        QFileDialog::getOpenFileContent(filter, [guard, applyImage](const QString& fileName, const QByteArray& content)
        {
            if (guard.isNull() || fileName.isEmpty())
            {
                return;
            }
            applyImage(QImage::fromData(content));
        });
        return;
#endif
        const QString path = QFileDialog::getOpenFileName(this, tr("选择图片"), QString(), filter);
        if (path.isEmpty())
        {
            return;
        }
        applyImage(QImage(path));
    });
    addRow(fileCard, {chooseButton, errorLabel});
}

ImageCropperPage::~ImageCropperPage()
{
}
