#include "MarqueePage.h"

#include <QWidget>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadMarquee.h"
#include "ShadSwitch.h"

namespace
{
    constexpr int kFastSpeed = 80;
    constexpr int kFadeWidth = 48;
} // namespace

MarqueePage::MarqueePage(QWidget* parent)
    : ExampleBasePage([] { return tr("跑马灯"); }, [] { return tr("ShadMarquee：一段文字沿水平方向匀速循环滚动，用于公告与提示。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("文字首尾相接铺满整个宽度，默认每秒滚动 40 像素"); });
    ShadMarquee* basicMarquee = new ShadMarquee(this);
    ExampleI18n::bind(basicMarquee, [basicMarquee]() { basicMarquee->setText(tr("ShadcnWidgetTools 新版本已发布：新增 K 线图、差异对比、跑马灯与骨架屏组件。")); });
    addStretchItem(basicCard, basicMarquee);

    ShadCard* directionCard = addSection([] { return tr("方向与速度"); }, [] { return tr("setReversed(true) 反向滚动，setSpeed(80) 加快速度，setGap 调整两段文字的间距"); });
    ShadMarquee* reversedMarquee = new ShadMarquee(this);
    reversedMarquee->setReversed(true);
    reversedMarquee->setSpeed(kFastSpeed);
    reversedMarquee->setColorRole(ShadThemeType::MutedForeground);
    ExampleI18n::bind(reversedMarquee, [reversedMarquee]() { reversedMarquee->setText(tr("系统将于今晚 23:00 至次日 01:00 进行维护，期间服务暂停。")); });
    addStretchItem(directionCard, reversedMarquee);

    ShadCard* hoverCard = addSection([] { return tr("悬停暂停与渐隐"); }, [] { return tr("setPauseOnHover(true) 悬停时暂停，setFadeWidth(48) 让两端渐隐"); });
    ShadMarquee* hoverMarquee = new ShadMarquee(this);
    hoverMarquee->setPauseOnHover(true);
    hoverMarquee->setFadeWidth(kFadeWidth);
    ExampleI18n::bind(hoverMarquee, [hoverMarquee]() { hoverMarquee->setText(tr("把鼠标移到这里可以暂停滚动，移开后从当前位置继续。")); });
    addStretchItem(hoverCard, hoverMarquee);

    ShadCard* overflowCard = addSection([] { return tr("仅在超出时滚动"); }, [] { return tr("setOverflowOnly(true)：文字放得下时静止显示，缩小窗口到放不下时开始滚动"); });
    ShadMarquee* shortMarquee = new ShadMarquee(this);
    shortMarquee->setOverflowOnly(true);
    ExampleI18n::bind(shortMarquee, [shortMarquee]() { shortMarquee->setText(tr("这段文字较短，不会滚动。")); });
    ShadMarquee* longMarquee = new ShadMarquee(this);
    longMarquee->setOverflowOnly(true);
    ExampleI18n::bind(longMarquee, [longMarquee]() { longMarquee->setText(tr("这段文字较长：当组件宽度不足以完整显示全部内容时，文字会自动开始滚动，窗口足够宽时保持静止并靠起始侧显示。")); });
    addStretchItem(overflowCard, shortMarquee);
    addStretchItem(overflowCard, longMarquee);

    ShadCard* runningCard = addSection([] { return tr("启停"); }, [] { return tr("setRunning(false) 停止滚动并回到起点"); });
    ShadSwitch* runningSwitch = new ShadSwitch(this);
    runningSwitch->setChecked(true);
    ExampleI18n::bind(runningSwitch, [runningSwitch]() { runningSwitch->setText(tr("滚动")); });
    ShadMarquee* runningMarquee = new ShadMarquee(this);
    ExampleI18n::bind(runningMarquee, [runningMarquee]() { runningMarquee->setText(tr("关闭开关后文字停止滚动并回到起点。")); });
    connect(runningSwitch, &ShadSwitch::toggled, runningMarquee, [runningMarquee](bool checked) { runningMarquee->setRunning(checked); });
    addItem(runningCard, runningSwitch);
    addStretchItem(runningCard, runningMarquee);
}

MarqueePage::~MarqueePage()
{
}
