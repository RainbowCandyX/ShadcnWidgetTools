#include "NavigationMenuPage.h"

#include <QAbstractButton>
#include <QAction>
#include <QCoreApplication>
#include <QGridLayout>
#include <QList>
#include <QObject>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

#include "ExampleI18n.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadNavigationMenu.h"
#include "ShadNavigationMenuLink.h"

namespace
{
    constexpr int kGuideWidth = 320;
    constexpr int kComponentWidth = 500;
    constexpr int kGap = 8;

    struct LinkInfo
    {
        ExampleI18n::TextFunction title;
        ExampleI18n::TextFunction description;
    };

    // 状态文字：保存生成函数，切换语言时按最近点击的项重新生成
    struct StatusText
    {
        ShadLabel* label{nullptr};
        ExampleI18n::TextFunction text;

        void set(const ExampleI18n::TextFunction& newText)
        {
            text = newText;
            label->setText(text());
        }
    };

    ShadNavigationMenuLink* createLink(const LinkInfo& info, const std::shared_ptr<StatusText>& status, QWidget* parent)
    {
        ShadNavigationMenuLink* link = new ShadNavigationMenuLink(parent);
        ExampleI18n::bind(link, [link, info]()
        {
            link->setTitle(info.title());
            link->setDescription(info.description());
        });
        const ExampleI18n::TextFunction title = info.title;
        QObject::connect(link, &QAbstractButton::clicked, status->label, [status, title]() { status->set([title] { return QCoreApplication::translate("NavigationMenuPage", "最近点击: %1").arg(title()); }); });
        return link;
    }

    QWidget* createGuideContent(const std::shared_ptr<StatusText>& status)
    {
        const QList<LinkInfo> links{
            {[] { return QCoreApplication::translate("NavigationMenuPage", "介绍"); }, [] { return QCoreApplication::translate("NavigationMenuPage", "基于 Qt Widgets 的可复用组件，风格对齐 shadcn/ui。"); }},
            {[] { return QCoreApplication::translate("NavigationMenuPage", "安装"); }, [] { return QCoreApplication::translate("NavigationMenuPage", "用 CMake 引入组件库并链接到你的程序。"); }},
            {[] { return QCoreApplication::translate("NavigationMenuPage", "主题"); }, [] { return QCoreApplication::translate("NavigationMenuPage", "五种基色与亮暗模式，可单独修改任意颜色。"); }},
        };
        QWidget* content = new QWidget();
        content->setFixedWidth(kGuideWidth);
        QVBoxLayout* layout = new QVBoxLayout(content);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kGap);
        for (const LinkInfo& info : links)
        {
            layout->addWidget(createLink(info, status, content));
        }
        return content;
    }

    QWidget* createComponentContent(const std::shared_ptr<StatusText>& status)
    {
        const QList<LinkInfo> links{
            {[] { return QCoreApplication::translate("NavigationMenuPage", "警告对话框"); }, [] { return QCoreApplication::translate("NavigationMenuPage", "打断用户并等待确认的模态对话框。"); }},
            {[] { return QCoreApplication::translate("NavigationMenuPage", "悬停卡片"); }, [] { return QCoreApplication::translate("NavigationMenuPage", "悬停在链接上时预览更多内容。"); }},
            {[] { return QCoreApplication::translate("NavigationMenuPage", "进度条"); }, [] { return QCoreApplication::translate("NavigationMenuPage", "显示任务的完成进度。"); }},
            {[] { return QCoreApplication::translate("NavigationMenuPage", "滚动区域"); }, [] { return QCoreApplication::translate("NavigationMenuPage", "带自定义滚动条的滚动容器。"); }},
            {[] { return QCoreApplication::translate("NavigationMenuPage", "标签页"); }, [] { return QCoreApplication::translate("NavigationMenuPage", "分层显示内容，每次只显示一个面板。"); }},
            {[] { return QCoreApplication::translate("NavigationMenuPage", "提示"); }, [] { return QCoreApplication::translate("NavigationMenuPage", "悬停或聚焦时显示相关信息的小浮层。"); }},
        };
        QWidget* content = new QWidget();
        content->setFixedWidth(kComponentWidth);
        QGridLayout* layout = new QGridLayout(content);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kGap);
        for (int index = 0; index < int(links.size()); ++index)
        {
            layout->addWidget(createLink(links.at(index), status, content), index / 2, index % 2);
        }
        return content;
    }
} // namespace

NavigationMenuPage::NavigationMenuPage(QWidget* parent)
    : ExampleBasePage([] { return tr("导航菜单"); }, [] { return tr("ShadNavigationMenu：导航菜单。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("两组内容与一个链接"); });
    std::shared_ptr<StatusText> status = std::make_shared<StatusText>();
    status->label = new ShadLabel(this);
    status->label->setColorRole(ShadThemeType::MutedForeground);
    status->text = [] { return tr("最近点击: 无"); };
    ExampleI18n::bind(status->label, [status]() { status->label->setText(status->text()); });
    ShadNavigationMenu* navigationMenu = new ShadNavigationMenu(this);
    QWidget* guideContent = createGuideContent(status);
    QWidget* componentContent = createComponentContent(status);
    navigationMenu->addMenu(QString(), guideContent);
    navigationMenu->addMenu(QString(), componentContent);
    ExampleI18n::bind(navigationMenu, [navigationMenu, guideContent, componentContent]()
    {
        navigationMenu->setMenuTitle(guideContent, tr("入门"));
        navigationMenu->setMenuTitle(componentContent, tr("组件"));
    });
    QAction* docsAction = navigationMenu->addLink(QString());
    ExampleI18n::bind(docsAction, [docsAction]() { docsAction->setText(tr("文档")); });
    connect(docsAction, &QAction::triggered, status->label, [status]() { status->set([] { return tr("最近点击: 文档"); }); });
    addItem(basicCard, navigationMenu);
    addItem(basicCard, status->label);
}

NavigationMenuPage::~NavigationMenuPage()
{
}
