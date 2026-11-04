#ifndef SHADBREADCRUMBITEM_H
#define SHADBREADCRUMBITEM_H

#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadIconType.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class ShadDropdownMenu;
class ShadTransition;

// ShadBreadcrumb 的一项，不对外导出：链接、当前页、下拉项、分隔符、省略号（可带菜单）
// 四周预留焦点环边距；文字放不下时尾部省略，并在悬停提示中显示完整文字
class ShadBreadcrumbItem : public QWidget
{
    Q_OBJECT

    public:
        enum class Kind
        {
            // BreadcrumbLink：text-muted-foreground hover:text-foreground
            Link,
            // BreadcrumbPage：text-foreground，不可点击
            Page,
            // 文字 + ChevronDown，点击弹出菜单
            Dropdown,
            // BreadcrumbSeparator：[&>svg]:size-3.5
            Separator,
            // BreadcrumbEllipsis：size-9 的 MoreHorizontal，只做展示
            Ellipsis,
            // 放在 ghost icon-sm 按钮中的省略号，点击弹出菜单
            EllipsisButton,
        };

        ShadBreadcrumbItem(Kind kind, QWidget* parent = nullptr);
        ~ShadBreadcrumbItem() override;

        Kind kind() const;
        bool isInteractive() const;
        bool hasText() const;

        void setText(const QString& text);
        QString text() const;
        // 分隔符图标
        void setShadIcon(ShadIconType::IconName icon);
        // Dropdown / EllipsisButton 点击时弹出的菜单，不接管所有权
        void setMenu(ShadDropdownMenu* menu);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void clicked();

    protected:
        void changeEvent(QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        // 不含焦点环边距的可视区域
        QRect bodyRect() const;
        // 文字可用宽度（Dropdown 扣除箭头）
        int textAvailableWidth() const;
        void updateElidedToolTip();
        void activate();

        Kind _kind{Kind::Link};
        QString _text;
        ShadIconType::IconName _icon{ShadIconType::ChevronRight};
        QPointer<ShadDropdownMenu> _menu;
        ShadTransition* _hoverTransition{nullptr};
        bool _isPressed{false};
        bool _isFocusVisible{false};
};

#endif // SHADBREADCRUMBITEM_H
