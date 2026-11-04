#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHash>
#include <QList>
#include <QString>
#include <QWidget>

#include "ExampleI18n.h"
#include "ShadIconType.h"
#include "ShadWindow.h"

class ExampleBasePage;
class QEvent;
class ShadBreadcrumb;
class ShadButton;
class ShadSidebar;
class ShadSidebarGroup;
class ShadSidebarLayout;
class ShadSidebarMenuButton;
class ShadStackedWidget;
class ShadSwitch;
class ShadToaster;

// 示例主窗口
class MainWindow : public ShadWindow
{
    Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

    protected:
        void changeEvent(QEvent* event) override;

    private:
        ShadSidebarMenuButton* addPage(ShadSidebarGroup* group, ShadIconType::IconName icon, ExampleBasePage* page);
        // name 返回当前语言的分类名，切换语言时重新调用
        ShadSidebarMenuButton* addCategory(ShadSidebarGroup* group, ShadIconType::IconName icon, ExampleI18n::TextFunction name);
        void addCategoryPage(ShadSidebarMenuButton* category, ExampleBasePage* page);
        void addUserMenu();
        void showPage(ShadSidebarMenuButton* button);
        // 按当前页面重新生成顶栏面包屑，切换语言后也调用
        void updateBreadcrumb();
        void updateThemeButton();

        ShadSidebarLayout* _sidebarLayout{nullptr};
        ShadSidebar* _sidebar{nullptr};
        ShadSidebarMenuButton* _homeButton{nullptr};
        QHash<ShadSidebarMenuButton*, int> _pageIndexes;
        // 直接放在分组中的页面没有分类按钮
        QHash<int, ShadSidebarMenuButton*> _pageCategories;
        QList<ShadSidebarMenuButton*> _categories;
        QHash<ShadSidebarMenuButton*, QList<ShadSidebarMenuButton*>> _categoryPageButtons;
        ShadStackedWidget* _pageStack{nullptr};
        QWidget* _sidebarDemoPage{nullptr};
        ShadBreadcrumb* _headerBreadcrumb{nullptr};
        ShadButton* _themeButton{nullptr};
        ShadSwitch* _followSystemSwitch{nullptr};
        ShadSwitch* _rtlSwitch{nullptr};
        ShadToaster* _toaster{nullptr};
};

#endif // MAINWINDOW_H
