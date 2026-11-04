#ifndef SHADNAVIGATIONMENU_H
#define SHADNAVIGATIONMENU_H

#include <QString>
#include <QWidget>

#include "ShadProperty.h"

class QAction;
class QEvent;
class QKeyEvent;
class QObject;

// 导航菜单
class ShadNavigationMenuPrivate;

class SHADCN_EXPORT ShadNavigationMenu : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadNavigationMenu)

    public:
        explicit ShadNavigationMenu(QWidget* parent = nullptr);
        ~ShadNavigationMenu() override;

        // content 归菜单所有
        void addMenu(const QString& title, QWidget* content);
        // 修改 addMenu 添加的菜单标题，content 为添加时传入的内容组件，例如切换语言时；找不到时不处理
        // 链接的标题跟随 addLink 返回的 QAction 文字
        void setMenuTitle(QWidget* content, const QString& title);
        // 没有下拉内容的链接，返回的 QAction 归菜单所有
        QAction* addLink(const QString& title);
        void closeMenu();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
};

#endif // SHADNAVIGATIONMENU_H
