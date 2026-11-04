#include "BreadcrumbPage.h"

#include <QAction>
#include <QCoreApplication>
#include <QString>
#include <QStringList>

#include <functional>

#include "ExampleI18n.h"
#include "ShadBreadcrumb.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDropdownMenu.h"
#include "ShadToaster.h"

namespace
{
    // 长文字示例的容器宽度
    constexpr int kNarrowWidth = 320;

    // 对应 shadcn 示例中的 Documentation / Themes / GitHub 菜单
    ShadDropdownMenu* createDocsMenu(QWidget* parent)
    {
        ShadDropdownMenu* menu = new ShadDropdownMenu(parent);
        QAction* docsAction = menu->addItem(QString());
        QAction* themesAction = menu->addItem(QString());
        menu->addItem(QStringLiteral("GitHub"));
        ExampleI18n::bind(menu, [docsAction, themesAction]()
        {
            docsAction->setText(QCoreApplication::translate("BreadcrumbPage", "文档"));
            themesAction->setText(QCoreApplication::translate("BreadcrumbPage", "主题"));
        });
        return menu;
    }

    // 按序号刷新面包屑已有各项的文字，不改变当前路径；texts 中为空的项（例如省略号）跳过
    void bindItemTexts(ShadBreadcrumb* breadcrumb, const std::function<QStringList()>& texts)
    {
        ExampleI18n::bind(breadcrumb, [breadcrumb, texts]()
        {
            const QStringList currentTexts = texts();
            for (int index = 0; index < breadcrumb->count() && index < currentTexts.size(); ++index)
            {
                if (!currentTexts.at(index).isEmpty())
                {
                    breadcrumb->setItemText(index, currentTexts.at(index));
                }
            }
        });
    }
} // namespace

