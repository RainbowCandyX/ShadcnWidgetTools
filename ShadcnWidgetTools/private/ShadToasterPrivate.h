#ifndef SHADTOASTERPRIVATE_H
#define SHADTOASTERPRIVATE_H

#include <QList>
#include <QPointer>
#include <QRect>
#include <QWidget>

#include "ShadDef.h"

class ShadToaster;
class ShadToastItem;

class ShadToasterPrivate
{
    SHAD_D_CREATE(ShadToaster)

    public:
        ShadToasterPrivate();
        ~ShadToasterPrivate();

        // Viewport：距窗口边缘 16px，最大宽度 max-w-sm（384px）
        static constexpr int kViewportOffset = 16;
        static constexpr int kMaxCardWidth = 384;
        // 展开时间距 --gap，收起时后面每条露出 --peek，每层缩小 10%
        static constexpr int kGap = 12;
        static constexpr int kPeek = 12;
        static constexpr qreal kScaleStep = 0.1;
        // 收起时最多显示的层数，对应 visibleToasts=3
        static constexpr int kVisibleStackCount = 3;
        // 卡片四周预留给 shadow-lg 的绘制与鼠标区域
        static constexpr int kShadowMargin = 20;

        bool isBottom() const;
        int cardWidth() const;
        // 宿主自绘标题栏的高度，读取 titleBarHeight 属性；没有该属性时为 0
        int titleBarInset() const;
        // ShadToaster 在宿主中的几何：与卡片同列、从标题栏下方到宿主底部，鼠标区域由遮罩限定
        QRect toasterGeometry() const;
        // 入场起点：在目标位置基础上沿堆叠方向移出 150% 自身高度（对应 data-starting-style:translateY(150%)）
        QRect offscreenRect(const QRect& rect) const;
        // 离场终点：不论原来在堆叠中的哪个位置，都从锚点移出 150% 自身高度，保证滑到窗口外
        // （对应 data-ending-style 的 translateY(150%)，该 transform 不叠加堆叠偏移）
        QRect exitRect(const QRect& rect) const;
        ShadToastItem* findItem(int id) const;
        int effectiveLimit() const;

        // 重新计算所有 toast 的位置，animate 为 false 时立即到位（例如宿主窗口缩放）
        void relayout(bool animate);
        void updateMask();
        void setExpanded(bool expanded);
        // 从显示列表移入离场列表并开始离场动画，发出 toastClosed
        void beginDismiss(ShadToastItem* item);
        void onExitFinished(ShadToastItem* item);

        QPointer<QWidget> _host;
        // 最新的在前，不含正在离场的
        QList<ShadToastItem*> _items;
        QList<ShadToastItem*> _exitingItems;
        ShadToastType::Position _position{ShadToastType::TopRight};
        // 0 表示不限制
        int _limit{0};
        int _duration{5000};
        int _nextId{1};
        bool _isExpanded{false};
};

#endif // SHADTOASTERPRIVATE_H
