#include "MainWindow.h"

#include <QAction>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>

#include "AccordionPage.h"
#ifdef SHADCN_EXAMPLE_AI_CHAT
#include "AiChatPage.h"
#endif
#include "CollapsiblePage.h"
#include "ExpanderPage.h"
#include "AlertDialogPage.h"
#include "AlertPage.h"
#include "AttachmentPage.h"
#include "AvatarPage.h"
#include "BadgePage.h"
#include "BiometricPage.h"
#include "BreadcrumbPage.h"
#include "ButtonPage.h"
#include "CardPage.h"
#include "CarouselPage.h"
#include "ChartAreaPage.h"
#include "ChartBarPage.h"
#include "ChartCandlestickPage.h"
#include "ChartHeatmapPage.h"
#include "ChartLinePage.h"
#include "ChartScatterPage.h"
#include "CheckboxPage.h"
#include "CodeBlockPage.h"
#include "CodeEditorPage.h"
#include "ColorPickerPage.h"
#include "ComboBoxPage.h"
#include "CommandPage.h"
#include "ContextMenuPage.h"
#include "DataTablePage.h"
#include "DatePickerPage.h"
#include "DescriptionsPage.h"
#include "DialogPage.h"
#include "DockPage.h"
#include "DrawerPage.h"
#include "DropdownMenuPage.h"
#include "EmptyPage.h"
#include "FieldPage.h"
#include "ExampleBasePage.h"
#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "HomePage.h"
#include "HoverCardPage.h"
#include "ImageCropperPage.h"
#include "ImageViewerPage.h"
#include "InputDialogPage.h"
#include "InputGroupPage.h"
#include "InputOtpPage.h"
#include "InputPage.h"
#include "ItemPage.h"
#include "KbdPage.h"
#include "LabelPage.h"
#include "ListViewPage.h"
#include "MarkdownViewPage.h"
#include "MarkerPage.h"
#include "MenubarPage.h"
#include "NavigationMenuPage.h"
#include "NotificationPage.h"
#include "PaginationPage.h"
#include "PasskeyPage.h"
#include "PopconfirmPage.h"
#include "PopoverPage.h"
#include "ProgressPage.h"
#include "PropertyGridPage.h"
#include "QrCodePage.h"
#include "QuestionnairePage.h"
#include "RadialProgressPage.h"
#include "RadioGroupPage.h"
#include "RatingPage.h"
#include "RtlPage.h"
#include "ScreenshotPage.h"
#include "SegmentedPage.h"
#include "SeparatorPage.h"
#include "SettingsDialog.h"
#include "SidebarPage.h"
#include "SignaturePadPage.h"
#include "SpinBoxPage.h"
#include "SpinnerPage.h"
#include "StatisticPage.h"
#include "StatusBarPage.h"
#include "ShadAvatar.h"
#include "ShadBreadcrumb.h"
#include "ShadButton.h"
#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadItem.h"
#include "ShadLabel.h"
#include "ShadSeparator.h"
#include "ShadSidebar.h"
#include "ShadSidebarGroup.h"
#include "ShadSidebarLayout.h"
#include "ShadSidebarMenuButton.h"
#include "ShadSidebarTrigger.h"
#include "ShadStackedWidget.h"
#include "ShadSwitch.h"
#include "ShadTheme.h"
#include "ShadToaster.h"
#include "SwitchPage.h"
#include "TablePage.h"
#include "TabViewPage.h"
#include "TabsPage.h"
#include "ThemePage.h"
#include "ToastPage.h"
#include "TogglePage.h"
#include "ToolbarPage.h"
#include "TooltipPage.h"
#include "TracePage.h"
#include "TreeSelectPage.h"
#include "TreeViewPage.h"
#include "WatermarkPage.h"
#include "WidgetPage.h"

namespace
{
    constexpr int kSidebarWidth = 240;
    constexpr int kSidebarMinimumWidth = 170;
    constexpr int kSidebarMaximumWidth = 270;
    constexpr int kHeaderHeight = 56;
    // 最小宽度保证最宽的固定宽度组件不被裁剪，最小高度保证顶栏下方至少显示一个演示区块
    constexpr int kMinimumWidth = 800;
    constexpr int kMinimumHeight = 600;

