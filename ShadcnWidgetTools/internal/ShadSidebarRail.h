#ifndef SHADSIDEBARRAIL_H
#define SHADSIDEBARRAIL_H

#include <QWidget>

class QEnterEvent;
class QEvent;
class QMouseEvent;
class QPaintEvent;

// 侧栏内侧边缘的切换条，组件库内部使用，对应 SidebarRail：悬停时在侧栏边缘显示 2px sidebar-border 竖线，点击切换侧栏展开状态
// 允许拖动时，移动超过系统拖动阈值即为拖动，松开时不发出 clicked
// lineAtStart 为 true 时竖线位于本组件的起始侧（逻辑左侧），否则位于结束侧
class ShadSidebarRail : public QWidget
{
    Q_OBJECT

    public:
        static constexpr int kRailWidth = 8;

        explicit ShadSidebarRail(QWidget* parent = nullptr);
        ~ShadSidebarRail() override;

        void setLineAtStart(bool lineAtStart);
        // 是否允许拖动，默认 false；同时切换提示文字
        void setDragEnabled(bool enabled);

    Q_SIGNALS:
        void clicked();
        void dragStarted();
        // deltaX 为相对按下位置的屏幕横向位移（物理方向，向右为正）
        void dragMoved(int deltaX);
        void dragFinished();

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        bool _isHovered{false};
        bool _isLineAtStart{false};
        bool _isDragEnabled{false};
        bool _isPressed{false};
        bool _isDragging{false};
        int _pressGlobalX{0};
};

#endif // SHADSIDEBARRAIL_H
