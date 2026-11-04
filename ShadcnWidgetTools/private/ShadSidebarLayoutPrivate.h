#ifndef SHADSIDEBARLAYOUTPRIVATE_H
#define SHADSIDEBARLAYOUTPRIVATE_H

#include <QMetaObject>
#include <QPointer>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QHBoxLayout;
class QObject;
class QPaintEvent;
class QShortcut;
class QVBoxLayout;
class ShadSidebar;
class ShadSidebarLayout;

// 主内容区：普通变体铺满 bg-background；Inset 变体在内容区四周留出边距，并在边距内绘制 rounded-xl shadow-sm 的卡片
class ShadSidebarInsetArea : public QWidget
{
    public:
        explicit ShadSidebarInsetArea(QWidget* parent = nullptr);

        void setInset(bool inset);
        bool isInset() const;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        bool _isInset{false};
};

// 继承 QObject 只为过滤侧栏的尺寸变化，不需要信号与槽，因此没有 Q_OBJECT
class ShadSidebarLayoutPrivate : public QObject
{
    SHAD_D_CREATE(ShadSidebarLayout)

    public:
        ShadSidebarLayoutPrivate();
        ~ShadSidebarLayoutPrivate() override;

        // m-2
        static constexpr int kInsetMargin = 8;

        // 按侧栏的位置重新排列侧栏与主内容区
        void arrange();
        // Inset 变体时主内容区的边距：靠侧栏一侧由侧栏的 p-2 提供，侧栏宽度为 0 时补上
        void updateInsetMargins();
        void connectSidebar();
        void disconnectSidebar();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    public:
        QHBoxLayout* _layout{nullptr};
        ShadSidebarInsetArea* _insetArea{nullptr};
        QVBoxLayout* _insetLayout{nullptr};
        QPointer<ShadSidebar> _sidebar;
        QPointer<QWidget> _content;
        QShortcut* _shortcut{nullptr};
        QMetaObject::Connection _sideConnection;
        QMetaObject::Connection _variantConnection;
};

#endif // SHADSIDEBARLAYOUTPRIVATE_H
