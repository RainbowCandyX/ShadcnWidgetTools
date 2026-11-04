#include "ShadColorPicker.h"

#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QList>
#include <QObject>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <initializer_list>

#include "ShadButton.h"
#include "ShadColorArea.h"
#include "ShadColorPickerPrivate.h"
#include "ShadColorPreview.h"
#include "ShadColorSwatches.h"
#include "ShadComboBox.h"
#include "ShadHueSlider.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadScreenColorPicker.h"
#include "ShadSpinBox.h"
#include "ShadStyleHelper.h"

namespace
{
    // 左右两列的可视间距
    constexpr int kColumnGap = 24;
    // 取色方块与色相滑条的可视间距、右列各控件的可视间距
    constexpr int kItemGap = 12;
    // 右列可视宽度
    constexpr int kRightColumnWidth = 200;
    // 数值输入框标签的宽度与可视间距
    constexpr int kSpinBoxLabelWidth = 16;
    constexpr int kSpinBoxLabelGap = 8;
    // 区块之间、区块标题与色块之间的间距
    constexpr int kSectionGap = 16;
    constexpr int kSectionTitleGap = 8;
    // 取色方块在左列中居中后，可视边缘距左列边缘 4px；色块圆形距格子边缘 4px，标题按同样的距离缩进
    constexpr int kContentInset = 4;
    // 色相滑条轨道上方的空白：(滑块 16 + ring 4 * 2 - 轨道 12) / 2
    constexpr int kHueTrackInset = 6;
    constexpr int kSwatchColumns = 12;

    ShadLabel* createSectionTitle(const QString& text, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(text, parent);
        ShadStyleHelper::applyFont(label, 14, QFont::Medium);
        label->setContentsMargins(kContentInset, 0, 0, 0);
        return label;
    }

    ShadButton* createIconButton(ShadIconType::IconName icon, const QString& toolTip, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Ghost, parent);
        button->setButtonSize(ShadButtonType::IconSmall);
        button->setShadIcon(icon);
        button->setToolTip(toolTip);
        return button;
    }

    // 内部组件的提示与分区标题，构造时与切换语言时设置
    void applyTexts(ShadColorPickerPrivate* d)
    {
        d->_preview->setToolTip(ShadColorPicker::tr("点击下半部分恢复原始颜色"));
        d->_screenPickButton->setToolTip(ShadColorPicker::tr("从屏幕取色"));
        d->_presetTitle->setText(ShadColorPicker::tr("基本颜色"));
        d->_addCustomButton->setToolTip(ShadColorPicker::tr("添加到自定义颜色"));
        d->_removeCustomButton->setToolTip(ShadColorPicker::tr("移除选中的自定义颜色"));
        d->_customTitle->setText(ShadColorPicker::tr("自定义颜色"));
    }
} // namespace

