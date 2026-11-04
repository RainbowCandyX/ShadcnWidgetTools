#include "ShadWindowFrame.h"

#include <QColor>
#include <QCursor>
#include <QEvent>
#include <QGuiApplication>
#include <QImage>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLayout>
#include <QMargins>
#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QVariant>
#include <QWidget>
#include <QWindow>

#ifdef Q_OS_WIN
#include <QOperatingSystemVersion>

#include <memory>
#endif

#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTitleBar.h"
#include "ShadWindowButton.h"
#include "ShadWindowCorner.h"
#include "ShadWindowEffect.h"
#include "ShadWindowShadow.h"

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#endif

namespace
{
#ifdef Q_OS_WIN
    // DWMWA_USE_IMMERSIVE_DARK_MODE，Windows 10 20H1 起支持；旧版 SDK 没有该常量，这里直接使用数值
    constexpr DWORD kDwmUseImmersiveDarkMode = 20;

    // 系统缩放边框厚度（物理像素）；最大化时窗口四周会超出屏幕同样的距离
    int resizeBorderThickness(HWND hwnd, bool isHorizontal)
    {
        const UINT dpi = ::GetDpiForWindow(hwnd);
        return ::GetSystemMetricsForDpi(isHorizontal ? SM_CXSIZEFRAME : SM_CYSIZEFRAME, dpi) + ::GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    }

    // Windows 10 无边框窗口需要补回的系统样式：有标题与缩放边框才支持贴靠、Win + 方向键与最小化动画
    // 客户区在 WM_NCCALCSIZE 中占满整个窗口，系统不会绘制这些边框
    // 最小化 / 最大化跟随标题栏按钮：设置了 FramelessWindowHint 时 Qt 不会自动加上对应的窗口标志，不能以窗口标志为准
    DWORD windows10StyleBits(ShadWindowType::ButtonFlags buttonFlags)
    {
        DWORD bits = WS_CAPTION | WS_THICKFRAME | WS_SYSMENU;
        if (buttonFlags.testFlag(ShadWindowType::MinimizeButton))
        {
            bits |= WS_MINIMIZEBOX;
        }
        if (buttonFlags.testFlag(ShadWindowType::MaximizeButton))
        {
            bits |= WS_MAXIMIZEBOX;
        }
        return bits;
    }

    // 缩放边缘对应的 WM_NCHITTEST 返回值，edges 不能为空
    LRESULT hitTestForEdges(Qt::Edges edges)
    {
        const bool isLeft = edges.testFlag(Qt::LeftEdge);
        const bool isRight = edges.testFlag(Qt::RightEdge);
        if (edges.testFlag(Qt::TopEdge))
        {
            return isLeft ? HTTOPLEFT : (isRight ? HTTOPRIGHT : HTTOP);
        }
        if (edges.testFlag(Qt::BottomEdge))
        {
            return isLeft ? HTBOTTOMLEFT : (isRight ? HTBOTTOMRIGHT : HTBOTTOM);
        }
        return isLeft ? HTLEFT : HTRIGHT;
    }
#else
    Qt::CursorShape cursorShapeForEdges(Qt::Edges edges)
    {
        const bool isLeft = edges.testFlag(Qt::LeftEdge);
        const bool isRight = edges.testFlag(Qt::RightEdge);
        const bool isTop = edges.testFlag(Qt::TopEdge);
        const bool isBottom = edges.testFlag(Qt::BottomEdge);
        if ((isLeft && isTop) || (isRight && isBottom))
        {
            return Qt::SizeFDiagCursor;
        }
        if ((isRight && isTop) || (isLeft && isBottom))
        {
            return Qt::SizeBDiagCursor;
        }
        if (isLeft || isRight)
        {
            return Qt::SizeHorCursor;
        }
        return Qt::SizeVerCursor;
    }
#endif
} // namespace

