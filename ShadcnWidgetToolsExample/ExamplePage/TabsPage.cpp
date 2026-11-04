#include "TabsPage.h"

#include <QList>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadTabs.h"

namespace
{
    // 纵向示例中内容卡片的宽度
    constexpr int kVerticalContentWidth = 320;

    // 标签对应的内容：一张卡片，内容为 text-sm text-muted-foreground；文字随语言刷新
    ShadCard* createPanel(ExampleI18n::TextFunction title, ExampleI18n::TextFunction description, ExampleI18n::TextFunction content, QWidget* parent)
    {
        ShadCard* card = new ShadCard(parent);
        ShadLabel* label = new ShadLabel(card);
        label->setColorRole(ShadThemeType::MutedForeground);
        label->setWordWrap(true);
        card->contentLayout()->addWidget(label);
        ExampleI18n::bind(card, [card, label, title, description, content]()
        {
            card->setTitle(title());
            card->setDescription(description());
            label->setText(content());
        });
        return card;
    }

    ShadLabel* createMutedText(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    // 按序号刷新已添加标签的文字，不重建标签，当前标签与焦点保持不变
    void bindTabTexts(ShadTabs* tabs, const QList<ExampleI18n::TextFunction>& texts)
    {
        ExampleI18n::bind(tabs, [tabs, texts]()
        {
            for (int index = 0; index < texts.size(); ++index)
            {
                tabs->setTabText(index, texts.at(index)());
            }
        });
    }
} // namespace

TabsPage::TabsPage(QWidget* parent)
    : ExampleBasePage([] { return tr("标签页"); }, [] { return tr("ShadTabs：标签页组件，一组分层的内容每次只显示其中一个。"); }, parent)
{
    // 基础用法：四个标签，每个标签对应一张卡片
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("添加标签与页面，只显示当前标签的页面"); });
    ShadTabs* basicTabs = new ShadTabs(this);
    struct Panel
    {
        ExampleI18n::TextFunction tab;
        ExampleI18n::TextFunction description;
        ExampleI18n::TextFunction content;
    };
    // 标签文字同时作为卡片标题
    const QList<Panel> panels{
        {[] { return tr("概览"); }, [] { return tr("查看关键指标与最近的项目动态，跟踪所有进行中项目的进度。"); }, [] { return tr("你有 12 个进行中的项目与 3 个待处理的任务。"); }},
        {[] { return tr("分析"); }, [] { return tr("跟踪性能与用户参与度，观察趋势并发现增长机会。"); }, [] { return tr("页面浏览量比上月增长 25%。"); }},
        {[] { return tr("报告"); }, [] { return tr("生成并下载详细报告，导出多种格式的数据用于分析。"); }, [] { return tr("有 5 份报告已生成，可以导出。"); }},
        {[] { return tr("设置"); }, [] { return tr("管理账户偏好与选项，按需定制使用体验。"); }, [] { return tr("配置通知、安全与主题。"); }},
    };
    QList<ExampleI18n::TextFunction> panelTabs;
    for (const Panel& panel : panels)
    {
        basicTabs->addTab(QString(), createPanel(panel.tab, panel.description, panel.content, basicTabs));
        panelTabs.append(panel.tab);
    }
    bindTabTexts(basicTabs, panelTabs);
    addStretchItem(basicCard, basicTabs);

    // 线条样式：横向与纵向
    ShadCard* lineCard = addSection([] { return tr("线条样式"); }, [] { return tr("setVariant(Line)：背景透明，选中项显示指示线"); });
    ShadTabs* lineTabs = new ShadTabs(this);
    lineTabs->setVariant(ShadTabsType::Line);
    lineTabs->addTab(QString());
    lineTabs->addTab(QString());
    lineTabs->addTab(QString());
    bindTabTexts(lineTabs, {[] { return tr("概览"); }, [] { return tr("分析"); }, [] { return tr("报告"); }});
    ShadTabs* verticalLineTabs = new ShadTabs(this);
    verticalLineTabs->setVariant(ShadTabsType::Line);
    verticalLineTabs->setOrientation(Qt::Vertical);
    verticalLineTabs->addTab(QString());
    verticalLineTabs->addTab(QString());
    verticalLineTabs->addTab(QString());
    bindTabTexts(verticalLineTabs, {[] { return tr("账户"); }, [] { return tr("密码"); }, [] { return tr("通知"); }});
    addItem(lineCard, lineTabs);
    addItem(lineCard, verticalLineTabs);

    // 纵向：标签栏在起始侧，内容在旁边
    ShadCard* verticalCard = addSection([] { return tr("纵向"); }, [] { return tr("setOrientation(Qt::Vertical)：标签纵向排列在起始侧"); });
    ShadTabs* verticalTabs = new ShadTabs(this);
    verticalTabs->setOrientation(Qt::Vertical);
    const QList<Panel> settingsPanels{
        {[] { return tr("账户"); }, [] { return tr("修改账户信息，保存后生效。"); }, [] { return tr("用户名、头像与个人简介。"); }},
        {[] { return tr("密码"); }, [] { return tr("修改密码，保存后需要重新登录。"); }, [] { return tr("建议使用 12 位以上的密码。"); }},
        {[] { return tr("通知"); }, [] { return tr("选择接收通知的方式。"); }, [] { return tr("邮件、推送与站内消息。"); }},
    };
    QList<ExampleI18n::TextFunction> settingsTabs;
    for (const Panel& panel : settingsPanels)
    {
        ShadCard* card = createPanel(panel.tab, panel.description, panel.content, verticalTabs);
        card->setFixedWidth(kVerticalContentWidth);
        verticalTabs->addTab(QString(), card);
        settingsTabs.append(panel.tab);
    }
    bindTabTexts(verticalTabs, settingsTabs);
    addItem(verticalCard, verticalTabs);

    // 禁用
    ShadCard* disabledCard = addSection([] { return tr("禁用"); }, [] { return tr("禁用的标签半透明，不能点击，键盘导航时跳过"); });
    ShadTabs* disabledTabs = new ShadTabs(this);
    disabledTabs->addTab(QString());
    disabledTabs->addTab(QString());
    disabledTabs->addTab(QString());
    disabledTabs->setTabEnabled(1, false);
    bindTabTexts(disabledTabs, {[] { return tr("首页"); }, [] { return tr("已禁用"); }, [] { return tr("设置"); }});
    addItem(disabledCard, disabledTabs);

    // 图标
    ShadCard* iconsCard = addSection([] { return tr("图标"); }, [] { return tr("在标签文字前显示图标"); });
    const QList<ExampleI18n::TextFunction> iconTabTexts{[] { return tr("预览"); }, [] { return tr("代码"); }};
    ShadTabs* iconTabs = new ShadTabs(this);
    iconTabs->addTab(ShadIconType::AppWindow, QString());
    iconTabs->addTab(ShadIconType::Code, QString());
    bindTabTexts(iconTabs, iconTabTexts);
    ShadTabs* iconLineTabs = new ShadTabs(this);
    iconLineTabs->setVariant(ShadTabsType::Line);
    iconLineTabs->addTab(ShadIconType::AppWindow, QString());
    iconLineTabs->addTab(ShadIconType::Code, QString());
    bindTabTexts(iconLineTabs, iconTabTexts);
    addRow(iconsCard, {iconTabs, iconLineTabs});

    // 键盘切换：方向键只移动焦点，或移动焦点时同时切换
    ShadCard* keyboardCard = addSection([] { return tr("键盘切换"); }, [] { return tr("用方向键在标签间移动，可设置为移动时直接切换"); });
    struct KeyboardDemo
    {
        ExampleI18n::TextFunction caption;
        bool isActivateOnFocus;
    };
    const QList<KeyboardDemo> keyboardDemos{
        {[] { return tr("回车或空格切换"); }, false},
        {[] { return tr("方向键直接切换"); }, true},
    };
    for (const KeyboardDemo& demo : keyboardDemos)
    {
        ShadTabs* tabs = new ShadTabs(this);
        tabs->setActivateOnFocus(demo.isActivateOnFocus);
        tabs->addTab(QString());
        tabs->addTab(QString());
        tabs->addTab(QString());
        tabs->addTab(QString());
        // 先绑定标签文字，切换语言时标签先刷新，状态文字再读取刷新后的标签文字
        bindTabTexts(tabs, {[] { return tr("日"); }, [] { return tr("周"); }, [] { return tr("月"); }, [] { return tr("年"); }});
        ShadLabel* stateLabel = createMutedText(this);
        const ExampleI18n::TextFunction caption = demo.caption;
        const auto updateState = [tabs, stateLabel, caption]() { stateLabel->setText(tr("%1：当前为 %2").arg(caption(), tabs->tabText(tabs->currentIndex()))); };
        connect(tabs, &ShadTabs::currentChanged, stateLabel, updateState);
        ExampleI18n::bind(stateLabel, updateState);
        addRow(keyboardCard, {tabs, stateLabel});
    }
}

TabsPage::~TabsPage()
{
}