BreadcrumbPage::BreadcrumbPage(ShadToaster* toaster, QWidget* parent)
    : ExampleBasePage([] { return tr("面包屑"); }, [] { return tr("ShadBreadcrumb：显示当前页面在层级中的路径，支持下拉菜单与自动折叠。"); }, parent)
{
    const auto notify = [toaster](const QString& title, const QString& description)
    {
        if (toaster != nullptr)
        {
            toaster->add(title, description);
        }
    };
    const auto basicPath = []() { return QStringList{tr("首页"), tr("组件"), tr("面包屑")}; };

    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("setPath：一次设置整条路径，末项为当前页"); });
    ShadBreadcrumb* basicBreadcrumb = new ShadBreadcrumb(this);
    basicBreadcrumb->setPath(basicPath());
    bindItemTexts(basicBreadcrumb, basicPath);
    addItem(basicCard, basicBreadcrumb);

    ShadCard* separatorCard = addSection([] { return tr("自定义分隔符"); }, [] { return tr("setSeparatorIcon：替换默认的分隔符图标"); });
    ShadBreadcrumb* dotBreadcrumb = new ShadBreadcrumb(this);
    dotBreadcrumb->setSeparatorIcon(ShadIconType::Dot);
    dotBreadcrumb->setPath(basicPath());
    bindItemTexts(dotBreadcrumb, basicPath);
    addItem(separatorCard, dotBreadcrumb);

    ShadCard* dropdownCard = addSection([] { return tr("下拉菜单"); }, [] { return tr("addDropdown：点击层级项弹出下拉菜单"); });
    ShadBreadcrumb* dropdownBreadcrumb = new ShadBreadcrumb(this);
    dropdownBreadcrumb->setSeparatorIcon(ShadIconType::Dot);
    ShadDropdownMenu* componentsMenu = createDocsMenu(this);
    connect(componentsMenu, &ShadDropdownMenu::itemTriggered, this, [notify](QAction* action)
    {
        notify(tr("打开菜单项"), action->text());
    });
    dropdownBreadcrumb->addLink(QString());
    dropdownBreadcrumb->addDropdown(QString(), componentsMenu);
    dropdownBreadcrumb->addPage(QString());
    bindItemTexts(dropdownBreadcrumb, basicPath);
    addItem(dropdownCard, dropdownBreadcrumb);

    ShadCard* ellipsisCard = addSection([] { return tr("省略号"); }, [] { return tr("addEllipsis：省略中间层级，可点击弹出菜单"); });
    const auto ellipsisPath = []() { return QStringList{tr("首页"), QString(), tr("组件"), tr("面包屑")}; };
    ShadBreadcrumb* menuEllipsisBreadcrumb = new ShadBreadcrumb(this);
    ShadDropdownMenu* ellipsisMenu = createDocsMenu(this);
    connect(ellipsisMenu, &ShadDropdownMenu::itemTriggered, this, [notify](QAction* action)
    {
        notify(tr("打开菜单项"), action->text());
    });
    menuEllipsisBreadcrumb->addLink(QString());
    menuEllipsisBreadcrumb->addEllipsis(ellipsisMenu);
    menuEllipsisBreadcrumb->addLink(QString());
    menuEllipsisBreadcrumb->addPage(QString());
    bindItemTexts(menuEllipsisBreadcrumb, ellipsisPath);
    ShadBreadcrumb* staticEllipsisBreadcrumb = new ShadBreadcrumb(this);
    staticEllipsisBreadcrumb->addLink(QString());
    staticEllipsisBreadcrumb->addEllipsis();
    staticEllipsisBreadcrumb->addLink(QString());
    staticEllipsisBreadcrumb->addPage(QString());
    bindItemTexts(staticEllipsisBreadcrumb, ellipsisPath);
    addItem(ellipsisCard, menuEllipsisBreadcrumb);
    addItem(ellipsisCard, staticEllipsisBreadcrumb);

    ShadCard* collapseCard = addSection([] { return tr("自动折叠"); }, []
    {
        return tr("setMaximumVisibleItems(3)：中间层级收进省略号菜单");
    });
    const auto collapsePath = []() { return QStringList{tr("首页"), tr("文档"), tr("组件"), tr("表单"), tr("输入"), tr("文本框")}; };
    ShadBreadcrumb* collapseBreadcrumb = new ShadBreadcrumb(this);
    collapseBreadcrumb->setMaximumVisibleItems(3);
    collapseBreadcrumb->setPath(collapsePath());
    bindItemTexts(collapseBreadcrumb, collapsePath);
    connect(collapseBreadcrumb, &ShadBreadcrumb::linkClicked, this, [notify, collapseBreadcrumb](int index)
    {
        notify(tr("跳转"), collapseBreadcrumb->getItemText(index));
    });
    addItem(collapseCard, collapseBreadcrumb);

    ShadCard* longTextCard = addSection([] { return tr("长文字"); }, []
    {
        return tr("setMaximumItemWidth：限制单项宽度，超出时省略");
    });
    const auto longPath = []()
    {
        return QStringList{tr("首页"), tr("2026 年第三季度产品规划与路线图"), tr("设计评审"), tr("移动端导航栏交互方案（第二版）")};
    };
    ShadBreadcrumb* longBreadcrumb = new ShadBreadcrumb(this);
    longBreadcrumb->setFixedWidth(kNarrowWidth);
    longBreadcrumb->setMaximumItemWidth(120);
    longBreadcrumb->setPath(longPath());
    bindItemTexts(longBreadcrumb, longPath);
    addItem(longTextCard, longBreadcrumb);

    ShadCard* navigateCard = addSection([] { return tr("导航"); }, [] { return tr("点击链接截掉后面的层级，按钮恢复完整路径"); });
    ShadBreadcrumb* navigateBreadcrumb = new ShadBreadcrumb(this);
    const std::function<QStringList()> fullPath = []()
    {
        return QStringList{
            tr("首页"), tr("项目"), QStringLiteral("ShadcnWidgetTools"), tr("源代码"), QStringLiteral("ShadBreadcrumb.cpp")
        };
    };
    navigateBreadcrumb->setPath(fullPath());
    // 只刷新当前保留的层级，用户截掉的层级不会恢复
    bindItemTexts(navigateBreadcrumb, fullPath);
    connect(navigateBreadcrumb, &ShadBreadcrumb::linkClicked, this, [notify, navigateBreadcrumb, fullPath](int index)
    {
        const QStringList currentPath = fullPath();
        // 在点击信号中修改面包屑是安全的，旧的子组件会延后删除
        navigateBreadcrumb->setPath(currentPath.mid(0, index + 1));
        notify(tr("跳转"), currentPath.at(index));
    });
    ShadButton* resetButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    resetButton->setButtonSize(ShadButtonType::Small);
    ExampleI18n::bind(resetButton, [resetButton]() { resetButton->setText(tr("恢复完整路径")); });
    connect(resetButton, &ShadButton::clicked, navigateBreadcrumb, [navigateBreadcrumb, fullPath]()
    {
        navigateBreadcrumb->setPath(fullPath());
    });
    addItem(navigateCard, navigateBreadcrumb);
    addItem(navigateCard, resetButton);
}

BreadcrumbPage::~BreadcrumbPage()
{
}
