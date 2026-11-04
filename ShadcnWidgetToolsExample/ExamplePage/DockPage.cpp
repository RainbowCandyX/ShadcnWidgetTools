#include "DockPage.h"

#include <QAction>
#include <QByteArray>
#include <QCoreApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QObject>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadDockManager.h"
#include "ShadDockPanel.h"
#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadWidget.h"

namespace
{
    constexpr int kViewHeight = 420;
    constexpr int kPagePadding = 16;
    constexpr int kPageGap = 8;
    constexpr int kWindowWidth = 1100;
    constexpr int kWindowHeight = 700;
    constexpr int kToolbarGap = 4;

    QWidget* createContent(ExampleI18n::TextFunction title, ExampleI18n::TextFunction content)
    {
        QWidget* page = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(page);
        layout->setContentsMargins(kPagePadding, kPagePadding, kPagePadding, kPagePadding);
        layout->setSpacing(kPageGap);
        ShadLabel* heading = new ShadLabel(page);
        QFont headingFont = heading->font();
        headingFont.setPixelSize(16);
        headingFont.setWeight(QFont::DemiBold);
        heading->setFont(headingFont);
        ShadLabel* description = new ShadLabel(page);
        description->setColorRole(ShadThemeType::MutedForeground);
        description->setWordWrap(true);
        ExampleI18n::bind(page, [heading, description, title, content]()
        {
            heading->setText(title());
            description->setText(content());
        });
        layout->addWidget(heading);
        layout->addWidget(description);
        layout->addStretch(1);
        return page;
    }

    // 面板标题与内容随语言原地刷新，停靠布局不变
    ShadDockPanel* createPanel(const QString& id, ShadIconType::IconName icon, ExampleI18n::TextFunction title, ExampleI18n::TextFunction content)
    {
        ShadDockPanel* panel = new ShadDockPanel(id, icon, QString(), createContent(title, content));
        ExampleI18n::bind(panel, [panel, title]() { panel->setTitle(title()); });
        return panel;
    }

    void addDocument(ShadDockManager* manager, const QString& id, ShadIconType::IconName icon, ExampleI18n::TextFunction title, ExampleI18n::TextFunction content)
    {
        ShadDockPanel* panel = createPanel(id, icon, title, content);
        // 关闭的文档直接删除
        QObject::connect(panel, &ShadDockPanel::closed, panel, &QObject::deleteLater);
        manager->addDocument(panel);
    }

    ShadDockManager* createDockManager(QWidget* parent)
    {
        ShadDockManager* manager = new ShadDockManager(parent);
        ShadDockPanel* explorer = createPanel(QStringLiteral("explorer"), ShadIconType::Folder, [] { return QCoreApplication::translate("DockPage", "资源管理器"); }, [] { return QCoreApplication::translate("DockPage", "项目文件与目录结构。"); });
        ShadDockPanel* search = createPanel(QStringLiteral("search"), ShadIconType::Search, [] { return QCoreApplication::translate("DockPage", "搜索"); }, [] { return QCoreApplication::translate("DockPage", "在整个项目中查找文本。"); });
        ShadDockPanel* properties = createPanel(QStringLiteral("properties"), ShadIconType::SlidersHorizontal, [] { return QCoreApplication::translate("DockPage", "属性"); }, [] { return QCoreApplication::translate("DockPage", "不可关闭，也不能浮动。"); });
        properties->setPanelFlags(ShadDockType::PanelNotClosable | ShadDockType::PanelNotFloatable);
        ShadDockPanel* output = createPanel(QStringLiteral("output"), ShadIconType::SquareTerminal, [] { return QCoreApplication::translate("DockPage", "输出"); }, [] { return QCoreApplication::translate("DockPage", "构建与运行日志。"); });
        ShadDockPanel* problems = createPanel(QStringLiteral("problems"), ShadIconType::TriangleAlert, [] { return QCoreApplication::translate("DockPage", "问题"); }, [] { return QCoreApplication::translate("DockPage", "编译错误与警告。"); });
        manager->addPanel(explorer, ShadDockType::LeftArea);
        manager->tabifyPanel(search, explorer);
        manager->addPanel(properties, ShadDockType::RightArea);
        manager->addPanel(output, ShadDockType::BottomArea);
        manager->tabifyPanel(problems, output);
        addDocument(manager, QStringLiteral("readme"), ShadIconType::FileText, [] { return QStringLiteral("README.md"); }, [] { return QCoreApplication::translate("DockPage", "项目说明：构建方式、组件列表与许可证。"); });
        addDocument(manager, QStringLiteral("main-window"), ShadIconType::FileCode, [] { return QStringLiteral("MainWindow.cpp"); }, [] { return QCoreApplication::translate("DockPage", "示例程序的主窗口。"); });
        addDocument(manager, QStringLiteral("presets"), ShadIconType::FileJson, [] { return QStringLiteral("CMakePresets.json"); }, [] { return QCoreApplication::translate("DockPage", "各平台的构建预设。"); });
        return manager;
    }

