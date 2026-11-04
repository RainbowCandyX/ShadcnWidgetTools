#ifndef SHADCAROUSELPRIVATE_H
#define SHADCAROUSELPRIVATE_H

#include <QElapsedTimer>
#include <QList>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadProperty.h"

class QMouseEvent;
class QTimer;
class QWidget;
class ShadButton;
class ShadCarousel;
class ShadFrameAnimation;

class ShadCarouselPrivate
{
    SHAD_D_CREATE(ShadCarousel)

    public:
        ShadCarouselPrivate();
        ~ShadCarouselPrivate();

        // 翻页按钮主体到内容区的距离，对应 -left-12 / -right-12（纵向为 -top-12 / -bottom-12）
        static constexpr int kButtonOffset = 48;
        // 开始拖动前需要移动的距离，对应 Embla 的 dragThreshold
        static constexpr int kDragThreshold = 10;
        // 切换动画时长
        static constexpr int kScrollDuration = 400;
        // 超出首尾拖动时的阻力系数：实际移动距离为超出距离乘以该系数
        static constexpr qreal kEdgeResistance = 0.3;
        // 松开时按速度向前预估的时长（毫秒），用于判断是否切换到相邻停靠点
        static constexpr qreal kFlickProjectionMs = 150.0;
        // 计算松开速度时使用的最近一段时间（毫秒）
        static constexpr qint64 kVelocityWindowMs = 100;
        // 判断两个停靠点重合的容差（像素）
        static constexpr qreal kSnapTolerance = 0.5;

        // 方向：0 取最近的一侧，1 向结束侧，-1 向起始侧
        enum Direction
        {
            Nearest  = 0,
            Forward  = 1,
            Backward = -1,
        };

        // 拖动时记录的指针位置，用于计算松开时的速度
        struct DragSample
        {
            qint64 time;
            qreal position;
        };

        // 内容区距组件边缘的距离：显示按钮时为按钮偏移加焦点环边距，按钮组件的焦点环不会被裁剪
        int navigationMargin() const;
        QRect viewportRect(const QRect& bounds) const;
        // 视口在主轴方向的长度
        qreal viewportExtent() const;
        // 每张幻灯片占用的主轴长度（幻灯片加一个间距），对应 basis 包含 pl-4
        qreal slotExtent(qreal viewportLength) const;
        // 循环需要至少比一屏多一张幻灯片
        bool isLoopActive() const;
        // 循环一圈的长度
        qreal loopPeriod() const;
        // 不循环时的最大滚动位置
        qreal maxOffset() const;
        // 按方向把差值换算到一圈之内：Nearest 为 (-P/2, P/2]，Forward 为 (0, P]，Backward 为 [-P, 0)
        qreal wrapDelta(qreal delta, Direction direction) const;
        int nearestSnap(qreal offset) const;
        // 从 offset 出发到达停靠点 index 的目标位置；循环时按方向取最近的一圈
        qreal snapTarget(int index, qreal offset, Direction direction) const;

        // 单张幻灯片在主轴方向的尺寸提示，以及在给定主轴尺寸下交叉方向的尺寸
        int itemMainHint(const QWidget* item) const;
        int itemCrossHint(const QWidget* item, int mainExtent) const;
        // 横向时组件宽度为 width 时的视口高度；纵向时为默认的视口高度
        int contentCrossHint(int width) const;
        int contentMainHintVertical() const;

        // 重新计算停靠点，并在数量变化时发出信号；当前停靠点超出范围时取最后一个；返回停靠点是否有变化
        bool updateSnaps();
        // 放置视口与按钮，重新计算停靠点；停靠点变化或没有动画时停到当前停靠点，拖动中保持位置
        void updateLayout();
        void layoutItems();
        void setOffset(qreal offset);
        void updateButtons();
        // 设置当前停靠点并发出 currentChanged
        void setCurrent(int index);
        // 切换到停靠点 index，direction 用于循环时决定滚动方向
        void scrollToSnap(int index, Direction direction, bool animated);
        void animateTo(qreal target);
        void stopAnimation();
        // 回到当前停靠点：按下时会停住动画，没有拖动就松开时需要继续停靠
        void settle();
        // 方向键对应的切换方向：-1 上一张，1 下一张，0 不处理；横向 RTL 下左右对调
        int keyStep(int key) const;

        void restartAutoplay();
        void autoplayStep();

        // 为视口及其所有子组件安装事件过滤器，用于拖动与布局请求；只使用 QObject 接口，子组件析构过程中调用也是安全的
        void installFilter(QObject* object);
        void removeFilter(QObject* object);
        // 按下位置在输入框、滑块等自身需要拖动的组件内时不开始拖动（对应 Embla 跳过 input / select / textarea）
        bool isDragExcluded(QWidget* widget) const;
        // 处理视口内的鼠标事件，返回 true 表示事件已被拖动使用
        bool handleMouseEvent(QWidget* watched, QMouseEvent* event);
        // 指针位置在主轴方向的逻辑坐标（RTL 横向时取反），以及交叉方向的坐标
        qreal logicalAxisPosition(const QPoint& globalPos) const;
        qreal logicalCrossPosition(const QPoint& globalPos) const;
        void addSample(const QPoint& globalPos);
        void beginDrag(const QPoint& globalPos);
        // 松开时按拖动距离与速度选择停靠点
        void endDrag();
        // 结束拖动或指针跟踪，不再改变位置
        void resetPointer();

        QWidget* _viewport{nullptr};
        ShadButton* _prevButton{nullptr};
        ShadButton* _nextButton{nullptr};
        QList<QWidget*> _items;

        Qt::Orientation _orientation{Qt::Horizontal};
        int _itemsPerView{1};
        int _spacing{16};
        ShadCarouselType::Align _align{ShadCarouselType::Center};
        bool _isLoop{false};
        int _autoplayInterval{0};
        bool _isNavigationVisible{true};

        // 停靠点对应的滚动位置（视口起始边在轨道上的逻辑坐标）
        QList<qreal> _snaps;
        int _currentIndex{-1};
        // 当前滚动位置，循环时不限制范围，绘制时按一圈取模
        qreal _offset{0.0};
        ShadFrameAnimation* _animation{nullptr};

        QTimer* _autoplayTimer{nullptr};
        bool _isHovered{false};

        // 拖动状态
        bool _isPointerDown{false};
        bool _isDragging{false};
        // 正在向子组件发送取消点击的松开事件，事件过滤器放行
        bool _isSendingCancel{false};
        QPointer<QWidget> _pressWidget;
        QPoint _pressGlobalPos;
        qreal _pressOffset{0.0};
        int _pressSnap{0};
        QElapsedTimer _dragClock;
        QList<DragSample> _samples;
};

#endif // SHADCAROUSELPRIVATE_H
