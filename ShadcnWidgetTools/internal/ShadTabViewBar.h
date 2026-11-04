#ifndef SHADTABVIEWBAR_H
#define SHADTABVIEWBAR_H

#include <QFont>
#include <QList>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QVariant>
#include <QWidget>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QVariantAnimation;
class QWheelEvent;
class ShadTransition;

// ShadTabView 的标签栏，自绘全部标签；拖出与拖入由 ShadTabView 驱动
class ShadTabViewBar : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadTabViewBar(QWidget* parent = nullptr);
        ~ShadTabViewBar() override;

        int insertTab(int index, ShadIconType::IconName icon, const QString& text);
        void removeTab(int index);
        void moveTab(int from, int to);
        int count() const;

        void setTabText(int index, const QString& text);
        QString tabText(int index) const;
        void setTabIcon(int index, ShadIconType::IconName icon);
        ShadIconType::IconName tabIcon(int index) const;
        void setTabFlags(int index, ShadTabViewType::TabFlags flags);
        ShadTabViewType::TabFlags tabFlags(int index) const;
        void setTabToolTip(int index, const QString& toolTip);
        QString tabToolTip(int index) const;
        void setTabData(int index, const QVariant& data);
        QVariant tabData(int index) const;

        void setCurrentIndex(int index);
        int currentIndex() const;
        // 循环切换到相邻的可用标签
        void selectAdjacent(int step);

        void setTabsClosable(bool closable);
        bool isTabsClosable() const;
        void setAddButtonVisible(bool visible);
        bool isAddButtonVisible() const;

        // 标题栏据此判断能否拖动窗口
        bool isBlankAt(const QPoint& pos) const;

        void setDetachEnabled(bool enabled);
        // 收起拖出中的标签，-1 恢复
        void setHiddenIndex(int index);
        // -1 隐藏指示条
        void setDropIndicator(int slot);
        int dropIndicatorSlot() const;
        int insertionSlotAt(const QPoint& pos) const;
        // 未滚动时第 slot 个位置的标签矩形
        QRect restingTabRect(int slot) const;
        QPixmap dragPixmap(int index, const QPixmap& pageShot) const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void currentChanged(int index);
        void tabCloseRequested(int index);
        void newTabRequested();
        void tabMoved(int from, int to);
        // 接收方可以删除本组件
        void tabDragOutRequested(int index, const QPoint& grabOffset);

    protected:
        bool event(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        struct Tab
        {
            ShadIconType::IconName icon{ShadIconType::None};
            QString text;
            QString toolTip;
            QVariant data;
            ShadTabViewType::TabFlags flags{ShadTabViewType::NoTabFlag};
            ShadTransition* hoverTransition{nullptr};
            ShadTransition* activeTransition{nullptr};
            qreal startX{0.0};
            qreal startWidth{0.0};
        };

        enum class Part
        {
            None,
            Tab,
            Close,
            Add,
        };

        QFont tabFont() const;
        qreal availableWidth() const;
        int slotCount() const;
        int slotOf(int index) const;
        qreal tabWidth() const;
        qreal contentWidth() const;
        qreal maxScroll() const;
        qreal slotX(int slot) const;
        // 逻辑坐标（LTR），用 toVisual 按布局方向镜像
        QRectF logicalTabRect(int index) const;
        QRectF logicalCloseRect(int index) const;
        QRectF logicalAddRect() const;
        QRectF toVisual(const QRectF& logicalRect) const;
        bool isCloseVisible(int index) const;
        bool isSelectable(int index) const;
        int nextSelectable(int from, int step) const;
        // 两侧以不可移动标签为界的区间
        void movableRange(int index, int* first, int* last) const;
        void dropSlotRange(int* first, int* last) const;
        int hitTest(const QPoint& pos, Part* part) const;
        qreal logicalX(qreal x) const;

        void setHover(int index, Part part);
        void moveFocusTo(int index);
        void updateActiveTransitions();
        void setScrollOffset(qreal offset);
        void ensureVisible(int index);
        // 排布变化前记录位置，变化后从记录处过渡
        void snapshotLayout();
        void startLayoutAnimation();
        void releaseFrozenWidth();
        void endDrag();

        QList<Tab> _tabs;
        int _currentIndex{-1};
        int _focusIndex{-1};
        int _hoverIndex{-1};
        Part _hoverPart{Part::None};
        int _pressedIndex{-1};
        Part _pressedPart{Part::None};
        Qt::MouseButton _pressedButton{Qt::NoButton};
        bool _isFocusVisible{false};
        bool _isTabsClosable{true};
        bool _isAddButtonVisible{true};
        qreal _scrollOffset{0.0};
        // 小于 0 表示不保持
        qreal _frozenTabWidth{-1.0};
        QPoint _pressPos;
        qreal _dragGrabOffset{0.0};
        qreal _dragX{0.0};
        bool _isDragging{false};
        bool _isDetachEnabled{false};
        int _hiddenIndex{-1};
        int _dropSlot{-1};
        QVariantAnimation* _layoutAnimation{nullptr};
        qreal _layoutProgress{1.0};
        ShadTransition* _addHoverTransition{nullptr};
};

#endif // SHADTABVIEWBAR_H