    // Ghost 样式的工具栏按钮，文字随语言刷新
    ShadButton* createToolbarButton(ExampleI18n::TextFunction text, QWidget* parent)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Ghost, parent);
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
        return button;
    }
} // namespace

DockPage::DockPage(QWidget* parent)
    : ExampleBasePage([] { return tr("停靠布局"); }, [] { return tr("ShadDockManager：可拖动停靠、合并与浮动的多面板布局。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("拖动标签或面板标题到指引上停靠，在别处松开变为浮动窗口"); });
    ShadDockManager* basicManager = createDockManager(this);
    basicManager->setFixedHeight(kViewHeight);
    addStretchItem(basicCard, basicManager);

    ShadCard* windowCard = addSection([] { return tr("独立窗口"); }, [] { return tr("保存与恢复布局，从视图菜单显示或关闭面板"); });
    ShadButton* windowButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(windowButton, [windowButton]() { windowButton->setText(tr("打开窗口")); });
    connect(windowButton, &ShadButton::clicked, this, [this]() { ExampleLayout::presentTransientWindow(_window, [this]() { return createDockWindow(); }); });
    addItem(windowCard, windowButton);
}

DockPage::~DockPage()
{
    delete _window.data();
}

ShadWidget* DockPage::createDockWindow()
{
    ShadWidget* window = new ShadWidget();
    window->setAttribute(Qt::WA_QuitOnClose, false);
    ExampleI18n::bind(window, [window]() { window->setWindowTitle(tr("停靠布局")); });
    window->resize(kWindowWidth, kWindowHeight);
    ExampleLayout::addStayOnTopButton(window);
    ShadDockManager* manager = createDockManager(window);
    const QByteArray defaultLayout = manager->saveState();
    QVBoxLayout* layout = new QVBoxLayout(window);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(manager);

    QWidget* toolbar = new QWidget(window);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(kToolbarGap);
    ShadButton* viewButton = createToolbarButton([] { return tr("视图"); }, toolbar);
    ShadDropdownMenu* viewMenu = new ShadDropdownMenu(viewButton);
    for (ShadDockPanel* panel : manager->panels())
    {
        if (panel->isDocument())
        {
            continue;
        }
        // 与 toggleViewAction 同步；面板标题随语言变化时同步菜单项文字
        QAction* item = viewMenu->addCheckableItem(panel->getTitle(), panel->isOpen());
        connect(panel, &ShadDockPanel::titleChanged, item, &QAction::setText);
        QAction* viewAction = panel->toggleViewAction();
        connect(item, &QAction::toggled, viewAction, [viewAction](bool checked)
        {
            if (viewAction->isChecked() != checked)
            {
                viewAction->trigger();
            }
        });
        connect(viewAction, &QAction::toggled, item, &QAction::setChecked);
    }
    connect(viewButton, &ShadButton::clicked, viewMenu, [viewMenu, viewButton]() { viewMenu->popup(viewButton); });
    ShadButton* newButton = createToolbarButton([] { return tr("新建文档"); }, toolbar);
    connect(newButton, &ShadButton::clicked, manager, [manager, nextNumber = 1]() mutable
    {
        const int number = nextNumber;
        addDocument(manager, QStringLiteral("untitled-%1").arg(number), ShadIconType::FileText, [number] { return tr("未命名 %1").arg(number); }, [] { return tr("新建的空白文档。"); });
        ++nextNumber;
    });
    ShadButton* saveButton = createToolbarButton([] { return tr("保存布局"); }, toolbar);
    connect(saveButton, &ShadButton::clicked, this, [this, manager]() { _savedLayout = manager->saveState(); });
    ShadButton* restoreButton = createToolbarButton([] { return tr("恢复布局"); }, toolbar);
    connect(restoreButton, &ShadButton::clicked, this, [this, manager]() { manager->restoreState(_savedLayout); });
    ShadButton* defaultButton = createToolbarButton([] { return tr("默认布局"); }, toolbar);
    connect(defaultButton, &ShadButton::clicked, manager, [manager, defaultLayout]() { manager->restoreState(defaultLayout); });
    toolbarLayout->addWidget(viewButton);
    toolbarLayout->addWidget(newButton);
    toolbarLayout->addWidget(saveButton);
    toolbarLayout->addWidget(restoreButton);
    toolbarLayout->addWidget(defaultButton);
    window->setTitleBarWidget(ShadWindowType::RightArea, toolbar);
    return window;
}
