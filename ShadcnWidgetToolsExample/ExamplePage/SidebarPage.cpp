#include "SidebarPage.h"

#include <QCoreApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QList>
#include <QPointer>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include <memory>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ExampleMutedItem.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadSeparator.h"
#include "ShadSidebar.h"
#include "ShadSidebarGroup.h"
#include "ShadSidebarLayout.h"
#include "ShadSidebarMenuButton.h"
#include "ShadSidebarTrigger.h"
#include "ShadToggle.h"
#include "ShadToggleGroup.h"

namespace
{
    // 迷你应用的高度
    constexpr int kDemoHeight = 560;
    // 主内容区顶部 h-16，占位块高度
    constexpr int kHeaderHeight = 64;
    constexpr int kPlaceholderHeight = 120;
    constexpr int kContentPadding = 16;

    ShadLabel* createMutedText(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    // 菜单按钮文字随语言刷新
    void bindButtonText(ShadSidebarMenuButton* button, const ExampleI18n::TextFunction& text)
    {
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
    }

    // 分组标题随语言刷新
    void bindGroupLabel(ShadSidebarGroup* group, const ExampleI18n::TextFunction& label)
    {
        ExampleI18n::bind(group, [group, label]() { group->setLabel(label()); });
    }

    // 单选切换组：再次点击已选项不会取消选中
    ShadToggleGroup* createChoiceGroup(const QList<QString>& items, QWidget* parent)
    {
        ShadToggleGroup* group = new ShadToggleGroup(ShadToggleGroupType::Single, parent);
        group->setVariant(ShadToggleType::Outline);
        for (const QString& item : items)
        {
            group->addItem(item);
        }
        group->setCurrentIndex(0);
        return group;
    }

    // 一行：标签 + 切换组，标签随语言刷新
    QWidget* createChoiceRow(const ExampleI18n::TextFunction& label, ShadToggleGroup* group, QWidget* parent)
    {
        QWidget* row = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(12);
        ShadLabel* labelWidget = new ShadLabel(row);
        labelWidget->setFixedWidth(72);
        ExampleI18n::bind(labelWidget, [labelWidget, label]() { labelWidget->setText(label()); });
        group->setParent(row);
        layout->addWidget(labelWidget);
        layout->addWidget(group);
        layout->addStretch(1);
        return row;
    }

    // 对应 sidebar-demo：页头团队、平台分组（可折叠子菜单）、项目分组（徽章与操作按钮）、可折叠的帮助分组、页脚用户
    ShadSidebar* createDemoSidebar(QWidget* parent)
    {
        ShadSidebar* sidebar = new ShadSidebar(parent);
        ShadSidebarMenuButton* teamButton = new ShadSidebarMenuButton(ShadIconType::GalleryVerticalEnd, QStringLiteral("Acme Inc"), sidebar);
        teamButton->setButtonSize(ShadSidebarMenuButtonType::Large);
        ExampleI18n::bind(teamButton, [teamButton]() { teamButton->setDescription(QCoreApplication::translate("SidebarPage", "企业版")); });
        teamButton->setActionIcon(ShadIconType::ChevronsUpDown);
        teamButton->setActionAlwaysVisible(true);
        sidebar->headerLayout()->addWidget(teamButton);

        ShadSidebarGroup* platformGroup = sidebar->addGroup();
        bindGroupLabel(platformGroup, [] { return QCoreApplication::translate("SidebarPage", "平台"); });
        struct NavItem
        {
            ShadIconType::IconName icon;
            ExampleI18n::TextFunction title;
            QList<ExampleI18n::TextFunction> subItems;
        };
        const QList<NavItem> navItems{
            {ShadIconType::SquareTerminal, [] { return QCoreApplication::translate("SidebarPage", "演练场"); }, {[] { return QCoreApplication::translate("SidebarPage", "历史记录"); }, [] { return QCoreApplication::translate("SidebarPage", "已收藏"); }, [] { return QCoreApplication::translate("SidebarPage", "设置"); }}},
            {ShadIconType::Bot, [] { return QCoreApplication::translate("SidebarPage", "模型"); }, {[] { return QStringLiteral("Genesis"); }, [] { return QStringLiteral("Explorer"); }, [] { return QStringLiteral("Quantum"); }}},
            {ShadIconType::BookOpen, [] { return QCoreApplication::translate("SidebarPage", "文档"); }, {[] { return QCoreApplication::translate("SidebarPage", "简介"); }, [] { return QCoreApplication::translate("SidebarPage", "快速开始"); }, [] { return QCoreApplication::translate("SidebarPage", "教程"); }, [] { return QCoreApplication::translate("SidebarPage", "更新日志"); }}},
            {ShadIconType::Settings2, [] { return QCoreApplication::translate("SidebarPage", "设置"); }, {[] { return QCoreApplication::translate("SidebarPage", "通用"); }, [] { return QCoreApplication::translate("SidebarPage", "团队"); }, [] { return QCoreApplication::translate("SidebarPage", "账单"); }, [] { return QCoreApplication::translate("SidebarPage", "用量"); }}},
        };
        ShadSidebarMenuButton* firstButton = nullptr;
        ShadSidebarMenuButton* firstSubItem = nullptr;
        for (const NavItem& navItem : navItems)
        {
            ShadSidebarMenuButton* button = platformGroup->addItem(navItem.icon, QString());
            bindButtonText(button, navItem.title);
            for (const ExampleI18n::TextFunction& subItem : navItem.subItems)
            {
                ShadSidebarMenuButton* subButton = button->addSubItem(QString());
                bindButtonText(subButton, subItem);
                if (firstSubItem == nullptr)
                {
                    firstSubItem = subButton;
                }
            }
            if (firstButton == nullptr)
            {
                firstButton = button;
            }
        }
        // 第一项默认展开，并选中其中的"历史记录"
        if (firstButton != nullptr)
        {
            firstButton->setSubMenuExpanded(true);
        }
        if (firstSubItem != nullptr)
        {
            sidebar->setActiveButton(firstSubItem);
        }

        ShadSidebarGroup* projectGroup = sidebar->addGroup();
        bindGroupLabel(projectGroup, [] { return QCoreApplication::translate("SidebarPage", "项目"); });
        projectGroup->setActionIcon(ShadIconType::Plus);
        ShadSidebarMenuButton* designButton = projectGroup->addItem(ShadIconType::Frame, QString());
        designButton->setBadgeText(QStringLiteral("24"));
        bindButtonText(designButton, [] { return QCoreApplication::translate("SidebarPage", "设计工程"); });
        ShadSidebarMenuButton* salesButton = projectGroup->addItem(ShadIconType::ChartPie, QString());
        salesButton->setActionIcon(ShadIconType::Ellipsis);
        bindButtonText(salesButton, [] { return QCoreApplication::translate("SidebarPage", "销售与市场"); });
        ShadSidebarMenuButton* travelButton = projectGroup->addItem(ShadIconType::Map, QString());
        travelButton->setActionIcon(ShadIconType::Ellipsis);
        bindButtonText(travelButton, [] { return QCoreApplication::translate("SidebarPage", "差旅"); });

        ShadSidebarGroup* helpGroup = sidebar->addGroup();
        bindGroupLabel(helpGroup, [] { return QCoreApplication::translate("SidebarPage", "帮助"); });
        helpGroup->setCollapsible(true);
        bindButtonText(helpGroup->addItem(ShadIconType::LifeBuoy, QString()), [] { return QCoreApplication::translate("SidebarPage", "支持"); });
        bindButtonText(helpGroup->addItem(ShadIconType::Send, QString()), [] { return QCoreApplication::translate("SidebarPage", "反馈"); });

        ShadSidebarMenuButton* userButton = new ShadSidebarMenuButton(ShadIconType::User, QStringLiteral("ShadcnWidgetTools"), sidebar);
        userButton->setButtonSize(ShadSidebarMenuButtonType::Large);
        userButton->setDescription(QStringLiteral("m@example.com"));
        sidebar->footerLayout()->addWidget(userButton);
        return sidebar;
    }

    // 主内容区：顶部切换按钮与当前位置，下方为占位块；当前位置的文字由调用方设置
    QWidget* createDemoContent(ShadSidebar* sidebar, ShadLabel** locationLabel, QWidget* parent)
    {
        QWidget* content = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(content);
        layout->setContentsMargins(kContentPadding, 0, kContentPadding, kContentPadding);
        layout->setSpacing(kContentPadding);
        QWidget* header = new QWidget(content);
        header->setFixedHeight(kHeaderHeight);
        QHBoxLayout* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(8);
        ShadSidebarTrigger* trigger = new ShadSidebarTrigger(sidebar, header);
        ExampleI18n::bind(trigger, [trigger]() { trigger->setToolTip(QCoreApplication::translate("SidebarPage", "切换侧栏（Ctrl+B / Cmd+B）")); });
        ShadSeparator* separator = new ShadSeparator(Qt::Vertical, header);
        separator->setFixedHeight(16);
        *locationLabel = new ShadLabel(header);
        headerLayout->addWidget(trigger);
        headerLayout->addWidget(separator);
        headerLayout->addWidget(*locationLabel);
        headerLayout->addStretch(1);
        layout->addWidget(header);
        QHBoxLayout* placeholderRow = new QHBoxLayout();
        placeholderRow->setSpacing(kContentPadding);
        for (int index = 0; index < 3; ++index)
        {
            ExampleMutedItem* placeholder = new ExampleMutedItem(content);
            placeholder->setFixedHeight(kPlaceholderHeight);
            placeholderRow->addWidget(placeholder, 1);
        }
        layout->addLayout(placeholderRow);
        layout->addWidget(new ExampleMutedItem(content), 1);
        return content;
    }
} // namespace

SidebarPage::SidebarPage(QWidget* parent)
    : ExampleBasePage([] { return tr("侧栏"); }, [] { return tr("ShadSidebar / ShadSidebarLayout：可组合、可收起的侧栏，包含页头、分组、菜单与页脚。"); }, parent)
{
    ShadCard* demoCard = addSection([] { return tr("迷你应用"); }, [] { return tr("可切换侧栏的变体、收起方式与位置"); });
    ShadToggleGroup* variantGroup = createChoiceGroup({QStringLiteral("Sidebar"), QStringLiteral("Floating"), QStringLiteral("Inset")}, this);
    ShadToggleGroup* collapsibleGroup = createChoiceGroup({QStringLiteral("Offcanvas"), QStringLiteral("Icon"), QStringLiteral("None")}, this);
    ShadToggleGroup* sideGroup = createChoiceGroup({QString(), QString()}, this);
    ExampleI18n::bind(sideGroup, [sideGroup]()
    {
        sideGroup->itemAt(0)->setText(tr("起始侧"));
        sideGroup->itemAt(1)->setText(tr("结束侧"));
    });
    demoCard->contentLayout()->addWidget(createChoiceRow([] { return tr("变体"); }, variantGroup, this));
    demoCard->contentLayout()->addWidget(createChoiceRow([] { return tr("收起方式"); }, collapsibleGroup, this));
    demoCard->contentLayout()->addWidget(createChoiceRow([] { return tr("位置"); }, sideGroup, this));

    ShadSidebarLayout* sidebarLayout = new ShadSidebarLayout(this);
    sidebarLayout->setFixedHeight(kDemoHeight);
    ShadSidebar* sidebar = createDemoSidebar(sidebarLayout);
    sidebarLayout->setSidebar(sidebar);
    ShadLabel* locationLabel = nullptr;
    sidebarLayout->setContentWidget(createDemoContent(sidebar, &locationLabel, sidebarLayout));
    demoCard->contentLayout()->addWidget(sidebarLayout);
    ShadLabel* stateLabel = createMutedText(this);
    demoCard->contentLayout()->addWidget(stateLabel);

    // 最近一次由用户点击的菜单按钮，为空时显示初始位置；菜单按钮的文字先于此处刷新（连接顺序在前）
    const std::shared_ptr<QPointer<ShadSidebarMenuButton>> activatedButton = std::make_shared<QPointer<ShadSidebarMenuButton>>();
    const auto updateLocationLabel = [locationLabel, activatedButton]()
    {
        const ShadSidebarMenuButton* button = activatedButton->data();
        locationLabel->setText(button != nullptr ? tr("构建你的应用 / %1").arg(button->text()) : tr("构建你的应用 / 历史记录"));
    };
    const auto updateStateLabel = [stateLabel, sidebar]() { stateLabel->setText(sidebar->isExpanded() ? tr("展开") : tr("收起")); };
    connect(sidebar, &ShadSidebar::itemActivated, locationLabel, [activatedButton, updateLocationLabel](ShadSidebarMenuButton* button)
    {
        *activatedButton = button;
        updateLocationLabel();
    });
    connect(sidebar, &ShadSidebar::expandedChanged, stateLabel, updateStateLabel);
    ExampleI18n::bind(locationLabel, updateLocationLabel);
    ExampleI18n::bind(stateLabel, updateStateLabel);
    // 单选切换组再次点击已选项会取消选中，这里恢复到当前设置
    connect(variantGroup, &ShadToggleGroup::currentIndexChanged, sidebar, [variantGroup, sidebar](int index)
    {
        if (index < 0)
        {
            variantGroup->setCurrentIndex(static_cast<int>(sidebar->getVariant()));
            return;
        }
        sidebar->setVariant(static_cast<ShadSidebarType::Variant>(index));
    });
    connect(collapsibleGroup, &ShadToggleGroup::currentIndexChanged, sidebar, [collapsibleGroup, sidebar](int index)
    {
        if (index < 0)
        {
            collapsibleGroup->setCurrentIndex(static_cast<int>(sidebar->getCollapsible()));
            return;
        }
        sidebar->setCollapsible(static_cast<ShadSidebarType::Collapsible>(index));
    });
    connect(sideGroup, &ShadToggleGroup::currentIndexChanged, sidebar, [sideGroup, sidebar](int index)
    {
        if (index < 0)
        {
            sideGroup->setCurrentIndex(static_cast<int>(sidebar->getSide()));
            return;
        }
        sidebar->setSide(static_cast<ShadSidebarType::Side>(index));
    });
}

SidebarPage::~SidebarPage()
{
}
