#include "PropertyGridPage.h"

#include <QColor>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QMetaType>
#include <QSlider>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadPropertyGrid.h"
#include "ShadSlider.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kGridHeight = 420;
    constexpr int kReadOnlyGridHeight = 180;

    QStringList alignmentOptions()
    {
        return {QCoreApplication::translate("PropertyGridPage", "左对齐"), QCoreApplication::translate("PropertyGridPage", "居中"), QCoreApplication::translate("PropertyGridPage", "右对齐")};
    }

    QString valueText(const QVariant& value)
    {
        if (value.typeId() == QMetaType::QColor)
        {
            return value.value<QColor>().name(QColor::HexRgb).toUpper();
        }
        return value.toString();
    }

    void updateChangeText(ShadLabel* label, const QString& change)
    {
        label->setText(change.isEmpty() ? QCoreApplication::translate("PropertyGridPage", "最近修改: 无") : QCoreApplication::translate("PropertyGridPage", "最近修改: %1").arg(change));
    }
} // namespace

PropertyGridPage::PropertyGridPage(QWidget* parent)
    : ExampleBasePage([] { return tr("属性面板"); }, [] { return tr("ShadPropertyGrid：分组编辑键值属性。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("拖动分界线调整列宽"); });
    ShadPropertyGrid* grid = new ShadPropertyGrid(this);
    grid->setFixedSize(ExampleLayout::kWideFieldWidth, kGridHeight);
    const QString appearance = QStringLiteral("appearance");
    const QString behavior = QStringLiteral("behavior");
    grid->addProperty(QStringLiteral("title"), QString(), QStringLiteral("Shadcn"), appearance);
    grid->addProperty(QStringLiteral("width"), QString(), 320, appearance);
    grid->setPropertyRange(QStringLiteral("width"), 0, 2000);
    grid->addProperty(QStringLiteral("opacity"), QString(), 0.8, appearance);
    grid->setPropertyRange(QStringLiteral("opacity"), 0.0, 1.0);
    QDoubleSpinBox* opacityEditor = qobject_cast<QDoubleSpinBox*>(grid->getEditor(QStringLiteral("opacity")));
    if (opacityEditor != nullptr)
    {
        opacityEditor->setSingleStep(0.1);
    }
    grid->addProperty(QStringLiteral("visible"), QString(), true, appearance);
    grid->addProperty(QStringLiteral("background"), QString(), QColor(QStringLiteral("#18181B")), appearance);
    grid->addEnumProperty(QStringLiteral("alignment"), QString(), alignmentOptions(), 0, appearance);
    grid->addProperty(QStringLiteral("enabled"), QString(), true, behavior);
    grid->addProperty(QStringLiteral("version"), QString(), QStringLiteral("1.0.0"), behavior);
    grid->setPropertyReadOnly(QStringLiteral("version"), true);
    ShadSlider* volumeSlider = new ShadSlider(Qt::Horizontal);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);
    grid->addWidgetProperty(QStringLiteral("volume"), QString(), volumeSlider, behavior);
    ExampleI18n::bind(grid, [grid, appearance, behavior]()
    {
        grid->setGroupTitle(appearance, tr("外观"));
        grid->setGroupTitle(behavior, tr("行为"));
        grid->setPropertyLabel(QStringLiteral("title"), tr("标题"));
        grid->setPropertyDescription(QStringLiteral("title"), tr("显示在顶部的文字。"));
        grid->setPropertyLabel(QStringLiteral("width"), tr("宽度"));
        grid->setPropertyDescription(QStringLiteral("width"), tr("单位为像素，范围 0 到 2000。"));
        grid->setPropertyLabel(QStringLiteral("opacity"), tr("透明度"));
        grid->setPropertyDescription(QStringLiteral("opacity"), tr("0 为全透明，1 为不透明。"));
        grid->setPropertyLabel(QStringLiteral("visible"), tr("可见"));
        grid->setPropertyLabel(QStringLiteral("background"), tr("背景色"));
        grid->setPropertyDescription(QStringLiteral("background"), tr("点击打开取色对话框。"));
        grid->setPropertyLabel(QStringLiteral("alignment"), tr("对齐方式"));
        grid->setEnumOptions(QStringLiteral("alignment"), alignmentOptions());
        grid->setPropertyLabel(QStringLiteral("enabled"), tr("启用"));
        grid->setPropertyLabel(QStringLiteral("version"), tr("版本"));
        grid->setPropertyDescription(QStringLiteral("version"), tr("只读属性。"));
        grid->setPropertyLabel(QStringLiteral("volume"), tr("音量"));
        grid->setPropertyDescription(QStringLiteral("volume"), tr("自定义编辑器。"));
    });

    ShadLabel* changeLabel = new ShadLabel(this);
    changeLabel->setColorRole(ShadThemeType::MutedForeground);
    const std::shared_ptr<QString> lastChange = std::make_shared<QString>();
    ExampleI18n::bind(changeLabel, [changeLabel, lastChange]() { updateChangeText(changeLabel, *lastChange); });
    connect(grid, &ShadPropertyGrid::valueChanged, changeLabel, [changeLabel, lastChange](const QString& key, const QVariant& value)
    {
        *lastChange = QStringLiteral("%1 = %2").arg(key, valueText(value));
        updateChangeText(changeLabel, *lastChange);
    });
    connect(volumeSlider, &QSlider::valueChanged, changeLabel, [changeLabel, lastChange](int value)
    {
        *lastChange = QStringLiteral("volume = %1").arg(value);
        updateChangeText(changeLabel, *lastChange);
    });
    addItem(basicCard, grid);
    addItem(basicCard, changeLabel);

    ShadCard* readOnlyCard = addSection([] { return tr("只读"); }, [] { return tr("不分组，隐藏说明区"); });
    ShadPropertyGrid* readOnlyGrid = new ShadPropertyGrid(this);
    readOnlyGrid->setFixedSize(ExampleLayout::kWideFieldWidth, kReadOnlyGridHeight);
    readOnlyGrid->setDescriptionVisible(false);
    readOnlyGrid->addProperty(QStringLiteral("name"), QString(), QStringLiteral("report.pdf"));
    readOnlyGrid->addProperty(QStringLiteral("size"), QString(), 2048);
    readOnlyGrid->addProperty(QStringLiteral("shared"), QString(), false);
    readOnlyGrid->addProperty(QStringLiteral("color"), QString(), QColor(QStringLiteral("#3B82F6")));
    for (const QString& key : readOnlyGrid->getKeys())
    {
        readOnlyGrid->setPropertyReadOnly(key, true);
    }
    ExampleI18n::bind(readOnlyGrid, [readOnlyGrid]()
    {
        readOnlyGrid->setPropertyLabel(QStringLiteral("name"), tr("文件名"));
        readOnlyGrid->setPropertyLabel(QStringLiteral("size"), tr("大小 (KB)"));
        readOnlyGrid->setPropertyLabel(QStringLiteral("shared"), tr("已共享"));
        readOnlyGrid->setPropertyLabel(QStringLiteral("color"), tr("标签颜色"));
    });
    ShadSwitch* readOnlySwitch = new ShadSwitch(this);
    readOnlySwitch->setChecked(true);
    ExampleI18n::bind(readOnlySwitch, [readOnlySwitch]() { readOnlySwitch->setText(tr("只读")); });
    connect(readOnlySwitch, &ShadSwitch::toggled, readOnlyGrid, [readOnlyGrid](bool checked)
    {
        for (const QString& key : readOnlyGrid->getKeys())
        {
            readOnlyGrid->setPropertyReadOnly(key, checked);
        }
    });
    addItem(readOnlyCard, readOnlySwitch);
    addItem(readOnlyCard, readOnlyGrid);
}

PropertyGridPage::~PropertyGridPage()
{
}
