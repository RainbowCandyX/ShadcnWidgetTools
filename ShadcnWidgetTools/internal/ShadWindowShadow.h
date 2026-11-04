#ifndef SHADWINDOWSHADOW_H
#define SHADWINDOWSHADOW_H

#include <QtGlobal>

#ifdef Q_OS_WIN
#include <QRect>
#include <QSize>

class QImage;

// Windows 10 透明背景窗口的阴影，组件库内部使用，由 ShadWindowFrame 驱动
// 系统不给分层窗口绘制阴影，这里用一个跟随主窗口的独立分层窗口代替（与 Visual Studio、Office 在 Windows 10 上的做法相同）
// 阴影窗口由主窗口拥有（owned window）：系统保证它位于主窗口之上，不显示在任务栏与 Alt+Tab 中；
// 主窗口范围内的像素完全透明，并设置 WS_EX_TRANSPARENT，不影响主窗口的显示与鼠标操作
class ShadWindowShadow
{
    public:
        ShadWindowShadow();
        ~ShadWindowShadow();
        ShadWindowShadow(const ShadWindowShadow&) = delete;
        ShadWindowShadow& operator=(const ShadWindowShadow&) = delete;

        // 按主窗口当前状态显示、移动、重绘或隐藏阴影
        // ownerWinId 为主窗口的原生句柄；为 0、主窗口不可见、最小化、最大化或 isAllowed 为 false 时隐藏阴影
        // cornerRadius 为逻辑像素，devicePixelRatio 为主窗口的缩放比例，opacity 为主窗口的不透明度，isActive 为主窗口是否激活
        void update(quintptr ownerWinId, bool isAllowed, int cornerRadius, qreal devicePixelRatio, qreal opacity, bool isActive);

    private:
        // 阴影向窗口四周延伸的距离（逻辑像素），覆盖模糊半径的 3 倍与向下偏移
        static constexpr int kMargin = 28;
        // 高斯模糊的标准差（逻辑像素）
        static constexpr qreal kBlur = 8.0;
        // 阴影向下偏移（逻辑像素）
        static constexpr qreal kOffsetY = 4.0;
        // 阴影形状内部的最大不透明度
        static constexpr qreal kAlpha = 0.3;
        // 主窗口未激活时阴影的整体不透明度
        static constexpr qreal kInactiveOpacity = 0.6;

        void hide();
        // 确保存在属于 ownerWinId 的阴影窗口，主窗口更换时重新创建
        bool ensureWindow(quintptr ownerWinId);
        void destroyWindow();
        // shadowRect 为阴影窗口的屏幕物理像素矩形，margin 与 radius 为物理像素
        bool render(const QRect& shadowRect, int margin, qreal radius, qreal devicePixelRatio, int alpha);
        static void paintShadow(QImage& image, int margin, qreal radius, qreal devicePixelRatio);

        quintptr _hwnd{0};
        quintptr _ownerWinId{0};
        QRect _shadowRect;
        QSize _renderedOwnerSize;
        int _renderedRadius{-1};
        qreal _renderedDevicePixelRatio{0.0};
        int _alpha{-1};
        bool _isShown{false};
};

#endif

#endif // SHADWINDOWSHADOW_H