ShadWindowFrame::ShadWindowFrame(QWidget* window, ShadTitleBar* titleBar)
    : QObject(window), _window(window), _titleBar(titleBar)
{
#ifdef Q_OS_WIN
    // Windows 11 保留系统窗口样式以获得阴影、圆角与 Aero Snap，系统标题栏在 WM_NCCALCSIZE 中去掉
    // Windows 10 的系统不提供窗口圆角，与 macOS / Linux 一样使用透明背景自绘，系统窗口样式在 applyNativeFrame 中补回
    _isWindows10 = QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows11;
    if (_isWindows10)
    {
        _shadow = std::make_unique<ShadWindowShadow>();
    }
#else
    // 边缘缩放由 handleResizeEvent 处理：Linux 调用 startSystemResize，macOS 与 WebAssembly 手动计算窗口几何
#endif
    if (usesTranslucentFrame())
    {
        _window->setWindowFlag(Qt::FramelessWindowHint);
        _corners = {new ShadWindowCorner(Qt::TopLeftCorner, _window), new ShadWindowCorner(Qt::TopRightCorner, _window), new ShadWindowCorner(Qt::BottomLeftCorner, _window), new ShadWindowCorner(Qt::BottomRightCorner, _window)};
    }
    // 必须在原生窗口创建前设置；Linux 需要窗口合成器，否则透明区域显示为黑色
    _window->setAttribute(Qt::WA_TranslucentBackground);
    _window->installEventFilter(this);
    updateFrameLayout();
    updateCorners();
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]()
    {
        applyWindowEffect();
        _window->update();
#ifdef Q_OS_WIN
        updateNativeDarkMode();
        // 阴影形状跟随圆角半径
        updateShadow();
#endif
        // 圆角半径随主题的基础圆角变化，边框颜色随明暗变化
        updateCorners();
    });
}

ShadWindowFrame::~ShadWindowFrame()
{
#ifndef Q_OS_WIN
    if (_isOverrideCursor)
    {
        QGuiApplication::restoreOverrideCursor();
    }
#endif
}

bool ShadWindowFrame::usesTranslucentFrame() const
{
#ifdef Q_OS_WIN
    return _isWindows10;
#else
    return true;
#endif
}

int ShadWindowFrame::borderWidth() const
{
    if (!usesTranslucentFrame())
    {
        return 0;
    }
    return (_window->isMaximized() || _window->isFullScreen()) ? 0 : 1;
}

int ShadWindowFrame::cornerRadius() const
{
    if (!usesTranslucentFrame())
    {
        return 0;
    }
    return (_window->isMaximized() || _window->isFullScreen()) ? 0 : baseCornerRadius();
}

int ShadWindowFrame::baseCornerRadius() const
{
#ifdef Q_OS_WIN
    // 系统模糊作用于整个窗口矩形
    if (_isWindows10 && shadTheme->getWindowEffect() == ShadWindowType::Blur)
    {
        return 0;
    }
#endif
    return shadTheme->getRadius(ShadThemeType::Large);
}

bool ShadWindowFrame::hasWindowEffect() const
{
    const ShadWindowType::Effect effect = shadTheme->getWindowEffect();
    if (effect == ShadWindowType::Wallpaper)
    {
        return !ShadWindowEffect::wallpaper().isNull();
    }
    return effect != ShadWindowType::NoEffect;
}

void ShadWindowFrame::applyWindowEffect()
{
    const ShadWindowType::Effect effect = shadTheme->getWindowEffect();
    // 从未开启过特效时不调用平台接口
    if (_window->internalWinId() == 0 || (effect == ShadWindowType::NoEffect && !_hasAppliedEffect))
    {
        return;
    }
    ShadWindowEffect::apply(_window, effect, cornerRadius());
    _hasAppliedEffect = effect != ShadWindowType::NoEffect;
}

void ShadWindowFrame::updateFrameLayout()
{
    const int border = borderWidth();
    if (!_isTitleBarFloating)
    {
        _window->setContentsMargins(border, border, border, border);
        return;
    }
    // setBarHeight 通过 setFixedHeight 设置高度，最小高度即标题栏高度
    _window->setContentsMargins(border, border + _titleBar->minimumHeight(), border, border);
    layoutTitleBar();
}

void ShadWindowFrame::setTitleBarFloating(bool floating)
{
    if (_isTitleBarFloating == floating)
    {
        return;
    }
    _isTitleBarFloating = floating;
    updateFrameLayout();
}

void ShadWindowFrame::layoutTitleBar()
{
    const int border = borderWidth();
    _titleBar->setGeometry(border, border, qMax(0, _window->width() - 2 * border), _titleBar->minimumHeight());
}

bool ShadWindowFrame::canResize() const
{
    return !_window->isMaximized() && !_window->isFullScreen() && _window->minimumSize() != _window->maximumSize() && !_window->property(kResizeDisabledProperty).toBool();
}

