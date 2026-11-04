#include "SignaturePadPage.h"

#include <QColor>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadLabel.h"
#include "ShadSignaturePad.h"
#include "ShadSlider.h"
#include "ShadTheme.h"
#include "ShadToggle.h"
#include "ShadToggleGroup.h"

SignaturePadPage::SignaturePadPage(QWidget* parent)
    : ExampleBasePage([] { return tr("签名板"); }, [] { return tr("ShadSignaturePad：手写签名板，线宽随速度变化，支持数位板压感、撤销、清空与导出图片。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("用鼠标或数位板签名，聚焦后 Ctrl+Z 撤销，Delete 清空"); });
    ShadSignaturePad* pad = new ShadSignaturePad(this);
    pad->setFixedHeight(220);
    addStretchItem(basicCard, pad);

    ShadButton* undoButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadButton* clearButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ShadButton* exportButton = new ShadButton(QString(), ShadButtonType::Default, this);
    ExampleI18n::bind(undoButton, [undoButton]() { undoButton->setText(tr("撤销")); });
    ExampleI18n::bind(clearButton, [clearButton]() { clearButton->setText(tr("清空")); });
    ExampleI18n::bind(exportButton, [exportButton]() { exportButton->setText(tr("导出")); });
    exportButton->setEnabled(false);
    addRow(basicCard, {undoButton, clearButton, exportButton});

    QLabel* previewLabel = new QLabel(this);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->hide();
    const auto updatePreviewStyle = [previewLabel]() { previewLabel->setStyleSheet(QStringLiteral("QLabel { border: 1px solid %1; border-radius: 6px; padding: 8px; }").arg(shadTheme->getColor(ShadThemeType::Border).name())); };
    updatePreviewStyle();
    connect(shadTheme, &ShadTheme::themeChanged, previewLabel, updatePreviewStyle);
    addItem(basicCard, previewLabel);

    connect(undoButton, &ShadButton::clicked, pad, &ShadSignaturePad::undo);
    connect(clearButton, &ShadButton::clicked, pad, &ShadSignaturePad::clear);
    connect(pad, &ShadSignaturePad::changed, exportButton, [pad, exportButton]() { exportButton->setEnabled(!pad->isEmpty()); });
    connect(exportButton, &ShadButton::clicked, previewLabel, [pad, previewLabel]()
    {
        const QImage image = pad->toImage();
        if (image.isNull())
        {
            previewLabel->hide();
            return;
        }
        previewLabel->setPixmap(QPixmap::fromImage(image));
        previewLabel->show();
    });

    ShadCard* styleCard = addSection([] { return tr("笔迹样式"); }, [] { return tr("切换笔迹颜色，拖动滑块调整最大线宽"); });
    ShadSignaturePad* stylePad = new ShadSignaturePad(this);
    stylePad->setFixedHeight(160);
    addStretchItem(styleCard, stylePad);

    ShadToggleGroup* colorGroup = new ShadToggleGroup(ShadToggleGroupType::Single, this);
    ShadToggle* defaultItem = colorGroup->addItem(QString());
    ShadToggle* blueItem = colorGroup->addItem(QString());
    ShadToggle* redItem = colorGroup->addItem(QString());
    ExampleI18n::bind(colorGroup, [defaultItem, blueItem, redItem]()
    {
        defaultItem->setText(tr("默认"));
        blueItem->setText(tr("蓝色"));
        redItem->setText(tr("红色"));
    });
    colorGroup->setVariant(ShadToggleType::Outline);
    colorGroup->setCurrentIndex(0);
    connect(colorGroup, &ShadToggleGroup::currentIndexChanged, stylePad, [stylePad](int index)
    {
        if (index == 1)
        {
            stylePad->setPenColor(QColor(QStringLiteral("#2563eb")));
        }
        else if (index == 2)
        {
            stylePad->setPenColor(QColor(QStringLiteral("#dc2626")));
        }
        else
        {
            stylePad->setPenColor(QColor());
        }
    });
    addItem(styleCard, colorGroup);

    ShadSlider* widthSlider = new ShadSlider(Qt::Horizontal, this);
    widthSlider->setRange(2, 8);
    widthSlider->setValue(4);
    widthSlider->setFixedWidth(ExampleLayout::kFieldWidth);
    stylePad->setMaxPenWidth(widthSlider->value());
    ShadLabel* widthLabel = new ShadLabel(this);
    widthLabel->setColorRole(ShadThemeType::MutedForeground);
    const auto updateWidthLabel = [widthSlider, widthLabel]() { widthLabel->setText(tr("最大线宽: %1").arg(widthSlider->value())); };
    ExampleI18n::bind(widthLabel, updateWidthLabel);
    connect(widthSlider, &ShadSlider::valueChanged, widthLabel, updateWidthLabel);
    connect(widthSlider, &ShadSlider::valueChanged, stylePad, [stylePad](int value) { stylePad->setMaxPenWidth(value); });
    addItem(styleCard, widthSlider);
    addItem(styleCard, widthLabel);

    ShadCard* stateCard = addSection([] { return tr("只读与禁用"); }, [] { return tr("只读和禁用的签名板都不响应输入"); });
    ShadSignaturePad* readOnlyPad = new ShadSignaturePad(this);
    readOnlyPad->setFixedHeight(120);
    readOnlyPad->setReadOnly(true);
    ExampleI18n::bind(readOnlyPad, [readOnlyPad]() { readOnlyPad->setPlaceholderText(tr("只读")); });
    ShadSignaturePad* disabledPad = new ShadSignaturePad(this);
    disabledPad->setFixedHeight(120);
    disabledPad->setEnabled(false);
    addStretchItem(stateCard, readOnlyPad);
    addStretchItem(stateCard, disabledPad);
}

SignaturePadPage::~SignaturePadPage()
{
}
