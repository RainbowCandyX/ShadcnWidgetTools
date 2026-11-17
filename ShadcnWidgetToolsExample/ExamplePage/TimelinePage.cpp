#include "TimelinePage.h"

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadSwitch.h"
#include "ShadTimeline.h"

namespace
{
    constexpr int kTimelineWidth = 420;
    constexpr int kAlternateWidth = 560;

    ShadTimelineItem* addTimelineItem(ShadTimeline* timeline, ExampleI18n::TextFunction title, ExampleI18n::TextFunction time, ExampleI18n::TextFunction description)
    {
        ShadTimelineItem* item = timeline->addItem(QString());
        ExampleI18n::bind(item, [item, title, time, description]()
        {
            item->setTitle(title());
            item->setTime(time ? time() : QString());
            item->setDescription(description ? description() : QString());
        });
        return item;
    }

    void addOrderItems(ShadTimeline* timeline)
    {
        addTimelineItem(timeline, [] { return TimelinePage::tr("订单已提交"); }, [] { return QStringLiteral("2026-11-10 09:12"); }, [] { return TimelinePage::tr("等待商家确认"); });
        addTimelineItem(timeline, [] { return TimelinePage::tr("商家已发货"); }, [] { return QStringLiteral("2026-11-10 15:40"); }, [] { return TimelinePage::tr("包裹已交给快递公司"); });
        addTimelineItem(timeline, [] { return TimelinePage::tr("运输中"); }, [] { return QStringLiteral("2026-11-11 08:05"); }, [] { return TimelinePage::tr("已到达转运中心"); });
        addTimelineItem(timeline, [] { return TimelinePage::tr("派送中"); }, [] { return QStringLiteral("2026-11-12 10:30"); }, nullptr);
    }
} // namespace

TimelinePage::TimelinePage(QWidget* parent)
    : ExampleBasePage([] { return tr("时间轴"); }, [] { return tr("ShadTimeline：按时间顺序纵向排列的节点，用于操作记录、版本历史与物流进度。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("addItem 添加节点，每个节点包含时间、标题与描述"); });
    ShadTimeline* basicTimeline = new ShadTimeline(this);
    basicTimeline->setMaximumWidth(kTimelineWidth);
    addOrderItems(basicTimeline);
    addStretchItem(basicCard, basicTimeline);

    ShadCard* colorCard = addSection([] { return tr("颜色与图标"); }, [] { return tr("setColor 设置圆点颜色，setIcon 用图标代替圆点"); });
    ShadTimeline* colorTimeline = new ShadTimeline(this);
    colorTimeline->setMaximumWidth(kTimelineWidth);
    ShadTimelineItem* createdItem = addTimelineItem(colorTimeline, [] { return tr("创建仓库"); }, [] { return QStringLiteral("v0.1.0"); }, [] { return tr("初始化项目结构"); });
    createdItem->setIcon(ShadIconType::GitCommitHorizontal);
    ShadTimelineItem* releasedItem = addTimelineItem(colorTimeline, [] { return tr("发布正式版"); }, [] { return QStringLiteral("v1.0.0"); }, [] { return tr("全部测试通过"); });
    releasedItem->setIcon(ShadIconType::CircleCheck);
    releasedItem->setColor(ShadThemeType::Chart2);
    ShadTimelineItem* failedItem = addTimelineItem(colorTimeline, [] { return tr("构建失败"); }, [] { return QStringLiteral("v1.0.1"); }, [] { return tr("依赖下载超时"); });
    failedItem->setColor(ShadThemeType::Destructive);
    ShadTimelineItem* draftItem = addTimelineItem(colorTimeline, [] { return tr("草稿"); }, [] { return QStringLiteral("v1.1.0"); }, nullptr);
    draftItem->setColor(ShadThemeType::MutedForeground);
    addStretchItem(colorCard, colorTimeline);

    ShadCard* alternateCard = addSection([] { return tr("交替排列"); }, [] { return tr("setMode(Alternate)：轴居中，节点左右交替，时间显示在另一侧；End 模式轴在结束侧"); });
    ShadTimeline* alternateTimeline = new ShadTimeline(this);
    alternateTimeline->setMaximumWidth(kAlternateWidth);
    alternateTimeline->setMode(ShadTimelineType::Alternate);
    addOrderItems(alternateTimeline);
    addStretchItem(alternateCard, alternateTimeline);
    ShadTimeline* endTimeline = new ShadTimeline(this);
    endTimeline->setMaximumWidth(kTimelineWidth);
    endTimeline->setMode(ShadTimelineType::End);
    addOrderItems(endTimeline);
    addStretchItem(alternateCard, endTimeline);

    ShadCard* pendingCard = addSection([] { return tr("进行中与倒序"); }, [] { return tr("setPending 在末尾追加旋转图标的进行中节点，相连的一段为虚线；setReverse 倒序显示"); });
    ShadSwitch* reverseSwitch = new ShadSwitch(this);
    ExampleI18n::bind(reverseSwitch, [reverseSwitch]() { reverseSwitch->setText(tr("倒序")); });
    addItem(pendingCard, reverseSwitch);
    ShadTimeline* pendingTimeline = new ShadTimeline(this);
    pendingTimeline->setMaximumWidth(kTimelineWidth);
    addOrderItems(pendingTimeline);
    ExampleI18n::bind(pendingTimeline, [pendingTimeline]() { pendingTimeline->setPending(tr("等待签收...")); });
    connect(reverseSwitch, &QAbstractButton::toggled, pendingTimeline, &ShadTimeline::setReverse);
    addStretchItem(pendingCard, pendingTimeline);

    ShadCard* contentCard = addSection([] { return tr("自定义内容"); }, [] { return tr("contentLayout 可在节点中放入任意组件"); });
    ShadTimeline* contentTimeline = new ShadTimeline(this);
    contentTimeline->setMaximumWidth(kTimelineWidth);
    ShadTimelineItem* reviewItem = addTimelineItem(contentTimeline, [] { return tr("等待审核"); }, [] { return QStringLiteral("2026-11-15 14:00"); }, [] { return tr("合并请求需要一位成员批准"); });
    QWidget* buttonRow = new QWidget(reviewItem);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonRow);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(2);
    ShadButton* approveButton = new ShadButton(QString(), ShadButtonType::Default, buttonRow);
    approveButton->setButtonSize(ShadButtonType::Small);
    ShadButton* rejectButton = new ShadButton(QString(), ShadButtonType::Outline, buttonRow);
    rejectButton->setButtonSize(ShadButtonType::Small);
    ExampleI18n::bind(approveButton, [approveButton, rejectButton]()
    {
        approveButton->setText(tr("批准"));
        rejectButton->setText(tr("拒绝"));
    });
    buttonLayout->addWidget(approveButton);
    buttonLayout->addWidget(rejectButton);
    buttonLayout->addStretch(1);
    reviewItem->contentLayout()->addWidget(buttonRow);
    addTimelineItem(contentTimeline, [] { return tr("已合并"); }, [] { return QStringLiteral("2026-11-15 16:20"); }, nullptr);
    addStretchItem(contentCard, contentTimeline);
}

TimelinePage::~TimelinePage()
{
}