ShadColorPicker::ShadColorPicker(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadColorPickerPrivate())
{
    Q_D(ShadColorPicker);
    d->q_ptr = this;
    using Source = ShadColorPickerPrivate::Source;
    const int ring = ShadStyleHelper::kFocusRingWidth;
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);

    // 左列：饱和度与明度方块、色相滑条
    d->_area = new ShadColorArea(this);
    d->_hueSlider = new ShadHueSlider(this);
    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(kItemGap - ring - kHueTrackInset);
    leftLayout->addWidget(d->_area, 0, Qt::AlignHCenter);
    leftLayout->addWidget(d->_hueSlider, 0, Qt::AlignHCenter);
    leftLayout->addStretch(1);

    // 右列：预览条、十六进制输入、模式选择与数值输入框；各控件四周都有 3px 焦点环边距
    QWidget* rightColumn = new QWidget(this);
    rightColumn->setFixedWidth(kRightColumnWidth + 2 * ring);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightColumn);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(kItemGap - 2 * ring);
    d->_preview = new ShadColorPreview(rightColumn);
    d->_hexInput = new ShadInput(rightColumn);
    d->_hexInput->setMaxLength(7);
    d->_hexInput->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("#?[0-9A-Fa-f]{0,6}")), d->_hexInput));
    // 吸管按钮：outline icon，位于十六进制输入框的结束侧
    d->_screenPickButton = new ShadButton(QString(), ShadButtonType::Outline, rightColumn);
    d->_screenPickButton->setButtonSize(ShadButtonType::Icon);
    d->_screenPickButton->setShadIcon(ShadIconType::Pipette);
    d->_screenPickButton->setVisible(isScreenPickingSupported());
    QHBoxLayout* hexLayout = new QHBoxLayout();
    hexLayout->setContentsMargins(0, 0, 0, 0);
    // gap-2，扣除输入框与按钮相邻两侧的焦点环边距
    hexLayout->setSpacing(kSpinBoxLabelGap - 2 * ring);
    hexLayout->addWidget(d->_hexInput, 1);
    hexLayout->addWidget(d->_screenPickButton);
    d->_modeComboBox = new ShadComboBox(rightColumn);
    d->_modeComboBox->addItems({QStringLiteral("RGB"), QStringLiteral("HSV")});
    rightLayout->addWidget(d->_preview);
    rightLayout->addLayout(hexLayout);
    rightLayout->addWidget(d->_modeComboBox);
    for (int index = 0; index < ShadColorPickerPrivate::kSpinBoxCount; ++index)
    {
        ShadLabel* label = new ShadLabel(rightColumn);
        label->setColorRole(ShadThemeType::MutedForeground);
        // 左侧缩进 3px，与输入框的可视边缘对齐
        label->setContentsMargins(ring, 0, 0, 0);
        label->setFixedWidth(kSpinBoxLabelWidth + ring);
        ShadSpinBox* spinBox = new ShadSpinBox(rightColumn);
        QHBoxLayout* rowLayout = new QHBoxLayout();
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(kSpinBoxLabelGap - ring);
        rowLayout->addWidget(label);
        rowLayout->addWidget(spinBox, 1);
        rightLayout->addLayout(rowLayout);
        d->_spinBoxLabels[index] = label;
        d->_spinBoxes[index] = spinBox;
    }
    rightLayout->addStretch(1);

    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    // 扣除取色方块在左列中的 4px 与右列控件的 3px 焦点环边距
    topLayout->setSpacing(kColumnGap - kContentInset - ring);
    topLayout->addLayout(leftLayout);
    topLayout->addWidget(rightColumn);
    topLayout->addStretch(1);

    // 基本颜色
    d->_presetSwatches = new ShadColorSwatches(this);
    d->_presetSwatches->setColumns(kSwatchColumns);
    d->_presetSwatches->setColors(defaultPresetColors());
    QVBoxLayout* presetLayout = new QVBoxLayout();
    presetLayout->setContentsMargins(0, 0, 0, 0);
    presetLayout->setSpacing(kSectionTitleGap);
    d->_presetTitle = createSectionTitle(QString(), this);
    presetLayout->addWidget(d->_presetTitle);
    presetLayout->addWidget(d->_presetSwatches, 0, Qt::AlignLeft);

    // 自定义颜色：标题后为 + / − 按钮
    d->_addCustomButton = createIconButton(ShadIconType::Plus, QString(), this);
    d->_removeCustomButton = createIconButton(ShadIconType::Minus, QString(), this);
    d->_customSwatches = new ShadColorSwatches(this);
    d->_customSwatches->setColumns(kSwatchColumns);
    d->_customSwatches->setSelectable(true);
    d->_customSwatches->setColors(QList<QColor>(ShadColorPickerPrivate::kCustomSlotCount));
    QHBoxLayout* customHeaderLayout = new QHBoxLayout();
    customHeaderLayout->setContentsMargins(0, 0, 0, 0);
    customHeaderLayout->setSpacing(0);
    d->_customTitle = createSectionTitle(QString(), this);
    customHeaderLayout->addWidget(d->_customTitle);
    applyTexts(d);
    customHeaderLayout->addSpacing(kSectionTitleGap - ring);
    customHeaderLayout->addWidget(d->_addCustomButton);
    customHeaderLayout->addWidget(d->_removeCustomButton);
    customHeaderLayout->addStretch(1);
    QVBoxLayout* customLayout = new QVBoxLayout();
    customLayout->setContentsMargins(0, 0, 0, 0);
    customLayout->setSpacing(kSectionTitleGap - ring);
    customLayout->addLayout(customHeaderLayout);
    customLayout->addWidget(d->_customSwatches, 0, Qt::AlignLeft);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(kSectionGap);
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(presetLayout);
    mainLayout->addLayout(customLayout);

    connect(d->_area, &ShadColorArea::saturationValueChanged, this, [d](qreal saturation, qreal value) { d->applyHsv(d->_hue, saturation, value, Source::Area); });
    connect(d->_hueSlider, &ShadHueSlider::hueChanged, this, [d](qreal hue) { d->applyHsv(hue, d->_saturation, d->_value, Source::Hue); });
    connect(d->_preview, &ShadColorPreview::originalClicked, this, [d]() { d->applyColor(d->_originalColor, Source::None); });
    connect(d->_hexInput, &ShadInput::textEdited, this, [d](const QString& text) { d->onHexEdited(text); });
    connect(d->_hexInput, &ShadInput::editingFinished, this, [d]() { d->onHexEditingFinished(); });
    connect(d->_screenPickButton, &ShadButton::clicked, this, [this]() { pickScreenColor(); });
    connect(d->_modeComboBox, &ShadComboBox::currentIndexChanged, this, [this](int index) { setMode(index == 1 ? ShadColorPickerType::Hsv : ShadColorPickerType::Rgb); });
    for (ShadSpinBox* spinBox : d->_spinBoxes)
    {
        connect(spinBox, &QSpinBox::valueChanged, this, [d]() { d->onSpinBoxValueChanged(); });
    }
    connect(d->_presetSwatches, &ShadColorSwatches::swatchActivated, this, [d](int index) { d->applyColor(d->_presetSwatches->colorAt(index), Source::None); });
    connect(d->_customSwatches, &ShadColorSwatches::swatchActivated, this, [d](int index) { d->applyColor(d->_customSwatches->colorAt(index), Source::None); });
    connect(d->_customSwatches, &ShadColorSwatches::selectedIndexChanged, this, [d]() { d->updateCustomButtons(); });
    connect(d->_addCustomButton, &ShadButton::clicked, this, [d]() { d->addCustomColor(); });
    connect(d->_removeCustomButton, &ShadButton::clicked, this, [d]() { d->removeCustomColor(); });

    d->updateSpinBoxRanges();
    d->updateCustomButtons();
    d->applyColor(QColor(Qt::white), Source::None);
}

