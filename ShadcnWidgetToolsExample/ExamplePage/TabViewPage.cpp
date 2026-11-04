#include "TabViewPage.h"

#include <QCoreApplication>
#include <QFont>
#include <QList>
#include <QObject>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ExampleI18n.h"
#include "ExampleLayout.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadLabel.h"
#include "ShadTabView.h"
#include "ShadWidget.h"

namespace
{
    constexpr int kViewHeight = 240;
    constexpr int kPagePadding = 16;
    constexpr int kPageGap = 8;
    constexpr int kManyTabCount = 20;
    constexpr int kWindowWidth = 800;
    constexpr int kWindowHeight = 500;
    constexpr int kDetachedWidth = 640;
    constexpr int kDetachedHeight = 400;

    // 页面当前所在的标签视图；标签可能被拖动排序、拆分到新窗口或合并到其他视图，因此每次按页面查找
    ShadTabView* findTabView(QWidget* page)
    {
        for (QWidget* ancestor = page->parentWidget(); ancestor != nullptr; ancestor = ancestor->parentWidget())
        {
            if (ShadTabView* view = qobject_cast<ShadTabView*>(ancestor))
            {
                return view;
            }
        }
        return nullptr;
    }

    // 刷新页面所在标签的文字，页面尚未加入标签视图时跳过
    void updateTabText(QWidget* page, const QString& text)
    {
        ShadTabView* view = findTabView(page);
        if (view == nullptr)
        {
            return;
        }
        const int index = view->indexOf(page);
        if (index >= 0)
        {
            view->setTabText(index, text);
        }
    }

    // 标签页内容：标题与说明随语言刷新；syncTabText 为 true 时标题同时作为该页的标签文字刷新
    QWidget* createPage(const ExampleI18n::TextFunction& title, const ExampleI18n::TextFunction& content, bool syncTabText, QWidget* parent)
    {
        QWidget* page = new QWidget(parent);
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
        ExampleI18n::bind(page, [page, heading, description, title, content, syncTabText]()
        {
            const QString titleText = title();
            heading->setText(titleText);
            description->setText(content());
            if (syncTabText)
            {
                updateTabText(page, titleText);
            }
        });
        layout->addWidget(heading);
        layout->addWidget(description);
        layout->addStretch(1);
        return page;
    }

    // 添加标签，标签文字随语言刷新
    int addTranslatedTab(ShadTabView* view, ShadIconType::IconName icon, const ExampleI18n::TextFunction& title, const ExampleI18n::TextFunction& content)
    {
        return view->addTab(icon, title(), createPage(title, content, true, view));
    }

    ShadTabView* createDocumentView(QWidget* parent)
    {
        struct Document
        {
            ShadIconType::IconName icon;
            QString name;
            ExampleI18n::TextFunction content;
        };
        const QList<Document> documents{
            {ShadIconType::FileText, QStringLiteral("README.md"), [] { return QCoreApplication::translate("TabViewPage", "项目说明：构建方式、组件列表与许可证。"); }},
            {ShadIconType::FileCode, QStringLiteral("MainWindow.cpp"), [] { return QCoreApplication::translate("TabViewPage", "示例程序的主窗口，左侧侧栏按分类导航到各个演示页。"); }},
            {ShadIconType::FileJson, QStringLiteral("CMakePresets.json"), [] { return QCoreApplication::translate("TabViewPage", "各平台的构建预设，使用 Ninja 生成器。"); }},
        };
        ShadTabView* view = new ShadTabView(parent);
        for (const Document& document : documents)
        {
            const QString name = document.name;
            view->addTab(document.icon, name, createPage([name] { return name; }, document.content, false, view));
        }
        return view;
    }

    void connectTabActions(ShadTabView* view)
    {
        QObject::connect(view, &ShadTabView::newTabRequested, view, [view, nextNumber = 1]() mutable
        {
            const int number = nextNumber++;
            const int index = addTranslatedTab(view, ShadIconType::Globe, [number] { return QCoreApplication::translate("TabViewPage", "新标签页 %1").arg(number); }, [] { return QCoreApplication::translate("TabViewPage", "新建的空白页面。"); });
            view->setCurrentIndex(index);
        });
        QObject::connect(view, &ShadTabView::tabCloseRequested, view, [view](int index)
        {
            QWidget* page = view->widget(index);
            view->removeTab(index);
            page->deleteLater();
        });
    }

    ShadTabView* createTabWindow(bool hasDocuments);

    void enableDetach(ShadTabView* view)
    {
        view->setDragGroup(QStringLiteral("tab-view-example"));
        view->setDetachHandler([](ShadTabView* source)
        {
            Q_UNUSED(source);
            ShadTabView* detached = createTabWindow(false);
            detached->window()->resize(kDetachedWidth, kDetachedHeight);
            detached->window()->setAttribute(Qt::WA_DeleteOnClose);
            return detached;
        });
    }

