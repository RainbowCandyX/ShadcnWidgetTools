#include "ThemePage.h"

#include <QGridLayout>
#include <QList>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ColorSwatch.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadTheme.h"
#include "ShadToggle.h"
#include "ShadToggleGroup.h"

namespace
{
    // 名称顺序与 ShadThemeType::BaseColor 一致
    const QStringList kBaseColorNames{QStringLiteral("neutral"), QStringLiteral("zinc"), QStringLiteral("slate"), QStringLiteral("stone"), QStringLiteral("gray")};
    // 当前 token 网格每行色块数
    constexpr int kTokenColumns = 5;
} // namespace

ThemePage::ThemePage(QWidget* parent)
    : ExampleBasePage([] { return tr("主题"); }, [] { return tr("ShadTheme：管理颜色 token、基色、明暗模式与圆角，通常在启动时选定基色。"); }, parent)
{
    ShadCard* baseColorCard = addSection([] { return tr("基色"); }, [] { return tr("切换基色后所有组件立即重绘"); });
    ShadToggleGroup* baseColorGroup = new ShadToggleGroup(ShadToggleGroupType::Single, this);
    baseColorGroup->setVariant(ShadToggleType::Outline);
    for (const QString& name : kBaseColorNames)
    {
        baseColorGroup->addItem(name);
    }
    baseColorGroup->setCurrentIndex(static_cast<int>(shadTheme->getBaseColor()));
    connect(baseColorGroup, &ShadToggleGroup::currentIndexChanged, this, [baseColorGroup](int index)
    {
        // 不允许取消选中：再次点击当前基色时恢复选中
        if (index < 0)
        {
            baseColorGroup->setCurrentIndex(static_cast<int>(shadTheme->getBaseColor()));
            return;
        }
        shadTheme->setBaseColor(static_cast<ShadThemeType::BaseColor>(index));
    });
    connect(shadTheme, &ShadTheme::baseColorChanged, baseColorGroup, [baseColorGroup](ShadThemeType::BaseColor baseColor) { baseColorGroup->setCurrentIndex(static_cast<int>(baseColor)); });
    addItem(baseColorCard, baseColorGroup);

    ShadCard* tokenCard = addSection([] { return tr("颜色 token"); }, [] { return tr("当前基色与明暗模式下的全部颜色"); });
    QWidget* tokenGrid = new QWidget(this);
    QGridLayout* tokenLayout = new QGridLayout(tokenGrid);
    tokenLayout->setContentsMargins(0, 0, 0, 0);
    tokenLayout->setHorizontalSpacing(16);
    tokenLayout->setVerticalSpacing(12);
    for (int i = 0; i < ShadThemeType::ColorCount; ++i)
    {
        tokenLayout->addWidget(new ColorSwatch(static_cast<ShadThemeType::ThemeColor>(i), false, tokenGrid), i / kTokenColumns, i % kTokenColumns);
    }
    addItem(tokenCard, tokenGrid);

    ShadCard* compareCard = addSection([] { return tr("基色对比"); }, [] { return tr("对比五种基色的主要 token，悬停查看色值"); });
    const QList<ShadThemeType::ThemeColor> compareTokens{ShadThemeType::Background, ShadThemeType::Foreground, ShadThemeType::Primary, ShadThemeType::Secondary, ShadThemeType::MutedForeground, ShadThemeType::Accent, ShadThemeType::Border, ShadThemeType::Ring};
    QWidget* compareGrid = new QWidget(this);
    QGridLayout* compareLayout = new QGridLayout(compareGrid);
    compareLayout->setContentsMargins(0, 0, 0, 0);
    compareLayout->setHorizontalSpacing(8);
    compareLayout->setVerticalSpacing(8);
    for (int row = 0; row < kBaseColorNames.size(); ++row)
    {
        ShadLabel* nameLabel = new ShadLabel(kBaseColorNames.at(row), compareGrid);
        nameLabel->setFixedWidth(64);
        compareLayout->addWidget(nameLabel, row, 0);
        for (int column = 0; column < compareTokens.size(); ++column)
        {
            ColorSwatch* swatch = new ColorSwatch(compareTokens.at(column), true, compareGrid);
            swatch->setPresetBaseColor(static_cast<ShadThemeType::BaseColor>(row));
            compareLayout->addWidget(swatch, row, column + 1);
        }
    }
    addItem(compareCard, compareGrid);
}

ThemePage::~ThemePage()
{
}
