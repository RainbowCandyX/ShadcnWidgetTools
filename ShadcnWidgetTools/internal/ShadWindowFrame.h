#ifndef SHADWINDOWFRAME_H
#define SHADWINDOWFRAME_H

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QWindow>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <array>
#include <memory>

class QEvent;
class QPainter;
class QWidget;
class ShadTitleBar;
class ShadWindowCorner;
class ShadWindowShadow;

// 不使用系统标题栏的顶层窗口的边框逻辑，组件库内部使用，ShadWindow、ShadDialog 与 ShadWidget 共用
// Windows 11 保留系统边框以获得阴影、圆角与 Aero Snap，系统标题栏在 WM_NCCALCSIZE 中去掉；
// Windows 10 与 macOS / Linux 使用 FramelessWindowHint 与透明背景，自绘圆角（半径为 rounded-lg）与四周 1px 边框，最大化或全屏时为直角且去掉边框；窗口内侧 6px 拖动缩放
// Windows 10 另外补回系统窗口样式，贴靠、Win + 方向键、系统菜单与缩放仍由系统处理；系统不给透明窗口画阴影，改用 ShadWindowShadow 独立阴影窗口
// 构造时在窗口上安装事件过滤器，处理显示、窗口状态与激活状态变化；窗口需要在 nativeEvent 与 paintEvent 中转发给本类
class ShadWindowFrame : public QObject
{
    Q_OBJECT

    public:
        // window 必须是顶层窗口，titleBar 为其标题栏，两者生命周期不短于本对象
        ShadWindowFrame(QWidget* window, ShadTitleBar* titleBar);
        ~ShadWindowFrame() override;

        // 自绘边框宽度：Windows 11 由系统绘制边框，返回 0；其余平台非最大化、非全屏时为 1
        int borderWidth() const;
        // 自绘圆角半径：Windows 11 由系统处理圆角，返回 0；其余平台非最大化、非全屏时为主题的 rounded-lg
        int cornerRadius() const;
        // 根据窗口状态更新边框占用的 contentsMargins；标题栏浮动时上边距再加上标题栏高度，并重新放置标题栏
        void updateFrameLayout();
        // 标题栏不放进窗口的布局（ShadWidget 使用）：上边距让出标题栏高度，标题栏固定在窗口顶部并跟随窗口宽度，
        // 窗口自己的布局排在标题栏下方；默认 false，由窗口的根布局放置标题栏
        void setTitleBarFloating(bool floating);
        // 为 true 时禁止拖动缩放
        static constexpr const char* kResizeDisabledProperty = "_shadResizeDisabled";
        // 非最大化、非全屏且不是固定尺寸时允许拖动边缘缩放
        bool canResize() const;
        bool hasWindowEffect() const;

        // 在窗口的 nativeEvent 中调用，返回 true 表示已处理并写入 result
        bool handleNativeEvent(const QByteArray& eventType, void* message, qintptr* result);
        // 在窗口的 paintEvent 中调用：Background 背景与边框
        void paint(QPainter* painter) const;

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        // 透明背景自绘外形时，窗口内侧可拖动缩放的边缘宽度
        static constexpr int kResizeMargin = 6;

        // 是否使用透明背景自绘圆角与边框：Windows 10 与 macOS / Linux 为 true，Windows 11 为 false
        bool usesTranslucentFrame() const;
        // 不考虑最大化与全屏的圆角半径
        int baseCornerRadius() const;
        void applyWindowEffect();
        // 标题栏浮动时按边框与窗口宽度放置标题栏
        void layoutTitleBar();
        // 按窗口状态显示或隐藏四个圆角组件，放到对应位置并提到最上层；没有圆角组件时不做任何事
        void updateCorners();
        // 把圆角组件提到所有子组件之上（新增子组件后调用）
        void raiseCorners();
        // pos 为窗口逻辑坐标，返回落在窗口内侧 kResizeMargin 内的缩放边缘；不能缩放时为空
        Qt::Edges edgesAt(const QPoint& pos) const;
#ifdef Q_OS_WIN
        bool handleNativeMessage(void* message, qintptr* result);
        // Windows 11 触发 WM_NCCALCSIZE 去掉系统标题栏，Windows 10 补回系统窗口样式；同一个原生窗口只处理一次
        void applyNativeFrame();
        // 让系统绘制的窗口边框跟随主题明暗，Windows 10 没有系统边框，不处理
        void updateNativeDarkMode();
        // 右键标题栏弹出系统菜单，坐标为屏幕物理像素
        void showSystemMenu(int nativeX, int nativeY);
        // Windows 10 按窗口当前的位置、尺寸、状态、激活与不透明度更新阴影窗口；Windows 11 不处理
        void updateShadow();
#else
        // 在 QWindow 上过滤鼠标事件实现边缘缩放，子组件覆盖的边缘同样有效；返回 true 表示事件已被消费
        bool handleResizeEvent(QEvent* event);
        // 更新边缘悬停状态并切换缩放光标
        void setHoverEdges(Qt::Edges edges);
#if defined(Q_OS_MACOS) || defined(Q_OS_WASM)
        // macOS 与 WebAssembly 不使用 startSystemResize，按下边缘后根据鼠标位移手动计算窗口几何
        void applyManualResize(const QPoint& globalPos);
#endif
#endif

        QWidget* _window{nullptr};
        ShadTitleBar* _titleBar{nullptr};
        bool _isTitleBarFloating{false};
        bool _hasAppliedEffect{false};
        // 左上、右上、左下、右下四个圆角，只在 usesTranslucentFrame() 为 true 时创建
        std::array<ShadWindowCorner*, 4> _corners{};
#ifdef Q_OS_WIN
        quintptr _framedWinId{0};
        bool _isWindows10{false};
        // Windows 10 的阴影窗口，Windows 11 为空
        std::unique_ptr<ShadWindowShadow> _shadow;
        // 已连接 opacityChanged 的 QWindow；原生窗口重建后 QWindow 可能更换，需要重新连接
        QPointer<QWindow> _opacityWindow;
        QMetaObject::Connection _opacityConnection;
#else
        Qt::Edges _hoverEdges;
        bool _isOverrideCursor{false};
#if defined(Q_OS_MACOS) || defined(Q_OS_WASM)
        // 手动缩放状态：按下的边缘、按下时的窗口几何与鼠标全局位置
        Qt::Edges _resizeEdges;
        QRect _resizeStartGeometry;
        QPoint _resizeStartPos;
#endif
#endif
};

#endif // SHADWINDOWFRAME_H
