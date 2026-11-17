#include "BackTopPage.h"

#include <QFont>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadBackTop.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadScrollArea.h"
#include "ShadSkeleton.h"

namespace
{
    constexpr int kListHeight = 280;
    constexpr int kRowCount = 30;
    constexpr int kSkeletonHeight = 16;
    constexpr int kPageSkeletonCount = 24;
    constexpr int kListVisibilityHeight = 200;
    constexpr int kListOffset = 16;

    ShadLabel* createMutedLabel(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setWordWrap(true);
        label->setColorRole(ShadThemeType::MutedForeground);
        QFont font = label->font();
        font.setWeight(QFont::Normal);
        label->setFont(font);
        return label;
    }
} // namespace

BackTopPage::BackTopPage(QWidget* parent)
    : ExampleBasePage([] { return tr("回到顶部"); }, [] { return tr("ShadBackTop：滚动一段距离后在右下角出现的圆形按钮，点击平滑滚动到顶部。"); }, parent)
{
    ShadCard* pageCard = addSection([] { return tr("基础用法"); }, [] { return tr("ShadBackTop(this) 绑定整个示例页：向下滚动 400px 后右下角出现按钮"); });
    ShadLabel* pageLabel = createMutedLabel(this);
    ExampleI18n::bind(pageLabel, [pageLabel]() { pageLabel->setText(tr("继续向下滚动页面，按钮出现后点击回到顶部；动画期间手动滚动会中断动画。")); });
    addStretchItem(pageCard, pageLabel);
    new ShadBackTop(this);

    ShadCard* listCard = addSection([] { return tr("滚动区域"); }, [] { return tr("绑定任意 QAbstractScrollArea；setVisibilityHeight(200) 与 setOffset(16) 调整出现时机与位置"); });
    ShadScrollArea* listArea = new ShadScrollArea(this);
    listArea->setFixedHeight(kListHeight);
    QWidget* listContent = new QWidget();
    QVBoxLayout* listLayout = new QVBoxLayout(listContent);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(12);
    for (int row = 1; row <= kRowCount; ++row)
    {
        ShadLabel* rowLabel = createMutedLabel(listContent);
        ExampleI18n::bind(rowLabel, [rowLabel, row]() { rowLabel->setText(tr("第 %1 行").arg(row)); });
        listLayout->addWidget(rowLabel);
    }
    listArea->setWidget(listContent);
    ShadBackTop* listBackTop = new ShadBackTop(listArea);
    listBackTop->setVisibilityHeight(kListVisibilityHeight);
    listBackTop->setOffset(kListOffset);
    addStretchItem(listCard, listArea);

    ShadCard* fillerCard = addSection([] { return tr("占位内容"); }, [] { return tr("用于撑开页面高度"); });
    for (int index = 0; index < kPageSkeletonCount; ++index)
    {
        ShadSkeleton* skeleton = new ShadSkeleton(this);
        skeleton->setFixedHeight(kSkeletonHeight);
        addStretchItem(fillerCard, skeleton);
    }
}

BackTopPage::~BackTopPage()
{
}
