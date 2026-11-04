#include "ShadScreenshotToolbar.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QList>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSignalBlocker>
#include <QSize>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadLabel.h"
#include "ShadSeparator.h"
#include "ShadSlider.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadToggle.h"
#include "ShadToggleGroup.h"

namespace
{
    constexpr int kPadding = 2;
    constexpr int kRowSpacing = 2;
    constexpr int kSeparatorHeight = 16;
    constexpr int kSeparatorMargin = 4;
    constexpr int kSwatchSize = 24;
    constexpr qreal kSwatchDiameter = 14.0;
    constexpr qreal kSwatchRingWidth = 2.0;
    constexpr qreal kSwatchRingGap = 2.0;
    constexpr int kDefaultColorIndex = 0;
    constexpr int kSliderWidth = 96;
    constexpr int kSliderSpacing = 8;
    constexpr int kSizeLabelWidth = 40;
    constexpr int kSizeLabelPixelSize = 12;
    constexpr int kCaptionGap = 6;
    constexpr int kMinBlockSize = 4;
    constexpr int kMaxBlockSize = 40;

    const QList<ShadScreenshotAnnotation::Tool> kTools{ShadScreenshotAnnotation::Tool::Rectangle, ShadScreenshotAnnotation::Tool::Ellipse, ShadScreenshotAnnotation::Tool::Arrow, ShadScreenshotAnnotation::Tool::Pen, ShadScreenshotAnnotation::Tool::Mosaic, ShadScreenshotAnnotation::Tool::Text};
    const QList<ShadIconType::IconName> kToolIcons{ShadIconType::Square, ShadIconType::Circle, ShadIconType::MoveUpRight, ShadIconType::Pencil, ShadIconType::Grid3x3, ShadIconType::Type};

    class ColorSwatch : public QAbstractButton
    {
        public:
            ColorSwatch(const QColor& color, QWidget* parent)
                : QAbstractButton(parent), _color(color)
            {
                setCheckable(true);
                setFocusPolicy(Qt::NoFocus);
                setCursor(Qt::PointingHandCursor);
                setFixedSize(kSwatchSize, kSwatchSize);
            }

            QSize sizeHint() const override
            {
                return QSize(kSwatchSize, kSwatchSize);
            }

        protected:
            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);
                const QPointF center = QRectF(rect()).center();
                if (isChecked())
                {
                    const qreal ringRadius = kSwatchDiameter / 2.0 + kSwatchRingGap + kSwatchRingWidth / 2.0;
                    painter.setPen(QPen(shadTheme->getColor(ShadThemeType::Ring), kSwatchRingWidth));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawEllipse(center, ringRadius, ringRadius);
                }
                painter.setPen(QPen(shadTheme->getColor(ShadThemeType::Border), 1.0));
                painter.setBrush(_color);
                painter.drawEllipse(center, kSwatchDiameter / 2.0, kSwatchDiameter / 2.0);
            }

        private:
            QColor _color;
    };

    ShadLabel* createCaption(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        ShadStyleHelper::applyFont(label, kSizeLabelPixelSize, QFont::Normal);
        return label;
    }

    ShadLabel* createValueLabel(QWidget* parent)
    {
        ShadLabel* label = createCaption(parent);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setFixedWidth(kSizeLabelWidth);
        return label;
    }

    ShadSlider* createSlider(QWidget* parent)
    {
        ShadSlider* slider = new ShadSlider(Qt::Horizontal, parent);
        slider->setFocusPolicy(Qt::NoFocus);
        slider->setFixedWidth(kSliderWidth);
        return slider;
    }

    ShadSeparator* createSeparator(QWidget* parent)
    {
        ShadSeparator* separator = new ShadSeparator(Qt::Vertical, parent);
        separator->setFixedSize(2 * kSeparatorMargin + 1, kSeparatorHeight);
        return separator;
    }
}