ShadColorPicker::~ShadColorPicker()
{
    Q_D(ShadColorPicker);
    // 私有数据先于子组件释放，断开子组件到本组件的连接，避免子组件析构期间回调访问已释放的数据
    const QList<QObject*> senders{d->_area, d->_hueSlider, d->_preview, d->_hexInput, d->_screenPickButton, d->_modeComboBox, d->_presetSwatches, d->_customSwatches, d->_addCustomButton, d->_removeCustomButton};
    for (QObject* sender : senders)
    {
        QObject::disconnect(sender, nullptr, this, nullptr);
    }
    if (d->_screenPicker != nullptr)
    {
        QObject::disconnect(d->_screenPicker, nullptr, this, nullptr);
    }
    for (ShadSpinBox* spinBox : d->_spinBoxes)
    {
        QObject::disconnect(spinBox, nullptr, this, nullptr);
    }
}

void ShadColorPicker::setColor(const QColor& color)
{
    Q_D(ShadColorPicker);
    d->applyColor(color, ShadColorPickerPrivate::Source::None);
}

QColor ShadColorPicker::getColor() const
{
    Q_D(const ShadColorPicker);
    return d->currentColor();
}

void ShadColorPicker::setOriginalColor(const QColor& color)
{
    Q_D(ShadColorPicker);
    d->_originalColor = color.isValid() ? color.toRgb() : QColor();
    d->_preview->setOriginalColor(d->_originalColor);
}

QColor ShadColorPicker::getOriginalColor() const
{
    Q_D(const ShadColorPicker);
    return d->_originalColor;
}

void ShadColorPicker::setMode(ShadColorPickerType::Mode mode)
{
    Q_D(ShadColorPicker);
    if (d->_mode == mode)
    {
        return;
    }
    d->_mode = mode;
    {
        const QSignalBlocker blocker(d->_modeComboBox);
        d->_modeComboBox->setCurrentIndex(mode == ShadColorPickerType::Hsv ? 1 : 0);
    }
    d->updateSpinBoxRanges();
}

ShadColorPickerType::Mode ShadColorPicker::getMode() const
{
    Q_D(const ShadColorPicker);
    return d->_mode;
}

void ShadColorPicker::setPresetColors(const QList<QColor>& colors)
{
    Q_D(ShadColorPicker);
    d->_presetSwatches->setColors(colors);
}

QList<QColor> ShadColorPicker::getPresetColors() const
{
    Q_D(const ShadColorPicker);
    return d->_presetSwatches->getColors();
}

