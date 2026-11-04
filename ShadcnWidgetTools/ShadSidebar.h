#ifndef SHADSIDEBAR_H
#define SHADSIDEBAR_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QObject;
class QPaintEvent;
class QResizeEvent;
class QVBoxLayout;
class ShadSidebarGroup;
class ShadSidebarMenuButton;

// 侧栏，宽度由自身控制；与主内容配合时使用 ShadSidebarLayout
class ShadSidebarPrivate;

class SHADCN_EXPORT ShadSidebar : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSidebar)
    Q_PROPERTY(ShadSidebarType::Side side READ getSide WRITE setSide NOTIFY sideChanged)
    Q_PROPERTY(ShadSidebarType::Variant variant READ getVariant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(ShadSidebarType::Collapsible collapsible READ getCollapsible WRITE setCollapsible NOTIFY collapsibleChanged)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(int expandedWidth READ getExpandedWidth WRITE setExpandedWidth NOTIFY expandedWidthChanged)
    Q_PROPERTY(bool resizable READ isResizable WRITE setResizable)
    Q_PROPERTY(bool subMenuExclusive READ isSubMenuExclusive WRITE setSubMenuExclusive)
    Q_PROPERTY(bool railVisible READ isRailVisible WRITE setRailVisible)
    Q_PROPERTY(int indicatorDuration READ getIndicatorDuration WRITE setIndicatorDuration)

    public:
        explicit ShadSidebar(QWidget* parent = nullptr);
        ~ShadSidebar() override;

        void setSide(ShadSidebarType::Side side);
        ShadSidebarType::Side getSide() const;
        void setVariant(ShadSidebarType::Variant variant);
        ShadSidebarType::Variant getVariant() const;
        // 设为 NotCollapsible 时立即展开
        void setCollapsible(ShadSidebarType::Collapsible collapsible);
        ShadSidebarType::Collapsible getCollapsible() const;

        // 显示期间修改有过渡动画
        void setExpanded(bool expanded);
        bool isExpanded() const;
        void toggle();
        bool isIconCollapsed() const;

        // Floating / Inset 变体的宽度包含四周内边距
        void setExpandedWidth(int width);
        int getExpandedWidth() const;
        // 当前宽度超出范围时立即调整；maximum 小于 minimum 时按 minimum 处理
        void setExpandedWidthRange(int minimum, int maximum);
        int getMinimumExpandedWidth() const;
        int getMaximumExpandedWidth() const;
        // 仅展开且无过渡动画时可拖动 Rail
        void setResizable(bool resizable);
        bool isResizable() const;

        // 开启时同一时间只展开一个子菜单
        void setSubMenuExclusive(bool exclusive);
        bool isSubMenuExclusive() const;

        // 选中指示条切换动画的时长，为 0 时不播放
        void setIndicatorDuration(int msecs);
        int getIndicatorDuration() const;

        void setRailVisible(bool visible);
        bool isRailVisible() const;

        QVBoxLayout* headerLayout() const;
        QVBoxLayout* footerLayout() const;
        QVBoxLayout* contentLayout() const;
        ShadSidebarGroup* addGroup(const QString& label = QString());

        // 传 nullptr 清除；不发出 itemActivated
        void setActiveButton(ShadSidebarMenuButton* button);
        ShadSidebarMenuButton* getActiveButton() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void expandedChanged(bool expanded);
        // 拖动期间持续发出
        void expandedWidthChanged(int width);
        void sideChanged();
        void variantChanged();
        void collapsibleChanged();
        // 用户点击没有子菜单的菜单按钮时发出
        void itemActivated(ShadSidebarMenuButton* button);

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        friend class ShadSidebarMenuButtonPrivate;
};

#endif // SHADSIDEBAR_H