bool ShadWindowFrame::handleNativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
#ifdef Q_OS_WIN
    if (eventType == QByteArrayLiteral("windows_generic_MSG"))
    {
        return handleNativeMessage(message, result);
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
#endif
    return false;
}

void ShadWindowFrame::paint(QPainter* painter) const
{
    const QColor borderColor = shadTheme->getColor(ShadThemeType::Border);
    const int radius = cornerRadius();
    const QRectF windowRect(_window->rect());
    QColor background = shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Background);
    if (hasWindowEffect())
    {
        const ShadWindowType::Effect effect = shadTheme->getWindowEffect();
        background = ShadStyleHelper::withAlpha(background, shadTheme->getWindowEffectOpacity(effect));
        if (effect == ShadWindowType::Wallpaper)
        {
            ShadWindowEffect::paintWallpaper(painter, _window, windowRect, radius);
        }
    }
    if (radius > 0)
    {
        // 透明背景上画圆角矩形；子组件盖住的四角由 ShadWindowCorner 清除并补画边框
        painter->setRenderHint(QPainter::Antialiasing, true);
        ShadStyleHelper::fillRoundedRect(painter, windowRect, radius, background);
        ShadStyleHelper::strokeRoundedRect(painter, windowRect, radius, borderColor, 1.0);
        return;
    }
    painter->fillRect(_window->rect(), background);
    if (borderWidth() > 0)
    {
        painter->setPen(borderColor);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(_window->rect().adjusted(0, 0, -1, -1));
    }
}

bool ShadWindowFrame::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _window)
    {
        switch (event->type())
        {
            case QEvent::WindowStateChange:
                {
#ifdef Q_OS_MACOS
                    // 固定尺寸的窗口会被系统报告为最大化
                    if (_window->isMaximized() && _window->minimumSize() == _window->maximumSize())
                    {
                        _window->overrideWindowState(_window->windowState() & ~Qt::WindowMaximized);
                        return false;
                    }
#endif
                    updateFrameLayout();
                    _titleBar->updateMaximizeState();
#ifndef Q_OS_WIN
                    setHoverEdges(Qt::Edges());
#endif
                    // 最大化与全屏时为直角
                    updateCorners();
                    applyWindowEffect();
#ifdef Q_OS_WIN
                    // 最大化、全屏时隐藏阴影
                    updateShadow();
#endif
                    _window->update();
                    break;
                }
#ifdef Q_OS_MACOS
            case QEvent::KeyPress:
                {
                    if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Close) && _titleBar->getButtonFlags().testFlag(ShadWindowType::CloseButton))
                    {
                        _titleBar->requestClose();
                        return true;
                    }
                    break;
                }
#endif
            case QEvent::Move:
                {
                    if (shadTheme->getWindowEffect() == ShadWindowType::Wallpaper)
                    {
                        _window->update();
                    }
                    break;
                }
            case QEvent::ActivationChange:
                {
                    _titleBar->setWindowActive(_window->isActiveWindow());
#ifdef Q_OS_WIN
                    // 未激活时阴影变淡
                    updateShadow();
#endif
                    break;
                }
            case QEvent::Resize:
                {
                    if (_isTitleBarFloating)
                    {
                        layoutTitleBar();
                    }
                    updateCorners();
                    break;
                }
            case QEvent::ChildAdded:
                {
                    // 新增的子组件排在最上层，圆角组件需要重新提到它之上
                    raiseCorners();
                    break;
                }
            case QEvent::WinIdChange:
                {
#ifdef Q_OS_WIN
                    // 原生窗口重建后（例如 setWindowFlags、首次加入 QRhiWidget）Windows 10 需要重新补回系统窗口样式；销毁时 winId 为 0，不处理
                    // 旧原生窗口销毁时系统会连带销毁它拥有的阴影窗口，新原生窗口在这里重新建立阴影
                    if (_isWindows10 && _window->internalWinId() != 0)
                    {
                        applyNativeFrame();
                        updateShadow();
                    }
#endif
                    applyWindowEffect();
                    break;
                }
            case QEvent::Show:
                {
#ifdef Q_OS_WIN
                    applyNativeFrame();
                    updateNativeDarkMode();
                    // 此时原生窗口尚未显示，阴影在随后的 WM_WINDOWPOSCHANGED 中显示；这里先连接不透明度变化
                    updateShadow();
#else
                    // 原生窗口可能因 setWindowFlags 等操作重建，每次显示时重新安装；重复安装同一个过滤器不会产生多份
                    if (QWindow* handle = _window->windowHandle())
                    {
                        handle->installEventFilter(this);
                    }
#endif
                    applyWindowEffect();
                    // 显示前子组件可能被单独 raise，显示时再确认圆角组件位于最上层
                    updateCorners();
                    break;
                }
            default:
                {
                    break;
                }
        }
        return false;
    }
