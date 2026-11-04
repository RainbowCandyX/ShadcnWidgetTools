#include "WatermarkPage.h"

#include <QDate>
#include <QImage>
#include <QObject>
#include <QPixmap>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadSlider.h"
#include "ShadTheme.h"
#include "ShadWatermark.h"

namespace
{
    constexpr int kHostHeight = 240;

    ShadCard* createHost(QWidget* parent)
    {
        ShadCard* host = new ShadCard(parent);
        host->setFixedHeight(kHostHeight);
        return host;
    }
}

WatermarkPage::WatermarkPage(QWidget* parent)
    : ExampleBasePage([] { return tr("水印"); }, [] { return tr("ShadWatermark：在目标组件上平铺文字或图片水印，不拦截鼠标事件。"); }, parent)
{
    const auto fillHost = [](ShadCard* host)
    {
        ShadLabel* title = new ShadLabel(host);
        ExampleI18n::bind(title, [title]() { title->setText(tr("季度经营报告")); });
        ShadLabel* body = new ShadLabel(host);
        body->setColorRole(ShadThemeType::MutedForeground);
        body->setWordWrap(true);
        ExampleI18n::bind(body, [body]() { body->setText(tr("本季度整体收入保持稳定增长，各业务线的成本控制良好，详细数据见附表。")); });
        host->contentLayout()->addWidget(title);
        host->contentLayout()->addWidget(body);
        host->contentLayout()->addStretch();
    };

    ShadCard* textCard = addSection([] { return tr("文字水印"); }, [] { return tr("水印覆盖在内容之上，奇数行交错排列"); });
    ShadCard* textHost = createHost(this);
    fillHost(textHost);
    ShadWatermark* textMark = new ShadWatermark(textHost);
    ExampleI18n::bind(textMark, [textMark]() { textMark->setText(tr("内部资料")); });
    addStretchItem(textCard, textHost);

    ShadCard* multiCard = addSection([] { return tr("多行文字"); }, [] { return tr("文字中的换行符会拆成多行"); });
    ShadCard* multiHost = createHost(this);
    fillHost(multiHost);
    ShadWatermark* multiMark = new ShadWatermark(multiHost);
    ExampleI18n::bind(multiMark, [multiMark]() { multiMark->setText(tr("ShadcnWidgetTools") + QLatin1Char('\n') + QDate::currentDate().toString(Qt::ISODate)); });
    addStretchItem(multiCard, multiHost);

    ShadCard* imageCard = addSection([] { return tr("图片水印"); }, [] { return tr("使用图标渲染的图片，透明度 0.15"); });
    ShadCard* imageHost = createHost(this);
    fillHost(imageHost);
    ShadWatermark* imageMark = new ShadWatermark(imageHost);
    imageMark->setOpacity(0.15);
    imageMark->setImageSize(QSize(40, 40));
    imageMark->setRotation(0.0);
    imageMark->setGap(QSize(60, 60));
    const auto updateImage = [imageMark]() { imageMark->setImage(ShadIcon::pixmap(ShadIconType::Package, shadTheme->getColor(ShadThemeType::Foreground), 40, 2.0).toImage()); };
    updateImage();
    connect(shadTheme, &ShadTheme::themeChanged, imageMark, updateImage);
    addStretchItem(imageCard, imageHost);

    ShadCard* customCard = addSection([] { return tr("自定义"); }, [] { return tr("拖动滑块实时调整旋转角度、间距和字号"); });
    ShadCard* customHost = createHost(this);
    fillHost(customHost);
    ShadWatermark* customMark = new ShadWatermark(customHost);
    ExampleI18n::bind(customMark, [customMark]() { customMark->setText(tr("ShadcnWidgetTools")); });
    addStretchItem(customCard, customHost);

    ShadLabel* rotationLabel = new ShadLabel(this);
    rotationLabel->setColorRole(ShadThemeType::MutedForeground);
    ShadSlider* rotationSlider = new ShadSlider(Qt::Horizontal, this);
    rotationSlider->setRange(-90, 90);
    rotationSlider->setValue(static_cast<int>(customMark->getRotation()));
    const auto updateRotationLabel = [rotationLabel, rotationSlider]() { rotationLabel->setText(tr("旋转角度: %1").arg(rotationSlider->value())); };
    ExampleI18n::bind(rotationLabel, updateRotationLabel);
    connect(rotationSlider, &ShadSlider::valueChanged, customMark, [customMark, updateRotationLabel](int value)
    {
        customMark->setRotation(value);
        updateRotationLabel();
    });
    addStretchItem(customCard, rotationLabel);
    addStretchItem(customCard, rotationSlider);

    ShadLabel* gapLabel = new ShadLabel(this);
    gapLabel->setColorRole(ShadThemeType::MutedForeground);
    ShadSlider* gapSlider = new ShadSlider(Qt::Horizontal, this);
    gapSlider->setRange(40, 200);
    gapSlider->setValue(customMark->getGap().width());
    const auto updateGapLabel = [gapLabel, gapSlider]() { gapLabel->setText(tr("间距: %1").arg(gapSlider->value())); };
    ExampleI18n::bind(gapLabel, updateGapLabel);
    connect(gapSlider, &ShadSlider::valueChanged, customMark, [customMark, updateGapLabel](int value)
    {
        customMark->setGap(QSize(value, value));
        customMark->setOffset(QPoint(value / 2, value / 2));
        updateGapLabel();
    });
    addStretchItem(customCard, gapLabel);
    addStretchItem(customCard, gapSlider);

    ShadLabel* fontLabel = new ShadLabel(this);
    fontLabel->setColorRole(ShadThemeType::MutedForeground);
    ShadSlider* fontSlider = new ShadSlider(Qt::Horizontal, this);
    fontSlider->setRange(12, 32);
    fontSlider->setValue(customMark->getFontPixelSize());
    const auto updateFontLabel = [fontLabel, fontSlider]() { fontLabel->setText(tr("字号: %1").arg(fontSlider->value())); };
    ExampleI18n::bind(fontLabel, updateFontLabel);
    connect(fontSlider, &ShadSlider::valueChanged, customMark, [customMark, updateFontLabel](int value)
    {
        customMark->setFontPixelSize(value);
        updateFontLabel();
    });
    addStretchItem(customCard, fontLabel);
    addStretchItem(customCard, fontSlider);
}

WatermarkPage::~WatermarkPage()
{
}
