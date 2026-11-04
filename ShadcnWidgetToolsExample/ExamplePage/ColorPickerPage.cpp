#include "ColorPickerPage.h"

#include <QColor>
#include <QList>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <initializer_list>
#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadColorDialog.h"
#include "ShadColorPicker.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadSwitch.h"
#include "ShadTheme.h"

namespace
{
    // 颜色块边长
    constexpr int kChipSize = 24;

    // 显示颜色的小方块：rounded-md，1px border
    class ColorChip : public QWidget
    {
        public:
            explicit ColorChip(const QColor& color, QWidget* parent = nullptr)
                : QWidget(parent), _color(color)
            {
                setFixedSize(kChipSize, kChipSize);
                connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
            }

            void setColor(const QColor& color)
            {
                _color = color;
                update();
            }

            QColor getColor() const
            {
                return _color;
            }

        protected:
            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setPen(shadTheme->getColor(ShadThemeType::Border));
                painter.setBrush(_color);
                painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
            }

        private:
            QColor _color;
    };

    ShadLabel* createMutedText(const QString& text, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(text, parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    // 例如 "#3B82F6 · rgb(59, 130, 246)"
    QString describeColor(const QColor& color)
    {
        return QStringLiteral("%1 · rgb(%2, %3, %4)").arg(color.name(QColor::HexRgb).toUpper()).arg(color.red()).arg(color.green()).arg(color.blue());
    }

    // Tailwind 色板的 500 色阶与几种中性色，两行各 12 个
    QList<QColor> tailwindColors()
    {
        return {
            QColor(0xEF, 0x44, 0x44), QColor(0xF9, 0x73, 0x16), QColor(0xF5, 0x9E, 0x0B), QColor(0xEA, 0xB3, 0x08), QColor(0x84, 0xCC, 0x16), QColor(0x22, 0xC5, 0x5E),
            QColor(0x10, 0xB9, 0x81), QColor(0x14, 0xB8, 0xA6), QColor(0x06, 0xB6, 0xD4), QColor(0x0E, 0xA5, 0xE9), QColor(0x3B, 0x82, 0xF6), QColor(0x63, 0x66, 0xF1),
            QColor(0x8B, 0x5C, 0xF6), QColor(0xA8, 0x55, 0xF7), QColor(0xD9, 0x46, 0xEF), QColor(0xEC, 0x48, 0x99), QColor(0xF4, 0x3F, 0x5E), QColor(0x64, 0x74, 0x8B),
            QColor(0x6B, 0x72, 0x80), QColor(0x71, 0x71, 0x7A), QColor(0x73, 0x73, 0x73), QColor(0x78, 0x71, 0x6C), QColor(0x0A, 0x0A, 0x0A), QColor(0xFA, 0xFA, 0xFA),
        };
    }
} // namespace

ColorPickerPage::ColorPickerPage(QWidget* parent)
    : ExampleBasePage([] { return tr("颜色选择器"); }, [] { return tr("ShadColorPicker / ShadColorDialog：取色面板与颜色对话框，支持屏幕取色。"); }, parent)
{
    // 作用于本页的面板与对话框，静态函数 getColor 内部自建面板，保持默认
    ShadCard* screenPickingCard = addSection([] { return tr("屏幕取色"); }, [] { return tr("隐藏本程序窗口，取窗口背后的颜色"); });
    ShadSwitch* hideWindowsSwitch = new ShadSwitch(this);
    ExampleI18n::bind(hideWindowsSwitch, [hideWindowsSwitch]() { hideWindowsSwitch->setText(tr("取色时隐藏")); });
    addItem(screenPickingCard, hideWindowsSwitch);

    // 内嵌面板
    ShadCard* inlineCard = addSection([] { return tr("内嵌面板"); }, [] { return tr("直接放进页面使用，吸管可从屏幕取色"); });
    ShadColorPicker* inlinePicker = new ShadColorPicker(this);
    inlinePicker->setColor(QColor(0x3B, 0x82, 0xF6));
    ShadLabel* inlineValue = createMutedText(QString(), this);
    // 记录最近一次屏幕取色是否取消，按当前状态生成文字，切换语言时重新生成
    std::shared_ptr<bool> isPickingCanceled = std::make_shared<bool>(false);
    const auto updateInlineValue = [inlinePicker, inlineValue, isPickingCanceled]() { inlineValue->setText(*isPickingCanceled ? tr("已取消屏幕取色") : describeColor(inlinePicker->getColor())); };
    ExampleI18n::bind(inlineValue, updateInlineValue);
    connect(inlinePicker, &ShadColorPicker::colorChanged, inlineValue, [isPickingCanceled, updateInlineValue]()
    {
        *isPickingCanceled = false;
        updateInlineValue();
    });
    // 屏幕取色取消时提示，取到颜色时由 colorChanged 更新
    connect(inlinePicker, &ShadColorPicker::screenPickingFinished, inlineValue, [isPickingCanceled, updateInlineValue](bool picked)
    {
        *isPickingCanceled = !picked;
        updateInlineValue();
    });
    addItem(inlineCard, inlinePicker);
    addItem(inlineCard, inlineValue);

    // 对话框
    ShadCard* dialogCard = addSection([] { return tr("对话框"); }, [] { return tr("在对话框中选色，可恢复原始颜色"); });
    ShadButton* dialogButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(dialogButton, [dialogButton]() { dialogButton->setText(tr("选择主题色")); });
    const QColor themeColor(0xF9, 0x73, 0x16);
    ColorChip* dialogChip = new ColorChip(themeColor, this);
    ShadLabel* dialogValue = createMutedText(describeColor(themeColor), this);
    // 每次点击新建对话框，以色块当前颜色为初始颜色
    connect(dialogButton, &ShadButton::clicked, this, [this, dialogChip, dialogValue, hideWindowsSwitch]()
    {
        ShadColorDialog* colorDialog = new ShadColorDialog(dialogChip->getColor(), this);
        // 对话框打开期间切换语言时标题随之刷新
        ExampleI18n::bind(colorDialog, [colorDialog]() { colorDialog->setTitle(tr("选择主题色")); });
        colorDialog->colorPicker()->setOwnWindowsVisibleWhilePicking(!hideWindowsSwitch->isChecked());
        ExampleLayout::addStayOnTopButton(colorDialog);
        connect(colorDialog, &ShadColorDialog::colorSelected, dialogChip, [dialogChip, dialogValue](const QColor& color)
        {
            dialogChip->setColor(color);
            dialogValue->setText(describeColor(color));
        });
        ExampleLayout::openTransientDialog(colorDialog);
    });
    addRow(dialogCard, {dialogButton, dialogChip, dialogValue});

    // 静态函数
    ShadCard* staticCard = addSection([] { return tr("静态函数"); }, [] { return tr("ShadColorDialog::getColor：阻塞打开并返回所选颜色"); });
    ShadButton* staticButton = new ShadButton(QStringLiteral("getColor"), ShadButtonType::Outline, this);
    ColorChip* staticChip = new ColorChip(QColor(0x22, 0xC5, 0x5E), this);
    ShadLabel* staticValue = createMutedText(QString(), this);
    // 保存当前状态的文字生成函数：尚未选择、已取消或所选颜色，切换语言时重新生成
    std::shared_ptr<ExampleI18n::TextFunction> staticText = std::make_shared<ExampleI18n::TextFunction>([] { return tr("尚未选择"); });
    ExampleI18n::bind(staticValue, [staticValue, staticText]() { staticValue->setText((*staticText)()); });
    connect(staticButton, &ShadButton::clicked, this, [this, staticChip, staticValue, staticText]()
    {
        // 阻塞打开的对话框标题只在打开时取一次
        const QColor color = ShadColorDialog::getColor(QColor(0x22, 0xC5, 0x5E), this, tr("选择标签颜色"));
        if (!color.isValid())
        {
            *staticText = [] { return tr("已取消"); };
        }
        else
        {
            staticChip->setColor(color);
            *staticText = [color] { return describeColor(color); };
        }
        staticValue->setText((*staticText)());
    });
    addRow(staticCard, {staticButton, staticChip, staticValue});

    // 自定义预设
    ShadCard* presetCard = addSection([] { return tr("自定义预设"); }, [] { return tr("替换基本颜色，切换 HSV 模式，预填自定义颜色"); });
    ShadColorPicker* presetPicker = new ShadColorPicker(this);
    presetPicker->setPresetColors(tailwindColors());
    presetPicker->setMode(ShadColorPickerType::Hsv);
    presetPicker->setCustomColors({QColor(0x0A, 0x0A, 0x0A), QColor(0x73, 0x73, 0x73), QColor(0xE5, 0xE5, 0xE5)});
    presetPicker->setColor(QColor(0x8B, 0x5C, 0xF6));
    addItem(presetCard, presetPicker);

    // RTL
    ShadCard* rtlCard = addSection([] { return QStringLiteral("RTL"); }, [] { return tr("从右到左布局下面板与色块网格镜像"); });
    ShadColorPicker* rtlPicker = new ShadColorPicker(this);
    rtlPicker->setLayoutDirection(Qt::RightToLeft);
    rtlPicker->setColor(QColor(0x14, 0xB8, 0xA6));
    addItem(rtlCard, rtlPicker);

    for (ShadColorPicker* picker : {inlinePicker, presetPicker, rtlPicker})
    {
        connect(hideWindowsSwitch, &ShadSwitch::toggled, picker, [picker](bool isHidden) { picker->setOwnWindowsVisibleWhilePicking(!isHidden); });
    }
}

ColorPickerPage::~ColorPickerPage()
{
}