#ifndef Q_OS_WIN
    if (watched == _window->windowHandle() && handleResizeEvent(event))
    {
        return true;
    }
#endif
    return QObject::eventFilter(watched, event);
}

void ShadWindowFrame::updateCorners()
{
    const int radius = cornerRadius();
    for (ShadWindowCorner* corner : _corners)
    {
        if (corner == nullptr)
        {
            continue;
        }
        corner->setVisible(radius > 0);
        if (radius > 0)
        {
            corner->place(radius);
        }
    }
    raiseCorners();
}

void ShadWindowFrame::raiseCorners()
{
    for (ShadWindowCorner* corner : _corners)
    {
        // 构造圆角组件的过程中也会收到 ChildAdded，此时数组尚未填好
        if (corner != nullptr)
        {
            corner->raise();
        }
    }
}

Qt::Edges ShadWindowFrame::edgesAt(const QPoint& pos) const
{
    Qt::Edges edges;
    if (!canResize())
    {
        return edges;
    }
    if (pos.x() < kResizeMargin)
    {
        edges |= Qt::LeftEdge;
    }
    if (pos.x() >= _window->width() - kResizeMargin)
    {
        edges |= Qt::RightEdge;
    }
    if (pos.y() < kResizeMargin)
    {
        edges |= Qt::TopEdge;
    }
    if (pos.y() >= _window->height() - kResizeMargin)
    {
        edges |= Qt::BottomEdge;
    }
    return edges;
}