    const QString kUserName = QStringLiteral("shadcn");
    const QString kUserEmail = QStringLiteral("m@example.com");
    const QString kUserInitials = QStringLiteral("CN");

    QPixmap createUserPhoto()
    {
        constexpr int kPhotoSize = 160;
        QPixmap photo(kPhotoSize, kPhotoSize);
        photo.fill(Qt::transparent);
        QPainter painter(&photo);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient gradient(QPointF(0, 0), QPointF(kPhotoSize, kPhotoSize));
        gradient.setColorAt(0.0, QColor(0x60, 0xa5, 0xfa));
        gradient.setColorAt(1.0, QColor(0x1e, 0x3a, 0x8a));
        painter.fillRect(photo.rect(), gradient);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 200));
        painter.drawEllipse(QRectF(kPhotoSize * 0.32, kPhotoSize * 0.2, kPhotoSize * 0.36, kPhotoSize * 0.36));
        painter.drawEllipse(QRectF(kPhotoSize * 0.14, kPhotoSize * 0.62, kPhotoSize * 0.72, kPhotoSize * 0.6));
        return photo;
    }

    ShadAvatar* createUserAvatar(QWidget* parent)
    {
        ShadAvatar* avatar = new ShadAvatar(kUserInitials, parent);
        avatar->setShape(ShadAvatarType::Rounded);
        avatar->setImage(createUserPhoto());
        return avatar;
    }

    QWidget* createUserMenuHeader(QWidget* parent)
    {
        constexpr int kAvatarMargin = 2;
        QWidget* header = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(header);
        layout->setContentsMargins(4 - kAvatarMargin, 6 - kAvatarMargin, 4 - kAvatarMargin, 6 - kAvatarMargin);
        layout->setSpacing(8 - kAvatarMargin);
        layout->addWidget(createUserAvatar(header));
        QVBoxLayout* textLayout = new QVBoxLayout();
        textLayout->setContentsMargins(0, 0, 0, 0);
        textLayout->setSpacing(0);
        textLayout->addWidget(new ShadLabel(kUserName, header));
        ShadLabel* emailLabel = new ShadLabel(kUserEmail, header);
        QFont emailFont = emailLabel->font();
        emailFont.setPixelSize(12);
        emailFont.setWeight(QFont::Normal);
        emailLabel->setFont(emailFont);
        textLayout->addWidget(emailLabel);
        layout->addLayout(textLayout, 1);
        return header;
    }
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : ShadWindow(parent)
{
    setMinimumSize(kMinimumWidth, kMinimumHeight);
    resize(1200, 820);
    ExampleLayout::addStayOnTopButton(this);

    QWidget* titleBarActions = new QWidget(this);
    QHBoxLayout* titleBarActionsLayout = new QHBoxLayout(titleBarActions);
    titleBarActionsLayout->setContentsMargins(0, 0, 0, 0);
    titleBarActionsLayout->setSpacing(8);
    _rtlSwitch = new ShadSwitch(QStringLiteral("RTL"), titleBarActions);
    _rtlSwitch->setChecked(QGuiApplication::layoutDirection() == Qt::RightToLeft);
    connect(_rtlSwitch, &ShadSwitch::toggled, this, [](bool checked)
    {
        QGuiApplication::setLayoutDirection(checked ? Qt::RightToLeft : Qt::LeftToRight);
    });
    titleBarActionsLayout->addWidget(_rtlSwitch);
    _followSystemSwitch = new ShadSwitch(titleBarActions);
    _followSystemSwitch->setChecked(shadTheme->isFollowSystemTheme());
    connect(_followSystemSwitch, &ShadSwitch::toggled, this, [](bool checked) { shadTheme->setFollowSystemTheme(checked); });
    connect(shadTheme, &ShadTheme::followSystemThemeChanged, _followSystemSwitch, &ShadSwitch::setChecked);
    _themeButton = new ShadButton(QString(), ShadButtonType::Ghost, titleBarActions);
    _themeButton->setButtonSize(ShadButtonType::IconSmall);
    titleBarActionsLayout->addWidget(_followSystemSwitch);
    titleBarActionsLayout->addWidget(_themeButton);
    setTitleBarWidget(ShadWindowType::RightArea, titleBarActions);

    _sidebarLayout = new ShadSidebarLayout(this);

    _sidebar = new ShadSidebar(_sidebarLayout);
    _sidebar->setExpandedWidthRange(kSidebarMinimumWidth, kSidebarMaximumWidth);
    _sidebar->setExpandedWidth(kSidebarWidth);
    _sidebar->setCollapsible(ShadSidebarType::Icon);
    _sidebar->setResizable(true);
    _sidebar->setSubMenuExclusive(true);
    // 点击回到首页，不占用选中项
    ShadSidebarMenuButton* brandButton = new ShadSidebarMenuButton(ShadIconType::Component, QStringLiteral("ShadcnWidgetTools"), _sidebar);
    brandButton->setButtonSize(ShadSidebarMenuButtonType::Large);
    brandButton->setDescription(QStringLiteral("v1.1"));
    brandButton->setSelectable(false);
    connect(brandButton, &ShadSidebarMenuButton::clicked, this, [this]()
    {
        _sidebar->setActiveButton(_homeButton);
        showPage(_homeButton);
    });
    _sidebar->headerLayout()->addWidget(brandButton);
    _sidebarLayout->setSidebar(_sidebar);

    QWidget* mainArea = new QWidget(_sidebarLayout);
    QVBoxLayout* mainAreaLayout = new QVBoxLayout(mainArea);
    mainAreaLayout->setContentsMargins(0, 0, 0, 0);
    mainAreaLayout->setSpacing(0);
    QWidget* header = new QWidget(mainArea);
    header->setFixedHeight(kHeaderHeight);
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    // 控件四周带焦点环边距，左右边距相应扣除
    headerLayout->setContentsMargins(16 - ExampleLayout::kFocusRingWidth, 0, 24 - ExampleLayout::kFocusRingWidth, 0);
    headerLayout->setSpacing(8);
    ShadSidebarTrigger* sidebarTrigger = new ShadSidebarTrigger(_sidebar, header);
    // 窗口标题与标题栏、顶栏中的提示随语言刷新
    ExampleI18n::bind(this, [this, sidebarTrigger]()
    {
        setWindowTitle(tr("ShadcnWidgetTools 示例"));
        _rtlSwitch->setToolTip(tr("切换从右到左布局"));
        _followSystemSwitch->setText(tr("跟随系统"));
        _themeButton->setToolTip(tr("切换明暗主题（会关闭跟随系统）"));
        sidebarTrigger->setToolTip(tr("切换侧栏（Ctrl+B / Cmd+B）"));
    });
    ShadSeparator* headerSeparator = new ShadSeparator(Qt::Vertical, header);
    headerSeparator->setFixedHeight(16);
    _headerBreadcrumb = new ShadBreadcrumb(header);
    connect(_headerBreadcrumb, &ShadBreadcrumb::linkClicked, this, [this](int index)
    {
        if (index == 0)
        {
            _sidebar->setActiveButton(_homeButton);
            showPage(_homeButton);
            return;
        }
        if (ShadSidebarMenuButton* category = _pageCategories.value(_pageStack->currentIndex(), nullptr))
        {
            category->setSubMenuExpanded(true);
        }
    });
    headerLayout->addWidget(sidebarTrigger);
    headerLayout->addWidget(headerSeparator);
    headerLayout->addWidget(_headerBreadcrumb);
    headerLayout->addStretch(1);
    _pageStack = new ShadStackedWidget(mainArea);
    mainAreaLayout->addWidget(header);
    mainAreaLayout->addWidget(new ShadSeparator(Qt::Horizontal, mainArea));
    mainAreaLayout->addWidget(_pageStack, 1);
    _sidebarLayout->setContentWidget(mainArea);

    QWidget* centralArea = new QWidget(this);
    QVBoxLayout* centralLayout = new QVBoxLayout(centralArea);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(new ShadSeparator(Qt::Horizontal, centralArea));
    centralLayout->addWidget(_sidebarLayout, 1);
    setCentralWidget(centralArea);

    // 主窗口的 toaster，需在使用它的页面之前创建
    _toaster = ShadToaster::forWindow(this);
    addUserMenu();

    ShadSidebarGroup* startGroup = _sidebar->addGroup();
    HomePage* homePage = new HomePage(_pageStack);
    _homeButton = addPage(startGroup, ShadIconType::House, homePage);
    addPage(startGroup, ShadIconType::Palette, new ThemePage(_pageStack));
    addPage(startGroup, ShadIconType::Languages, new RtlPage(_pageStack));
#ifdef SHADCN_EXAMPLE_AI_CHAT
    addPage(startGroup, ShadIconType::Sparkles, new AiChatPage(_pageStack));
#endif

    ShadSidebarGroup* componentGroup = _sidebar->addGroup();
    ExampleI18n::bind(componentGroup, [componentGroup]() { componentGroup->setLabel(tr("组件")); });
    ShadSidebarMenuButton* actionCategory = addCategory(componentGroup, ShadIconType::MousePointerClick, [] { return tr("按钮与操作"); });
    addCategoryPage(actionCategory, new ButtonPage(_toaster, _pageStack));
    addCategoryPage(actionCategory, new TogglePage(_pageStack));
    addCategoryPage(actionCategory, new SegmentedPage(_pageStack));
    addCategoryPage(actionCategory, new DropdownMenuPage(_pageStack));
    addCategoryPage(actionCategory, new MenubarPage(_pageStack));
    addCategoryPage(actionCategory, new ToolbarPage(_pageStack));
    addCategoryPage(actionCategory, new ContextMenuPage(_pageStack));
    addCategoryPage(actionCategory, new CommandPage(_pageStack));
    addCategoryPage(actionCategory, new StatusBarPage(_pageStack));

    ShadSidebarMenuButton* formCategory = addCategory(componentGroup, ShadIconType::FormInput, [] { return tr("数据录入"); });
    addCategoryPage(formCategory, new FieldPage(_pageStack));
    addCategoryPage(formCategory, new LabelPage(_pageStack));
    addCategoryPage(formCategory, new InputPage(_pageStack));
    addCategoryPage(formCategory, new InputGroupPage(_toaster, _pageStack));
    addCategoryPage(formCategory, new InputOtpPage(_toaster, _pageStack));
    addCategoryPage(formCategory, new PasskeyPage(_pageStack));
    addCategoryPage(formCategory, new BiometricPage(_pageStack));
    addCategoryPage(formCategory, new CodeEditorPage(_pageStack));
    addCategoryPage(formCategory, new SpinBoxPage(_pageStack));
    addCategoryPage(formCategory, new ComboBoxPage(_pageStack));
    addCategoryPage(formCategory, new TreeSelectPage(_pageStack));
    addCategoryPage(formCategory, new CheckboxPage(_pageStack));
    addCategoryPage(formCategory, new SwitchPage(_pageStack));
    addCategoryPage(formCategory, new RadioGroupPage(_pageStack));
    addCategoryPage(formCategory, new RatingPage(_pageStack));
    addCategoryPage(formCategory, new ColorPickerPage(_pageStack));
    addCategoryPage(formCategory, new DatePickerPage(_pageStack));
    addCategoryPage(formCategory, new AttachmentPage(_toaster, _pageStack));
    addCategoryPage(formCategory, new ImageCropperPage(_pageStack));
    addCategoryPage(formCategory, new SignaturePadPage(_pageStack));
    addCategoryPage(formCategory, new QuestionnairePage(_pageStack));

    ShadSidebarMenuButton* dataCategory = addCategory(componentGroup, ShadIconType::LayoutGrid, [] { return tr("数据展示"); });
    addCategoryPage(dataCategory, new CardPage(_pageStack));
    addCategoryPage(dataCategory, new AccordionPage(_pageStack));
    addCategoryPage(dataCategory, new CollapsiblePage(_pageStack));
    addCategoryPage(dataCategory, new ExpanderPage(_pageStack));
    addCategoryPage(dataCategory, new ItemPage(_pageStack));
    addCategoryPage(dataCategory, new ListViewPage(_pageStack));
    addCategoryPage(dataCategory, new CarouselPage(_pageStack));
    addCategoryPage(dataCategory, new ImageViewerPage(_pageStack));
    addCategoryPage(dataCategory, new AvatarPage(_pageStack));
    addCategoryPage(dataCategory, new BadgePage(_pageStack));
    addCategoryPage(dataCategory, new TablePage(_pageStack));
    addCategoryPage(dataCategory, new DataTablePage(_pageStack));
    addCategoryPage(dataCategory, new DescriptionsPage(_pageStack));
    addCategoryPage(dataCategory, new PropertyGridPage(_pageStack));
    addCategoryPage(dataCategory, new TreeViewPage(_pageStack));
    addCategoryPage(dataCategory, new ChartAreaPage(_pageStack));
    addCategoryPage(dataCategory, new ChartLinePage(_pageStack));
    addCategoryPage(dataCategory, new ChartBarPage(_pageStack));
    addCategoryPage(dataCategory, new ChartScatterPage(_pageStack));
    addCategoryPage(dataCategory, new ChartHeatmapPage(_pageStack));
    addCategoryPage(dataCategory, new ChartCandlestickPage(_pageStack));
    addCategoryPage(dataCategory, new StatisticPage(_pageStack));
    addCategoryPage(dataCategory, new CodeBlockPage(_pageStack));
    addCategoryPage(dataCategory, new MarkdownViewPage(_pageStack));
    addCategoryPage(dataCategory, new TracePage(_pageStack));
    addCategoryPage(dataCategory, new MarkerPage(_pageStack));
    addCategoryPage(dataCategory, new KbdPage(_toaster, _pageStack));
    addCategoryPage(dataCategory, new QrCodePage(_pageStack));
    addCategoryPage(dataCategory, new WatermarkPage(_pageStack));

    ShadSidebarMenuButton* feedbackCategory = addCategory(componentGroup, ShadIconType::Bell, [] { return tr("反馈"); });
    addCategoryPage(feedbackCategory, new AlertPage(_pageStack));
    addCategoryPage(feedbackCategory, new EmptyPage(_pageStack));
    addCategoryPage(feedbackCategory, new ProgressPage(_pageStack));
    addCategoryPage(feedbackCategory, new RadialProgressPage(_pageStack));
    addCategoryPage(feedbackCategory, new SpinnerPage(_pageStack));
    addCategoryPage(feedbackCategory, new ToastPage(_toaster, _pageStack));
    addCategoryPage(feedbackCategory, new NotificationPage(_pageStack));

    ShadSidebarMenuButton* overlayCategory = addCategory(componentGroup, ShadIconType::AppWindow, [] { return tr("浮层与窗口"); });
    addCategoryPage(overlayCategory, new PopoverPage(_pageStack));
    addCategoryPage(overlayCategory, new PopconfirmPage(_pageStack));
    addCategoryPage(overlayCategory, new TooltipPage(_pageStack));
    addCategoryPage(overlayCategory, new HoverCardPage(_pageStack));
    addCategoryPage(overlayCategory, new DialogPage(_pageStack));
    addCategoryPage(overlayCategory, new AlertDialogPage(_pageStack));
    addCategoryPage(overlayCategory, new InputDialogPage(_pageStack));
    addCategoryPage(overlayCategory, new DrawerPage(_pageStack));
    addCategoryPage(overlayCategory, new WidgetPage(_pageStack));
    addCategoryPage(overlayCategory, new ScreenshotPage(_pageStack));

    ShadSidebarMenuButton* navigationCategory = addCategory(componentGroup, ShadIconType::Compass, [] { return tr("导航与布局"); });
    addCategoryPage(navigationCategory, new NavigationMenuPage(_pageStack));
    addCategoryPage(navigationCategory, new BreadcrumbPage(_toaster, _pageStack));
    addCategoryPage(navigationCategory, new PaginationPage(_pageStack));
    addCategoryPage(navigationCategory, new TabsPage(_pageStack));
    addCategoryPage(navigationCategory, new TabViewPage(_pageStack));
    addCategoryPage(navigationCategory, new DockPage(_pageStack));
    SidebarPage* sidebarPage = new SidebarPage(_pageStack);
    _sidebarDemoPage = sidebarPage;
    addCategoryPage(navigationCategory, sidebarPage);
    addCategoryPage(navigationCategory, new SeparatorPage(_pageStack));

    for (ShadSidebarMenuButton* category : _categories)
    {
        const QList<ShadSidebarMenuButton*> pageButtons = _categoryPageButtons.value(category);
        QList<ExampleBasePage*> pages;
        for (ShadSidebarMenuButton* pageButton : pageButtons)
        {
            pages.append(qobject_cast<ExampleBasePage*>(_pageStack->widget(_pageIndexes.value(pageButton))));
        }
        // 切换语言时首页卡片重新读取：分类按钮与页面标题都在此之前绑定，读到的已是新语言
        const auto pageTitles = [pages]()
        {
            QStringList titles;
            for (const ExampleBasePage* page : pages)
            {
                if (page != nullptr)
                {
                    titles.append(page->getTitle());
                }
            }
            return titles;
        };
        ShadItem* categoryItem = homePage->addCategoryItem(category->getShadIcon(), [category]() { return category->text(); }, pageTitles);
        connect(categoryItem, &ShadItem::clicked, this, [this, category, pageButtons]()
        {
            if (pageButtons.isEmpty())
            {
                return;
            }
            category->setSubMenuExpanded(true);
            _sidebar->setActiveButton(pageButtons.first());
            showPage(pageButtons.first());
        });
    }

    connect(_sidebar, &ShadSidebar::itemActivated, this, [this](ShadSidebarMenuButton* button) { showPage(button); });
    connect(_themeButton, &ShadButton::clicked, this, []()
    {
        shadTheme->setThemeMode(shadTheme->isDark() ? ShadThemeType::Light : ShadThemeType::Dark);
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { updateThemeButton(); });

    _sidebar->setActiveButton(_homeButton);
    showPage(_homeButton);
    updateThemeButton();
    // 在页面与分类之后绑定：切换语言时它们的文字先刷新，面包屑再按新文字重建
    ExampleI18n::bind(this, [this]() { updateBreadcrumb(); });
}

MainWindow::~MainWindow()
{
}

void MainWindow::changeEvent(QEvent* event)
{
    // 布局方向会被设置对话框改变，同步 RTL 开关
    if (event->type() == QEvent::LayoutDirectionChange && _rtlSwitch != nullptr)
    {
        _rtlSwitch->setChecked(layoutDirection() == Qt::RightToLeft);
    }
    ShadWindow::changeEvent(event);
}

ShadSidebarMenuButton* MainWindow::addPage(ShadSidebarGroup* group, ShadIconType::IconName icon, ExampleBasePage* page)
{
    const int index = _pageStack->addWidget(page);
    ShadSidebarMenuButton* button = group->addItem(icon, page->getTitle());
    _pageIndexes.insert(button, index);
    // 页面标题随语言刷新后同步侧栏按钮
    connect(page, &ExampleBasePage::titleChanged, button, &ShadSidebarMenuButton::setText);
    return button;
}

ShadSidebarMenuButton* MainWindow::addCategory(ShadSidebarGroup* group, ShadIconType::IconName icon, ExampleI18n::TextFunction name)
{
    // 有子菜单的按钮点击时只展开 / 折叠子菜单
    ShadSidebarMenuButton* category = group->addItem(icon, QString());
    ExampleI18n::bind(category, [category, name]() { category->setText(name()); });
    _categories.append(category);
    return category;
}

void MainWindow::addCategoryPage(ShadSidebarMenuButton* category, ExampleBasePage* page)
{
    const int index = _pageStack->addWidget(page);
    ShadSidebarMenuButton* button = category->addSubItem(page->getTitle());
    _pageIndexes.insert(button, index);
    _pageCategories.insert(index, category);
    _categoryPageButtons[category].append(button);
    connect(page, &ExampleBasePage::titleChanged, button, &ShadSidebarMenuButton::setText);
}

void MainWindow::addUserMenu()
{
    ShadSidebarMenuButton* userButton = new ShadSidebarMenuButton(_sidebar);
    userButton->setText(kUserName);
    userButton->setDescription(kUserEmail);
    userButton->setButtonSize(ShadSidebarMenuButtonType::Large);
    userButton->setAvatar(createUserAvatar(userButton));
    userButton->setActionIcon(ShadIconType::ChevronsUpDown);
    userButton->setActionAlwaysVisible(true);
    // 只打开菜单，不改变选中页面
    userButton->setSelectable(false);
    _sidebar->footerLayout()->addWidget(userButton);

    ShadDropdownMenu* userMenu = new ShadDropdownMenu(this);
    userMenu->setMatchAnchorWidth(true);
    userMenu->addWidget(createUserMenuHeader(userMenu));
    userMenu->addSeparator();
    QAction* upgradeAction = userMenu->addItem(QString(), ShadIconType::Sparkles);
    userMenu->addSeparator();
    QAction* accountAction = userMenu->addItem(QString(), ShadIconType::BadgeCheck);
    QAction* billingAction = userMenu->addItem(QString(), ShadIconType::CreditCard);
    QAction* notificationAction = userMenu->addItem(QString(), ShadIconType::Bell);
    QAction* settingsAction = userMenu->addItem(QString(), ShadIconType::Settings);
    userMenu->addSeparator();
    QAction* logOutAction = userMenu->addItem(QString(), ShadIconType::LogOut);
    ExampleI18n::bind(userMenu, [upgradeAction, accountAction, billingAction, notificationAction, settingsAction, logOutAction]()
    {
        upgradeAction->setText(tr("升级到 Pro"));
        accountAction->setText(tr("账户"));
        billingAction->setText(tr("账单"));
        notificationAction->setText(tr("通知"));
        settingsAction->setText(tr("设置"));
        logOutAction->setText(tr("退出登录"));
    });
    connect(settingsAction, &QAction::triggered, this, [this]()
    {
        ExampleLayout::openTransientDialog(new SettingsDialog(_sidebar, _toaster, this));
    });
    connect(userMenu, &ShadDropdownMenu::itemTriggered, this, [this, settingsAction](QAction* action)
    {
        if (action != settingsAction)
        {
            _toaster->add(tr("已选择：%1").arg(action->text()));
        }
    });
    connect(userMenu, &ShadDropdownMenu::aboutToHide, userButton, [userButton]() { userButton->setOpen(false); });

    const auto openUserMenu = [this, userButton, userMenu]()
    {
        userButton->setOpen(true);
        const bool isExpanded = _sidebar->isExpanded();
        userMenu->popup(userButton, isExpanded ? ShadPlacementType::Top : ShadPlacementType::Right, isExpanded ? ShadPlacementType::Start : ShadPlacementType::End);
    };
    connect(userButton, &ShadSidebarMenuButton::clicked, userMenu, openUserMenu);
    connect(userButton, &ShadSidebarMenuButton::actionClicked, userMenu, openUserMenu);
}

void MainWindow::showPage(ShadSidebarMenuButton* button)
{
    const int index = _pageIndexes.value(button, -1);
    if (index < 0)
    {
        return;
    }
    _pageStack->setCurrentIndex(index);
    // 侧栏页自带的演示也响应 Ctrl+B，两个快捷键同时生效都不会触发，显示该页时关闭主窗口的快捷键
    _sidebarLayout->setShortcutEnabled(_pageStack->currentWidget() != _sidebarDemoPage);
    updateBreadcrumb();
}

void MainWindow::updateBreadcrumb()
{
    const int index = _pageStack->currentIndex();
    ExampleBasePage* page = qobject_cast<ExampleBasePage*>(_pageStack->widget(index));
    if (page == nullptr)
    {
        return;
    }
    QStringList path{tr("首页")};
    if (_pageIndexes.value(_homeButton, -1) == index)
    {
        _headerBreadcrumb->setPath(path);
        return;
    }
    if (ShadSidebarMenuButton* category = _pageCategories.value(index, nullptr))
    {
        path.append(category->text());
    }
    path.append(page->getTitle());
    _headerBreadcrumb->setPath(path);
}

void MainWindow::updateThemeButton()
{
    _themeButton->setShadIcon(shadTheme->isDark() ? ShadIconType::Sun : ShadIconType::Moon);
}