ShadScreenshotToolbar::ShadScreenshotToolbar(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setCursor(Qt::ArrowCursor);
    _colors = {QColor(0xef, 0x44, 0x44), QColor(0xea, 0xb3, 0x08), QColor(0x22, 0xc5, 0x5e), QColor(0x3b, 0x82, 0xf6), QColor(Qt::white), QColor(0x0a, 0x0a, 0x0a)};

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(kPadding, kPadding, kPadding, kPadding);
    rootLayout->setSpacing(kRowSpacing);

    QHBoxLayout* mainRow = new QHBoxLayout();
    mainRow->setContentsMargins(0, 0, 0, 0);
    mainRow->setSpacing(0);
    _toolGroup = new ShadToggleGroup(ShadToggleGroupType::Single, this);
    _toolGroup->setToggleSize(ShadToggleType::Small);
    for (ShadIconType::IconName icon : kToolIcons)
    {
        _toolGroup->addItem(icon)->setFocusPolicy(Qt::NoFocus);
    }
    _undoButton = createButton(ShadIconType::Undo2, ShadButtonType::Ghost);
    _undoButton->setEnabled(false);
    _cancelButton = createButton(ShadIconType::X, ShadButtonType::Ghost);
    _saveButton = createButton(ShadIconType::Download, ShadButtonType::Ghost);
    _copyButton = createButton(ShadIconType::Copy, ShadButtonType::Ghost);
    _pinButton = createButton(ShadIconType::Pin, ShadButtonType::Ghost);
    _doneButton = createButton(ShadIconType::Check, ShadButtonType::Default);
    mainRow->addWidget(_toolGroup, 0, Qt::AlignVCenter);
    mainRow->addWidget(createSeparator(this), 0, Qt::AlignVCenter);
    mainRow->addWidget(_undoButton);
    mainRow->addWidget(createSeparator(this), 0, Qt::AlignVCenter);
    mainRow->addWidget(_cancelButton);
    mainRow->addWidget(_saveButton);
    mainRow->addWidget(_copyButton);
    mainRow->addWidget(_pinButton);
    mainRow->addWidget(_doneButton);
    rootLayout->addLayout(mainRow);

    _optionsRow = new QWidget(this);
    QHBoxLayout* optionsLayout = new QHBoxLayout(_optionsRow);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(0);
    _colorArea = new QWidget(_optionsRow);
    QHBoxLayout* colorLayout = new QHBoxLayout(_colorArea);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->setSpacing(0);
    _colorGroup = new QButtonGroup(this);
    _colorGroup->setExclusive(true);
    for (int i = 0; i < _colors.size(); ++i)
    {
        ColorSwatch* swatch = new ColorSwatch(_colors.at(i), _colorArea);
        _colorGroup->addButton(swatch, i);
        colorLayout->addWidget(swatch);
    }
    _colorGroup->button(kDefaultColorIndex)->setChecked(true);
    colorLayout->addWidget(createSeparator(_colorArea), 0, Qt::AlignVCenter);
    _sizeCaption = createCaption(_optionsRow);
    _sizeSlider = createSlider(_optionsRow);
    _sizeLabel = createValueLabel(_optionsRow);
    _blurArea = new QWidget(_optionsRow);
    QHBoxLayout* blurLayout = new QHBoxLayout(_blurArea);
    blurLayout->setContentsMargins(0, 0, 0, 0);
    blurLayout->setSpacing(0);
    _blurCaption = createCaption(_blurArea);
    _blurSlider = createSlider(_blurArea);
    _blurSlider->setRange(kMinBlockSize, kMaxBlockSize);
    _blurSlider->setValue(_mosaicBlockSize);
    _blurLabel = createValueLabel(_blurArea);
    _blurLabel->setText(QStringLiteral("%1 px").arg(_mosaicBlockSize));
    blurLayout->addWidget(createSeparator(_blurArea), 0, Qt::AlignVCenter);
    blurLayout->addWidget(_blurCaption, 0, Qt::AlignVCenter);
    blurLayout->addSpacing(kCaptionGap);
    blurLayout->addWidget(_blurSlider, 0, Qt::AlignVCenter);
    blurLayout->addWidget(_blurLabel, 0, Qt::AlignVCenter);
    optionsLayout->addWidget(_colorArea);
    optionsLayout->addSpacing(kSliderSpacing);
    optionsLayout->addWidget(_sizeCaption, 0, Qt::AlignVCenter);
    optionsLayout->addSpacing(kCaptionGap);
    optionsLayout->addWidget(_sizeSlider, 0, Qt::AlignVCenter);
    optionsLayout->addWidget(_sizeLabel, 0, Qt::AlignVCenter);
    optionsLayout->addWidget(_blurArea);
    optionsLayout->addSpacing(kSliderSpacing);
    optionsLayout->addStretch(1);
    _optionsRow->hide();
    rootLayout->addWidget(_optionsRow);

    connect(_toolGroup, &ShadToggleGroup::currentIndexChanged, this, [this](int index)
    {
        updateOptions();
        Q_EMIT toolChanged(index >= 0 && index < kTools.size() ? kTools.at(index) : ShadScreenshotAnnotation::Tool::None);
    });
    connect(_sizeSlider, &ShadSlider::valueChanged, this, [this](int value)
    {
        _sizes.insert(currentTool(), value);
        _sizeLabel->setText(QStringLiteral("%1 px").arg(value));
        Q_EMIT styleChanged();
    });
    connect(_blurSlider, &ShadSlider::valueChanged, this, [this](int value)
    {
        _mosaicBlockSize = value;
        _blurLabel->setText(QStringLiteral("%1 px").arg(value));
    });
    connect(_colorGroup, &QButtonGroup::idClicked, this, [this]() { Q_EMIT styleChanged(); });
    connect(_undoButton, &QAbstractButton::clicked, this, &ShadScreenshotToolbar::undoClicked);
    connect(_cancelButton, &QAbstractButton::clicked, this, &ShadScreenshotToolbar::cancelClicked);
    connect(_saveButton, &QAbstractButton::clicked, this, &ShadScreenshotToolbar::saveClicked);
    connect(_copyButton, &QAbstractButton::clicked, this, &ShadScreenshotToolbar::copyClicked);
    connect(_pinButton, &QAbstractButton::clicked, this, &ShadScreenshotToolbar::pinClicked);
    connect(_doneButton, &QAbstractButton::clicked, this, &ShadScreenshotToolbar::doneClicked);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    retranslate();
    resize(sizeHint());
}