#ifdef Q_OS_WIN
bool ShadWindowFrame::handleNativeMessage(void* message, qintptr* result)
{
    const MSG* msg = static_cast<const MSG*>(message);
    if (msg == nullptr || msg->hwnd == nullptr || reinterpret_cast<WId>(msg->hwnd) != _window->internalWinId())
    {
        return false;
    }
    const HWND hwnd = msg->hwnd;
    ShadWindowButton* maximizeButton = _titleBar->getMaximizeButton();
    switch (msg->message)
    {
        case WM_NCCALCSIZE:
            {
                if (_isWindows10)
                {
                    // 整个窗口都是客户区：分层窗口按整个窗口矩形显示，Qt 也按无边框窗口计算几何，保留非客户区会让两者错位
                    *result = 0;
                    return true;
                }
                if (msg->wParam == FALSE)
                {
                    return false;
                }
                NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
                const LONG originalTop = params->rgrc[0].top;
                const LRESULT defaultResult = ::DefWindowProcW(hwnd, WM_NCCALCSIZE, msg->wParam, msg->lParam);
                if (defaultResult != 0)
                {
                    *result = defaultResult;
                    return true;
                }
                // 去掉系统标题栏：左右下三边保留系统计算的边框（Windows 10/11 上不可见，用于缩放与阴影），顶部与窗口上沿对齐
                params->rgrc[0].top = originalTop;
                // 最大化时窗口四周超出屏幕一个边框厚度，顶部让出同样的距离，避免内容被裁掉
                if (::IsZoomed(hwnd))
                {
                    params->rgrc[0].top += resizeBorderThickness(hwnd, false);
                }
                *result = 0;
                return true;
            }
        case WM_NCHITTEST:
            {
                if (_window->isFullScreen())
                {
                    *result = HTCLIENT;
                    return true;
                }
                POINT nativePos{GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
                ::ScreenToClient(hwnd, &nativePos);
                const QPoint logicalPos = (QPointF(nativePos.x, nativePos.y) / _window->devicePixelRatioF()).toPoint();
                if (_isWindows10)
                {
                    // 没有系统边框，缩放区域为窗口内侧 kResizeMargin；圆角外的透明像素由系统直接穿透，收不到本消息
                    const Qt::Edges edges = edgesAt(logicalPos);
                    if (edges != Qt::Edges())
                    {
                        *result = hitTestForEdges(edges);
                        return true;
                    }
                }
                else
                {
                    // 左右下三边落在保留的系统边框上，由系统判断缩放方向
                    const LRESULT defaultHit = ::DefWindowProcW(hwnd, WM_NCHITTEST, msg->wParam, msg->lParam);
                    if (defaultHit != HTCLIENT)
                    {
                        const bool isResizeHit = defaultHit >= HTLEFT && defaultHit <= HTBOTTOMRIGHT;
                        *result = (isResizeHit && !canResize()) ? HTBORDER : defaultHit;
                        return true;
                    }
                    // 顶部系统边框已被去掉，在客户区内侧补上顶部缩放区域
                    if (canResize() && nativePos.y < resizeBorderThickness(hwnd, false))
                    {
                        RECT clientRect{};
                        ::GetClientRect(hwnd, &clientRect);
                        const int horizontalThickness = resizeBorderThickness(hwnd, true);
                        if (nativePos.x < horizontalThickness)
                        {
                            *result = HTTOPLEFT;
                        }
                        else if (nativePos.x >= clientRect.right - horizontalThickness)
                        {
                            *result = HTTOPRIGHT;
                        }
                        else
                        {
                            *result = HTTOP;
                        }
                        return true;
                    }
                }
                const QPoint titleBarPos = _titleBar->mapFrom(_window, logicalPos);
                // 最大化按钮返回 HTMAXBUTTON，Windows 11 悬停时才会弹出贴靠布局
                const bool isOverMaximize = maximizeButton->isVisible() && maximizeButton->geometry().contains(titleBarPos);
                maximizeButton->setNativeHovered(isOverMaximize);
                if (isOverMaximize)
                {
                    *result = HTMAXBUTTON;
                    return true;
                }
                *result = _titleBar->isCaptionAt(titleBarPos) ? HTCAPTION : HTCLIENT;
                return true;
            }
        case WM_NCMOUSEMOVE:
            {
                const bool isOverMaximize = msg->wParam == HTMAXBUTTON;
                maximizeButton->setNativeHovered(isOverMaximize);
                if (isOverMaximize)
                {
                    // 请求 WM_NCMOUSELEAVE，鼠标从最大化按钮直接移出窗口时也能清除悬停
                    TRACKMOUSEEVENT track{};
                    track.cbSize = sizeof(track);
                    track.dwFlags = TME_LEAVE | TME_NONCLIENT;
                    track.hwndTrack = hwnd;
                    ::TrackMouseEvent(&track);
                }
                return false;
            }
        case WM_NCMOUSELEAVE:
            {
                maximizeButton->setNativeHovered(false);
                maximizeButton->setNativePressed(false);
                return false;
            }
        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONDBLCLK:
            {
                // 最大化按钮的点击由这里接管，交给系统会绘制经典样式的按钮
                if (msg->wParam == HTMAXBUTTON)
                {
                    maximizeButton->setNativePressed(true);
                    *result = 0;
                    return true;
                }
                // 隐藏最大化按钮或固定尺寸时，双击标题栏不最大化
                if (msg->message == WM_NCLBUTTONDBLCLK && msg->wParam == HTCAPTION && !_titleBar->canMaximize())
                {
                    *result = 0;
                    return true;
                }
                return false;
            }
        case WM_NCLBUTTONUP:
            {
                const bool wasPressed = maximizeButton->isNativePressed();
                maximizeButton->setNativePressed(false);
                if (msg->wParam == HTMAXBUTTON)
                {
                    if (wasPressed)
                    {
                        _titleBar->toggleMaximize();
                    }
                    *result = 0;
                    return true;
                }
                return false;
            }
        case WM_NCRBUTTONUP:
            {
                // 标题栏位于客户区内，Qt 会把右键转成 QContextMenuEvent，系统菜单需要自己弹出
                if (msg->wParam == HTCAPTION)
                {
                    showSystemMenu(GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
                    *result = 0;
                    return true;
                }
                return false;
            }
        case WM_NCACTIVATE:
            {
                // lParam 传 -1 阻止系统重绘非客户区，避免窗口失去激活时闪出系统标题栏
                *result = ::DefWindowProcW(hwnd, WM_NCACTIVATE, msg->wParam, -1);
                return true;
            }
        case WM_STYLECHANGING:
            {
                // Qt 修改窗口标志时按无边框窗口重写样式（例如切换置顶），Windows 10 在这里补回系统窗口样式
                // 全屏时 Qt 有意去掉边框样式，不补；wParam 只比较低 32 位，GWL_STYLE 为负数
                if (_isWindows10 && static_cast<int>(msg->wParam) == GWL_STYLE && !_window->isFullScreen())
                {
                    STYLESTRUCT* styleStruct = reinterpret_cast<STYLESTRUCT*>(msg->lParam);
                    styleStruct->styleNew |= windows10StyleBits(_titleBar->getButtonFlags());
                }
                return false;
            }
        case WM_WINDOWPOSCHANGING:
            {
                // 带缩放边框的窗口最大化时系统让四周超出屏幕一个边框厚度，Windows 10 的分层窗口会因此裁掉内容边缘
                // 这里把最大化的窗口收回到所在屏幕的工作区，同时不遮挡任务栏；只修改尺寸与位置，其余交给 Qt 与系统
                WINDOWPOS* windowPos = reinterpret_cast<WINDOWPOS*>(msg->lParam);
                if (!_isWindows10 || windowPos == nullptr || (windowPos->flags & (SWP_NOMOVE | SWP_NOSIZE)) != 0 || !::IsZoomed(hwnd) || _window->isFullScreen())
                {
                    return false;
                }
                const RECT proposedRect{windowPos->x, windowPos->y, windowPos->x + windowPos->cx, windowPos->y + windowPos->cy};
                MONITORINFO monitorInfo{};
                monitorInfo.cbSize = sizeof(monitorInfo);
                if (!::GetMonitorInfoW(::MonitorFromRect(&proposedRect, MONITOR_DEFAULTTONEAREST), &monitorInfo))
                {
                    qWarning("获取窗口所在屏幕信息失败，GetMonitorInfoW 错误码: %lu", ::GetLastError());
                    return false;
                }
                const RECT& workRect = monitorInfo.rcWork;
                windowPos->x = workRect.left;
                windowPos->y = workRect.top;
                windowPos->cx = workRect.right - workRect.left;
                windowPos->cy = workRect.bottom - workRect.top;
                return false;
            }
        case WM_WINDOWPOSCHANGED:
            {
                // 显示、隐藏、移动、缩放、最小化与还原都会经过这里，阴影随之更新；消息继续交给 Qt 处理
                if (_isWindows10)
                {
                    updateShadow();
                }
                if ((reinterpret_cast<const WINDOWPOS*>(msg->lParam)->flags & SWP_SHOWWINDOW) != 0)
                {
                    applyWindowEffect();
                }
                return false;
            }
        default:
            {
                return false;
            }
    }
}

void ShadWindowFrame::applyNativeFrame()
{
    const WId winId = _window->winId();
    if (winId == _framedWinId)
    {
        return;
    }
    _framedWinId = winId;
    const HWND hwnd = reinterpret_cast<HWND>(winId);
    if (_isWindows10)
    {
        // 全屏时 Qt 有意去掉边框样式，退出全屏时 Qt 写回样式，由 WM_STYLECHANGING 补上
        if (_window->isFullScreen())
        {
            return;
        }
        const LONG_PTR style = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
        const LONG_PTR newStyle = style | static_cast<LONG_PTR>(windows10StyleBits(_titleBar->getButtonFlags()));
        if (newStyle == style)
        {
            return;
        }
        ::SetLastError(0);
        if (::SetWindowLongPtrW(hwnd, GWL_STYLE, newStyle) == 0 && ::GetLastError() != 0)
        {
            qWarning("补回窗口样式失败，SetWindowLongPtrW 错误码: %lu", ::GetLastError());
            return;
        }
        // 样式改变后需要 SWP_FRAMECHANGED 才会生效；客户区占满整个窗口，尺寸不变
        if (!::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE))
        {
            qWarning("刷新窗口样式失败，SetWindowPos 错误码: %lu", ::GetLastError());
        }
        return;
    }
    UINT flags = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE;
    int width = 0;
    int height = 0;
    if (::IsZoomed(hwnd) || _window->isFullScreen())
    {
        flags |= SWP_NOSIZE;
    }
    else
    {
        // 去掉标题栏后客户区会变高，按原客户区高度加底部边框重新设置窗口高度，保持调用方设置的尺寸
        RECT windowRect{};
        RECT clientRect{};
        ::GetWindowRect(hwnd, &windowRect);
        ::GetClientRect(hwnd, &clientRect);
        width = windowRect.right - windowRect.left;
        height = (clientRect.bottom - clientRect.top) + resizeBorderThickness(hwnd, false);
    }
    if (!::SetWindowPos(hwnd, nullptr, 0, 0, width, height, flags))
    {
        qWarning("去除系统标题栏失败，SetWindowPos 错误码: %lu", ::GetLastError());
    }
}

void ShadWindowFrame::updateNativeDarkMode()
{
    if (_framedWinId == 0 || _isWindows10)
    {
        return;
    }
    const BOOL isDark = shadTheme->isDark() ? TRUE : FALSE;
    const HRESULT hr = ::DwmSetWindowAttribute(reinterpret_cast<HWND>(_framedWinId), kDwmUseImmersiveDarkMode, &isDark, sizeof(isDark));
    if (FAILED(hr))
    {
        qWarning("设置窗口边框深色模式失败，HRESULT: 0x%08lx", static_cast<unsigned long>(hr));
    }
}

void ShadWindowFrame::showSystemMenu(int nativeX, int nativeY)
{
    const HWND hwnd = reinterpret_cast<HWND>(_window->winId());
    const HMENU menu = ::GetSystemMenu(hwnd, FALSE);
    if (menu == nullptr)
    {
        return;
    }
    const bool isMaximized = _window->isMaximized();
    const bool isFixedSize = _window->minimumSize() == _window->maximumSize() || _window->property(kResizeDisabledProperty).toBool();
    const ShadWindowType::ButtonFlags buttonFlags = _titleBar->getButtonFlags();
    auto setItemEnabled = [menu](UINT item, bool isEnabled) { ::EnableMenuItem(menu, item, MF_BYCOMMAND | (isEnabled ? MF_ENABLED : MF_GRAYED)); };
    setItemEnabled(SC_RESTORE, isMaximized);
    setItemEnabled(SC_MOVE, !isMaximized);
    setItemEnabled(SC_SIZE, !isMaximized && !isFixedSize);
    setItemEnabled(SC_MINIMIZE, buttonFlags.testFlag(ShadWindowType::MinimizeButton));
    setItemEnabled(SC_MAXIMIZE, !isMaximized && _titleBar->canMaximize());
    setItemEnabled(SC_CLOSE, buttonFlags.testFlag(ShadWindowType::CloseButton));
    const UINT alignment = QGuiApplication::isRightToLeft() ? TPM_RIGHTALIGN : TPM_LEFTALIGN;
    const BOOL command = ::TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | alignment, nativeX, nativeY, 0, hwnd, nullptr);
    if (command != 0)
    {
        ::PostMessageW(hwnd, WM_SYSCOMMAND, static_cast<WPARAM>(command), 0);
    }
}

