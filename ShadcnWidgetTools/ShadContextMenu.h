#ifndef SHADCONTEXTMENU_H
#define SHADCONTEXTMENU_H

#include <QPoint>

#include "ShadDropdownMenu.h"
#include "ShadProperty.h"

class QEvent;
class QObject;
class QWidget;

// 右键菜单
class ShadContextMenuPrivate;

class SHADCN_EXPORT ShadContextMenu : public ShadDropdownMenu
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadContextMenu)

    public:
        explicit ShadContextMenu(QWidget* parent = nullptr);
        ~ShadContextMenu() override;

        // 目标不归菜单所有
        void attachTo(QWidget* target);
        void detachFrom(QWidget* target);

    Q_SIGNALS:
        // 弹出前发出，pos 为目标组件坐标
        void aboutToPopup(QWidget* target, const QPoint& pos);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // SHADCONTEXTMENU_H