ShadScreenshotToolbar::~ShadScreenshotToolbar()
{
}

void ShadScreenshotToolbar::setTool(ShadScreenshotAnnotation::Tool tool)
{
    _toolGroup->setCurrentIndex(kTools.indexOf(tool));
}

ShadScreenshotAnnotation::Tool ShadScreenshotToolbar::currentTool() const
{
    const int index = _toolGroup->getCurrentIndex();
    return index >= 0 && index < kTools.size() ? kTools.at(index) : ShadScreenshotAnnotation::Tool::None;
}

QColor ShadScreenshotToolbar::currentColor() const
{
    const int index = _colorGroup->checkedId();
    return index >= 0 && index < _colors.size() ? _colors.at(index) : _colors.at(kDefaultColorIndex);
}

int ShadScreenshotToolbar::currentMosaicBlockSize() const
{
    return _mosaicBlockSize;
}

int ShadScreenshotToolbar::currentSize() const
{
    const ShadScreenshotAnnotation::Tool tool = currentTool();
    return _sizes.value(tool, ShadScreenshotAnnotation::defaultSize(tool));
}

void ShadScreenshotToolbar::setUndoEnabled(bool enabled)
{
    _undoButton->setEnabled(enabled);
}

void ShadScreenshotToolbar::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        retranslate();
    }
    QWidget::changeEvent(event);
}

void ShadScreenshotToolbar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QRectF area = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    ShadStyleHelper::fillRoundedRect(&painter, area, radius, shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Popover));
    ShadStyleHelper::strokeRoundedRect(&painter, area, radius, shadTheme->getColor(ShadThemeType::Border));
}

ShadButton* ShadScreenshotToolbar::createButton(ShadIconType::IconName icon, ShadButtonType::Variant variant)
{
    ShadButton* button = new ShadButton(this);
    button->setVariant(variant);
    button->setButtonSize(ShadButtonType::IconSmall);
    button->setShadIcon(icon);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

void ShadScreenshotToolbar::updateOptions()
{
    const ShadScreenshotAnnotation::Tool tool = currentTool();
    _optionsRow->setVisible(tool != ShadScreenshotAnnotation::Tool::None);
    _colorArea->setVisible(tool != ShadScreenshotAnnotation::Tool::Mosaic);
    _blurArea->setVisible(tool == ShadScreenshotAnnotation::Tool::Mosaic);
    if (tool != ShadScreenshotAnnotation::Tool::None)
    {
        const int value = currentSize();
        const QSignalBlocker blocker(_sizeSlider);
        _sizeSlider->setRange(ShadScreenshotAnnotation::minimumSize(tool), ShadScreenshotAnnotation::maximumSize(tool));
        _sizeSlider->setValue(value);
        _sizeLabel->setText(QStringLiteral("%1 px").arg(value));
    }
    adjustSize();
    Q_EMIT layoutChanged();
}

void ShadScreenshotToolbar::retranslate()
{
    const QList<QString> toolNames{tr("矩形"), tr("椭圆"), tr("箭头"), tr("画笔"), tr("马赛克"), tr("文字")};
    for (int i = 0; i < toolNames.size() && i < _toolGroup->count(); ++i)
    {
        _toolGroup->itemAt(i)->setToolTip(toolNames.at(i));
    }
    _sizeCaption->setText(tr("粗细"));
    _blurCaption->setText(tr("模糊"));
    _undoButton->setToolTip(tr("撤销"));
    _cancelButton->setToolTip(tr("取消"));
    _saveButton->setToolTip(tr("保存"));
    _copyButton->setToolTip(tr("复制"));
    _pinButton->setToolTip(tr("钉在屏幕上"));
    _doneButton->setToolTip(tr("完成"));
}