void ShadWindowFrame::updateShadow()
{
    if (_shadow == nullptr)
    {
        return;
    }
    // 对话框淡入淡出、屏幕取色时隐藏窗口、停靠窗口拖动都会修改窗口不透明度，阴影需要同步
    // QWidget::setWindowOpacity 最终调用 QWindow::setOpacity 并发出 opacityChanged
    QWindow* handle = _window->windowHandle();
    if (handle != _opacityWindow)
    {
        QObject::disconnect(_opacityConnection);
        _opacityWindow = handle;
        if (handle != nullptr)
        {
            _opacityConnection = connect(handle, &QWindow::opacityChanged, this, &ShadWindowFrame::updateShadow);
        }
    }
    // 圆角半径与 cornerRadius() 不同：最大化时 cornerRadius() 为 0，但此时阴影本来就会隐藏
    _shadow->update(_window->internalWinId(), !_window->isFullScreen(), baseCornerRadius(), _window->devicePixelRatioF(), _window->windowOpacity(), _window->isActiveWindow());
}
#else
bool ShadWindowFrame::handleResizeEvent(QEvent* event)
{
    switch (event->type())
    {
        case QEvent::MouseMove:
            {
                const QMouseEvent* mouseEvent = static_cast<const QMouseEvent*>(event);
#if defined(Q_OS_MACOS) || defined(Q_OS_WASM)
                if (_resizeEdges != Qt::Edges())
                {
                    applyManualResize(mouseEvent->globalPosition().toPoint());
                    return true;
                }
#endif
                if (mouseEvent->buttons() == Qt::NoButton)
                {
                    setHoverEdges(edgesAt(mouseEvent->position().toPoint()));
                }
                return false;
            }
        case QEvent::MouseButtonPress:
            {
                const QMouseEvent* mouseEvent = static_cast<const QMouseEvent*>(event);
                const Qt::Edges edges = edgesAt(mouseEvent->position().toPoint());
                if (mouseEvent->button() != Qt::LeftButton || edges == Qt::Edges())
                {
                    return false;
                }
#if defined(Q_OS_MACOS) || defined(Q_OS_WASM)
                _resizeEdges = edges;
                _resizeStartGeometry = _window->geometry();
                _resizeStartPos = mouseEvent->globalPosition().toPoint();
                return true;
#else
                QWindow* handle = _window->windowHandle();
                return handle != nullptr && handle->startSystemResize(edges);
#endif
            }
#if defined(Q_OS_MACOS) || defined(Q_OS_WASM)
        case QEvent::MouseButtonRelease:
            {
                if (_resizeEdges == Qt::Edges())
                {
                    return false;
                }
                _resizeEdges = Qt::Edges();
                setHoverEdges(edgesAt(static_cast<const QMouseEvent*>(event)->position().toPoint()));
                return true;
            }
        case QEvent::Leave:
            {
                // 手动缩放过程中鼠标可能移出窗口，保持缩放光标直到松开
                if (_resizeEdges == Qt::Edges())
                {
                    setHoverEdges(Qt::Edges());
                }
                return false;
            }
#else
        case QEvent::Leave:
            {
                setHoverEdges(Qt::Edges());
                return false;
            }
#endif
        default:
            {
                return false;
            }
    }
}