    // 标签栏在标题栏中的窗口
    ShadTabView* createTabWindow(bool hasDocuments)
    {
        ShadWidget* window = new ShadWidget();
        window->setAttribute(Qt::WA_QuitOnClose, false);
        window->resize(kWindowWidth, kWindowHeight);
        ExampleLayout::addStayOnTopButton(window);
        ShadTabView* view = hasDocuments ? createDocumentView(window) : new ShadTabView(window);
        connectTabActions(view);
        enableDetach(view);
        // 标签全部关闭或拖走后关闭窗口
        QObject::connect(view, &ShadTabView::tabCloseRequested, window, [view, window]()
        {
            if (view->count() == 0)
            {
                window->close();
            }
        });
        QObject::connect(view, &ShadTabView::lastTabMovedOut, window, &ShadWidget::close);
        QVBoxLayout* layout = new QVBoxLayout(window);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(view);
        window->setTitleBarWidget(ShadWindowType::LeftArea, view->tabBar());
        return view;
    }
} // namespace

TabViewPage::TabViewPage(QWidget* parent)
    : ExampleBasePage([] { return tr("标签视图"); }, [] { return tr("ShadTabView：多标签视图，每个标签对应一个页面。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("新建、关闭标签，拖动调整顺序"); });
    ShadTabView* basicView = createDocumentView(this);
    basicView->setFixedHeight(kViewHeight);
    connectTabActions(basicView);
    addStretchItem(basicCard, basicView);

    ShadCard* detachCard = addSection([] { return tr("拆分与合并"); }, [] { return tr("setDragGroup：拖出标签栏拆分为新窗口，拖到同组标签栏上合并"); });
    ShadTabView* detachView = createDocumentView(this);
    detachView->setFixedHeight(kViewHeight);
    connectTabActions(detachView);
    enableDetach(detachView);
    addStretchItem(detachCard, detachView);

    ShadCard* propertyCard = addSection([] { return tr("单个标签属性"); }, [] { return tr("setTabFlags：首页不可关闭、不可拖动，归档禁用"); });
    ShadTabView* propertyView = createDocumentView(this);
    propertyView->setFixedHeight(kViewHeight);
    const ExampleI18n::TextFunction homeTitle = [] { return tr("首页"); };
    const int homeIndex = propertyView->insertTab(0, ShadIconType::House, homeTitle(), createPage(homeTitle, [] { return tr("不可关闭，不可拖动。"); }, true, propertyView));
    propertyView->setTabFlags(homeIndex, ShadTabViewType::TabNotClosable | ShadTabViewType::TabNotMovable);
    propertyView->setCurrentIndex(homeIndex);
    const int archiveIndex = addTranslatedTab(propertyView, ShadIconType::Archive, [] { return tr("归档"); }, [] { return tr("禁用的标签不能选中。"); });
    propertyView->setTabFlag(archiveIndex, ShadTabViewType::TabDisabled);
    // 提示属于 README.md 页面，按页面查找当前所在的标签
    QWidget* readmePage = propertyView->widget(homeIndex + 1);
    ExampleI18n::bind(readmePage, [readmePage]()
    {
        ShadTabView* view = findTabView(readmePage);
        const int index = view != nullptr ? view->indexOf(readmePage) : -1;
        if (index >= 0)
        {
            view->setTabToolTip(index, tr("项目根目录 / README.md"));
        }
    });
    connectTabActions(propertyView);
    enableDetach(propertyView);
    addStretchItem(propertyCard, propertyView);

    ShadCard* manyCard = addSection([] { return tr("大量标签"); }, [] { return tr("放不下时用滚轮横向滚动"); });
    ShadTabView* manyView = new ShadTabView(this);
    for (int number = 1; number <= kManyTabCount; ++number)
    {
        addTranslatedTab(manyView, ShadIconType::Globe, [number] { return tr("标签页 %1").arg(number); }, [number] { return tr("第 %1 个页面。").arg(number); });
    }
    manyView->setFixedHeight(kViewHeight);
    connectTabActions(manyView);
    addStretchItem(manyCard, manyView);

    ShadCard* fixedCard = addSection([] { return tr("固定标签"); }, [] { return tr("隐藏关闭与新建按钮"); });
    ShadTabView* fixedView = createDocumentView(this);
    fixedView->setFixedHeight(kViewHeight);
    fixedView->setTabsClosable(false);
    fixedView->setAddButtonVisible(false);
    addStretchItem(fixedCard, fixedView);

    ShadCard* windowCard = addSection([] { return tr("标题栏中的标签"); }, [] { return tr("把标签栏放进窗口标题栏"); });
    ShadButton* windowButton = new ShadButton(QString(), ShadButtonType::Outline, this);
    ExampleI18n::bind(windowButton, [windowButton]() { windowButton->setText(tr("打开窗口")); });
    connect(windowButton, &ShadButton::clicked, this, [this]() { ExampleLayout::presentTransientWindow(_titleBarWindow, []() { return static_cast<ShadWidget*>(createTabWindow(true)->window()); }); });
    addItem(windowCard, windowButton);
}

TabViewPage::~TabViewPage()
{
    delete _titleBarWindow.data();
}