QList<QColor> ShadColorPicker::defaultPresetColors()
{
    // 4 行 12 列，最后一行末尾为黑、灰、银、白
    return {
        QColor(0xF0, 0x87, 0x84), QColor(0xEB, 0x33, 0x24), QColor(0x77, 0x43, 0x42), QColor(0x8E, 0x40, 0x3A), QColor(0x3A, 0x06, 0x03), QColor(0x9F, 0xFC, 0xFD),
        QColor(0x73, 0xFB, 0xFD), QColor(0x32, 0x82, 0xF6), QColor(0x00, 0x23, 0xF5), QColor(0x00, 0x12, 0x9A), QColor(0x16, 0x41, 0x7C), QColor(0x00, 0x0C, 0x7B),
        QColor(0xFF, 0xFE, 0x91), QColor(0xFF, 0xFD, 0x55), QColor(0xF0, 0x9B, 0x59), QColor(0xF0, 0x86, 0x50), QColor(0x78, 0x43, 0x15), QColor(0x81, 0x7F, 0x26),
        QColor(0x7E, 0x84, 0xF7), QColor(0x73, 0x2B, 0xF5), QColor(0x35, 0x80, 0xBB), QColor(0x00, 0x02, 0x3D), QColor(0x58, 0x13, 0x5E), QColor(0x3A, 0x08, 0x3E),
        QColor(0xA1, 0xFB, 0x8E), QColor(0xA1, 0xFA, 0x4F), QColor(0x75, 0xF9, 0x4D), QColor(0x75, 0xFA, 0x61), QColor(0x75, 0xFA, 0x8D), QColor(0x81, 0x80, 0x49),
        QColor(0xEF, 0x88, 0xBE), QColor(0xEE, 0x8A, 0xF8), QColor(0xEA, 0x3F, 0xF7), QColor(0xEA, 0x36, 0x80), QColor(0x7F, 0x82, 0xBB), QColor(0x75, 0x16, 0x3F),
        QColor(0x37, 0x7D, 0x22), QColor(0x37, 0x7E, 0x47), QColor(0x36, 0x7E, 0x7F), QColor(0x50, 0x7F, 0x80), QColor(0x18, 0x3E, 0x0C), QColor(0x17, 0x3F, 0x3F),
        QColor(0x74, 0x1B, 0x7C), QColor(0x39, 0x10, 0x7B), QColor(0x00, 0x00, 0x00), QColor(0x80, 0x80, 0x80), QColor(0xC0, 0xC0, 0xC0), QColor(0xFF, 0xFF, 0xFF),
    };
}

void ShadColorPicker::setCustomColors(const QList<QColor>& colors)
{
    Q_D(ShadColorPicker);
    QList<QColor> slotColors = colors.mid(0, ShadColorPickerPrivate::kCustomSlotCount);
    while (slotColors.size() < ShadColorPickerPrivate::kCustomSlotCount)
    {
        slotColors.append(QColor());
    }
    d->_customSwatches->setColors(slotColors);
    d->_customSwatches->setSelectedIndex(-1);
    d->updateCustomButtons();
    Q_EMIT customColorsChanged();
}

QList<QColor> ShadColorPicker::getCustomColors() const
{
    Q_D(const ShadColorPicker);
    return d->_customSwatches->getColors();
}

void ShadColorPicker::setScreenPickingEnabled(bool enabled)
{
    Q_D(ShadColorPicker);
    d->_isScreenPickingEnabled = enabled;
    const bool isVisible = enabled && isScreenPickingSupported();
    if (!isVisible)
    {
        // 隐藏有焦点的按钮前清除焦点，避免焦点跳到下一个组件
        ShadStyleHelper::clearFocusWithin(d->_screenPickButton);
    }
    d->_screenPickButton->setVisible(isVisible);
}

bool ShadColorPicker::isScreenPickingEnabled() const
{
    Q_D(const ShadColorPicker);
    return d->_isScreenPickingEnabled;
}

void ShadColorPicker::setOwnWindowsVisibleWhilePicking(bool visible)
{
    Q_D(ShadColorPicker);
    d->_isOwnWindowsVisibleWhilePicking = visible;
}

bool ShadColorPicker::isOwnWindowsVisibleWhilePicking() const
{
    Q_D(const ShadColorPicker);
    return d->_isOwnWindowsVisibleWhilePicking;
}

void ShadColorPicker::changeEvent(QEvent* event)
{
    Q_D(ShadColorPicker);
    if (event->type() == QEvent::LanguageChange)
    {
        applyTexts(d);
    }
    QWidget::changeEvent(event);
}

bool ShadColorPicker::isScreenPickingSupported()
{
    return ShadScreenColorPicker::isSupported();
}

void ShadColorPicker::pickScreenColor()
{
    Q_D(ShadColorPicker);
    if (d->_screenPicker == nullptr)
    {
        d->_screenPicker = new ShadScreenColorPicker(this);
        connect(d->_screenPicker, &ShadScreenColorPicker::colorPicked, this, [this, d](const QColor& color)
        {
            d->applyColor(color, ShadColorPickerPrivate::Source::None);
            Q_EMIT screenPickingFinished(true);
        });
        connect(d->_screenPicker, &ShadScreenColorPicker::canceled, this, [this]() { Q_EMIT screenPickingFinished(false); });
    }
    if (d->_screenPicker->isActive())
    {
        return;
    }
    d->_screenPicker->setOwnWindowsVisible(d->_isOwnWindowsVisibleWhilePicking);
    if (!d->_screenPicker->start())
    {
        Q_EMIT screenPickingFinished(false);
    }
}
