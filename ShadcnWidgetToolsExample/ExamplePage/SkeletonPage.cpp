#include "SkeletonPage.h"

#include <QHBoxLayout>
#include <QList>
#include <QSize>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadSkeleton.h"
#include "ShadSwitch.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kAvatarSize = 48;
    constexpr int kLineHeight = 16;
    constexpr int kLongLineWidth = 250;
    constexpr int kShortLineWidth = 200;
    constexpr int kImageHeight = 125;
    constexpr int kAvatarGap = 16;
    constexpr int kLineGap = 8;
    constexpr int kImageGap = 12;

    QWidget* createLines(QWidget* parent)
    {
        QWidget* lines = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(lines);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kLineGap);
        layout->addWidget(new ShadSkeleton(QSize(kLongLineWidth, kLineHeight), lines));
        layout->addWidget(new ShadSkeleton(QSize(kShortLineWidth, kLineHeight), lines));
        return lines;
    }
} // namespace

SkeletonPage::SkeletonPage(QWidget* parent)
    : ExampleBasePage([] { return tr("骨架屏"); }, [] { return tr("ShadSkeleton：内容加载完成前的占位，全部骨架屏的脉冲动画保持同步。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("setCircular(true) 作为头像占位，其余为文字占位"); });
    QWidget* basicRow = new QWidget(this);
    QHBoxLayout* basicLayout = new QHBoxLayout(basicRow);
    basicLayout->setContentsMargins(0, 0, 0, 0);
    basicLayout->setSpacing(kAvatarGap);
    ShadSkeleton* avatar = new ShadSkeleton(QSize(kAvatarSize, kAvatarSize), basicRow);
    avatar->setCircular(true);
    basicLayout->addWidget(avatar, 0, Qt::AlignVCenter);
    basicLayout->addWidget(createLines(basicRow), 0, Qt::AlignVCenter);
    basicLayout->addStretch(1);
    addItem(basicCard, basicRow);

    ShadCard* cardCard = addSection([] { return tr("卡片"); }, [] { return tr("setRadius 设置圆角，图片占位使用 rounded-xl"); });
    QWidget* cardColumn = new QWidget(this);
    QVBoxLayout* cardLayout = new QVBoxLayout(cardColumn);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(kImageGap);
    ShadSkeleton* image = new ShadSkeleton(QSize(kLongLineWidth, kImageHeight), cardColumn);
    image->setRadius(shadTheme->getRadius(ShadThemeType::ExtraLarge));
    cardLayout->addWidget(image);
    cardLayout->addWidget(createLines(cardColumn));
    addItem(cardCard, cardColumn);

    ShadCard* animationCard = addSection([] { return tr("动画"); }, [] { return tr("setAnimated(false) 关闭脉冲动画，显示为静止的色块"); });
    ShadSwitch* animationSwitch = new ShadSwitch(this);
    animationSwitch->setChecked(true);
    ExampleI18n::bind(animationSwitch, [animationSwitch]() { animationSwitch->setText(tr("脉冲动画")); });
    QWidget* animationColumn = new QWidget(this);
    QVBoxLayout* animationLayout = new QVBoxLayout(animationColumn);
    animationLayout->setContentsMargins(0, 0, 0, 0);
    animationLayout->setSpacing(kLineGap);
    QList<ShadSkeleton*> skeletons;
    for (int index = 0; index < 3; ++index)
    {
        // 不设置宽度，随卡片伸展
        ShadSkeleton* skeleton = new ShadSkeleton(animationColumn);
        animationLayout->addWidget(skeleton);
        skeletons.append(skeleton);
    }
    connect(animationSwitch, &ShadSwitch::toggled, animationColumn, [skeletons](bool checked)
    {
        for (ShadSkeleton* skeleton : skeletons)
        {
            skeleton->setAnimated(checked);
        }
    });
    addItem(animationCard, animationSwitch);
    addStretchItem(animationCard, animationColumn);
}

SkeletonPage::~SkeletonPage()
{
}
