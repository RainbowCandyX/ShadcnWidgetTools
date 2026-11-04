#ifndef SHADMENUBAR_H
#define SHADMENUBAR_H

#include <QList>
#include <QString>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QKeyEvent;
class QObject;
class QPaintEvent;
class ShadDropdownMenu;

// 菜单栏，菜单内容为 ShadDropdownMenu
class ShadMenubarPrivate;

class SHADCN_EXPORT ShadMenubar : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadMenubar)
    Q_PROPERTY(bool nativeMenuBar READ isNativeMenuBar WRITE setNativeMenuBar)

    public:
        explicit ShadMenubar(QWidget* parent = nullptr);
        ~ShadMenubar() override;

        // 返回的菜单归菜单栏所有
        ShadDropdownMenu* addMenu(const QString& title);
        void removeMenu(ShadDropdownMenu* menu);
        // 修改 addMenu 返回的菜单在菜单栏上的标题，例如切换语言时；不属于本菜单栏的菜单不处理
        void setMenuTitle(ShadDropdownMenu* menu, const QString& title);
        QList<ShadDropdownMenu*> getMenus() const;

        // 只在 macOS 上生效：显示在系统菜单栏，本组件隐藏；系统菜单只显示 QAction::shortcut
        void setNativeMenuBar(bool native);
        bool isNativeMenuBar() const;

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADMENUBAR_H
