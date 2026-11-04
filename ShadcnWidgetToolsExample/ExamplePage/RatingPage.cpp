#include "RatingPage.h"

#include <QColor>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadRating.h"

namespace
{
    ShadLabel* createMutedText(const QString& text, QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(text, parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }
} // namespace

RatingPage::RatingPage(QWidget* parent)
    : ExampleBasePage([] { return tr("评分"); }, [] { return tr("ShadRating：星级评分，悬停时预览，点击设置评分，支持只读与自定义外观。"); }, parent)
{
    // 基础用法：显示当前评分
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("悬停预览，点击设置评分"); });
    ShadRating* basicRating = new ShadRating(this);
    basicRating->setRating(3);
    ShadLabel* basicLabel = createMutedText(QString(), this);
    const auto updateBasicLabel = [basicLabel, basicRating]() { basicLabel->setText(tr("当前评分：%1 / 5").arg(basicRating->getRating())); };
    connect(basicRating, &ShadRating::ratingChanged, basicLabel, updateBasicLabel);
    ExampleI18n::bind(basicLabel, updateBasicLabel);
    addRow(basicCard, {basicRating, basicLabel});

    // 只读
    ShadCard* readOnlyCard = addSection([] { return tr("只读与禁用"); }, [] { return tr("只读时仅显示评分，禁用时整体半透明"); });
    ShadRating* readOnlyRating = new ShadRating(this);
    readOnlyRating->setRating(4);
    readOnlyRating->setReadOnly(true);
    ShadRating* disabledRating = new ShadRating(this);
    disabledRating->setRating(2);
    disabledRating->setEnabled(false);
    addRow(readOnlyCard, {readOnlyRating, disabledRating});

    // 数量与尺寸
    ShadCard* sizeCard = addSection([] { return tr("数量与尺寸"); }, [] { return tr("设置星星的数量、大小与间距"); });
    ShadRating* smallRating = new ShadRating(this);
    smallRating->setStarSize(16);
    smallRating->setSpacing(2);
    smallRating->setRating(3);
    ShadRating* tenRating = new ShadRating(this);
    tenRating->setMaximum(10);
    tenRating->setRating(7);
    ShadRating* largeRating = new ShadRating(this);
    largeRating->setStarSize(32);
    largeRating->setSpacing(6);
    largeRating->setMaximum(3);
    largeRating->setRating(2);
    addItem(sizeCard, smallRating);
    addItem(sizeCard, tenRating);
    addItem(sizeCard, largeRating);

    // 自定义颜色
    ShadCard* colorCard = addSection([] { return tr("自定义颜色"); }, [] { return tr("setActiveColor：修改选中星星的颜色"); });
    ShadRating* amberRating = new ShadRating(this);
    amberRating->setActiveColor(QColor(0xfb, 0xbf, 0x24));
    amberRating->setRating(4);
    ShadRating* redRating = new ShadRating(this);
    redRating->setActiveColor(QColor(0xef, 0x44, 0x44));
    redRating->setRating(5);
    addRow(colorCard, {amberRating, redRating});

    // 键盘与 RTL
    ShadCard* keyboardCard = addSection([] { return tr("键盘与 RTL"); }, [] { return tr("键盘调整评分，以及从右到左布局"); });
    ShadRating* keyboardRating = new ShadRating(this);
    keyboardRating->setRating(2);
    ShadRating* rtlRating = new ShadRating(this);
    rtlRating->setLayoutDirection(Qt::RightToLeft);
    rtlRating->setRating(2);
    addRow(keyboardCard, {keyboardRating, rtlRating});
}

RatingPage::~RatingPage()
{
}
