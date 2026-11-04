#ifndef SHADTABSLIST_H
#define SHADTABSLIST_H

#include <QFont>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class ShadTransition;

// ShadTabs 的标签栏（对应 TabsList + TabsTrigger），所有标签在本组件内绘制
// Default：bg-muted rounded-lg p-[3px]，横向高 h-9，选中项 bg-background shadow-sm；Line：背景透明，标签之间 gap-1，选中项显示 2px 指示线
// 横向时各标签等宽（flex-1，宽度取最宽的标签）；纵向时标签占满宽度，内容靠起始侧
// 键盘：Tab 聚焦到当前标签，方向键与 Home / End 在可用的标签之间移动焦点并循环；activateOnFocus 关闭时回车或空格才切换
class ShadTabsList : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadTabsList(QWidget* parent = nullptr);
        ~ShadTabsList() override;

        // index 超出范围时添加到末尾，返回实际位置
        int insertTab(int index, ShadIconType::IconName icon, const QString& text);
        void removeTab(int index);
        int count() const;

        void setTabText(int index, const QString& text);
        QString tabText(int index) const;
        void setTabIcon(int index, ShadIconType::IconName icon);
        ShadIconType::IconName tabIcon(int index) const;
        // 禁用的标签为 50% 不透明度，不响应鼠标，键盘导航时跳过
        void setTabEnabled(int index, bool enabled);
        bool isTabEnabled(int index) const;

        void setCurrentIndex(int index);
        int currentIndex() const;

        void setVariant(ShadTabsType::Variant variant);
        ShadTabsType::Variant getVariant() const;
        void setOrientation(Qt::Orientation orientation);
        Qt::Orientation getOrientation() const;
        void setActivateOnFocus(bool activateOnFocus);
        bool isActivateOnFocus() const;

        // Line 变体的指示线超出标签栏的距离（横向在下方，纵向在结束侧），ShadTabs 据此减小与内容区的间距
        int indicatorOverflow() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void currentChanged(int index);

    protected:
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        struct Tab
        {
            ShadIconType::IconName icon{ShadIconType::None};
            QString text;
            bool isEnabled{true};
            // 悬停与选中的过渡（transition-all 150ms）
            ShadTransition* hoverTransition{nullptr};
            ShadTransition* activeTransition{nullptr};
        };

        QFont tabFont() const;
        // 每个标签的宽度：横向为最宽标签的宽度，纵向为占满标签栏
        int tabWidth() const;
        int tabHeight() const;
        int tabSpacing() const;
        // 标签栏背景（不含指示线超出的部分）的逻辑矩形
        QRect listRect() const;
        // 标签的逻辑矩形（LTR 坐标）与实际矩形（按布局方向镜像）
        QRect logicalTabRect(int index) const;
        QRect tabRect(int index) const;
        int tabAt(const QPoint& position) const;
        bool isSelectable(int index) const;
        // 从 from 开始按 step 方向找下一个可用的标签，到头后从另一端继续
        int nextSelectable(int from, int step) const;
        // 第一个或最后一个可用的标签
        int edgeSelectable(bool isFirst) const;
        void setHoverIndex(int index);
        void moveFocusTo(int index);
        void updateActiveTransitions();

        QList<Tab> _tabs;
        int _currentIndex{-1};
        // 键盘焦点所在的标签（roving tabindex）
        int _focusIndex{-1};
        int _hoverIndex{-1};
        int _pressedIndex{-1};
        bool _isFocusVisible{false};
        ShadTabsType::Variant _variant{ShadTabsType::Default};
        Qt::Orientation _orientation{Qt::Horizontal};
        bool _isActivateOnFocus{false};
};

#endif // SHADTABSLIST_H