void ShadWindowFrame::setHoverEdges(Qt::Edges edges)
{
    if (_hoverEdges == edges)
    {
        return;
    }
    _hoverEdges = edges;
    if (edges == Qt::Edges())
    {
        if (_isOverrideCursor)
        {
            QGuiApplication::restoreOverrideCursor();
            _isOverrideCursor = false;
        }
        return;
    }
    // 子组件可能设置了自己的光标，使用 override cursor 保证边缘处显示缩放光标
    const QCursor cursor(cursorShapeForEdges(edges));
    if (_isOverrideCursor)
    {
        QGuiApplication::changeOverrideCursor(cursor);
    }
    else
    {
        QGuiApplication::setOverrideCursor(cursor);
        _isOverrideCursor = true;
    }
}

#if defined(Q_OS_MACOS) || defined(Q_OS_WASM)
void ShadWindowFrame::applyManualResize(const QPoint& globalPos)
{
    const QPoint delta = globalPos - _resizeStartPos;
    const QSize minimumSize = _window->minimumSize().expandedTo(_window->minimumSizeHint());
    const QSize maximumSize = _window->maximumSize();
    QRect geometry = _resizeStartGeometry;
    // 拖动左边与上边时保持右边与下边不动
    if (_resizeEdges.testFlag(Qt::LeftEdge))
    {
        const int width = qBound(minimumSize.width(), _resizeStartGeometry.width() - delta.x(), maximumSize.width());
        geometry.setLeft(_resizeStartGeometry.right() - width + 1);
    }
    else if (_resizeEdges.testFlag(Qt::RightEdge))
    {
        geometry.setWidth(qBound(minimumSize.width(), _resizeStartGeometry.width() + delta.x(), maximumSize.width()));
    }
    if (_resizeEdges.testFlag(Qt::TopEdge))
    {
        const int height = qBound(minimumSize.height(), _resizeStartGeometry.height() - delta.y(), maximumSize.height());
        geometry.setTop(_resizeStartGeometry.bottom() - height + 1);
    }
    else if (_resizeEdges.testFlag(Qt::BottomEdge))
    {
        geometry.setHeight(qBound(minimumSize.height(), _resizeStartGeometry.height() + delta.y(), maximumSize.height()));
    }
    if (geometry == _window->geometry())
    {
        return;
    }
    // QWidget::setGeometry 先调整原生窗口再发送 QResizeEvent 触发布局，而 macOS 调整原生窗口时会同步重绘整个窗口
    // （setFrame:display:YES，expose 为 SynchronousDelivery），这次重绘用的仍是旧布局，新布局要等之后的异步 update 才画出来，
    // 拖动中每次移动都会先画一帧旧布局，表现为内容跟不上窗口尺寸；先按新尺寸完成布局，让同步重绘直接画出新布局
    if (QLayout* rootLayout = _window->layout())
    {
        rootLayout->setGeometry(QRect(QPoint(0, 0), geometry.size()).marginsRemoved(_window->contentsMargins()));
    }
    _window->setGeometry(geometry);
}
#endif
#endif
