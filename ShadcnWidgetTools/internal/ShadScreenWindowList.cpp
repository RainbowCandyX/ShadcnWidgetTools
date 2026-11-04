#include "ShadScreenWindowList.h"

#include <QList>
#include <QRect>
#include <QtGlobal>

#if defined(Q_OS_MACOS)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <QGuiApplication>
#include <QPoint>
#include <QPointF>
#include <QScreen>
#include <QSize>
#include <QString>
#include <QtGui/qscreen_platform.h>

// dwmapi.h 依赖 windows.h 中的类型定义，必须先包含 windows.h
#include <windows.h>
#include <dwmapi.h>
#endif

namespace
{
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    // 小于该尺寸的窗口视为看不见的辅助窗口
    constexpr int kMinimumWindowSize = 8;
#endif

#if defined(Q_OS_MACOS)
    bool readNumber(CFDictionaryRef info, CFStringRef key, CFNumberType type, void* value)
    {
        const CFNumberRef number = static_cast<CFNumberRef>(CFDictionaryGetValue(info, key));
        return number != nullptr && CFGetTypeID(number) == CFNumberGetTypeID() && CFNumberGetValue(number, type, value);
    }
#elif defined(Q_OS_WIN)
    // 桌面由 Progman / WorkerW 绘制，覆盖整块屏幕
    bool isDesktopWindow(HWND hwnd)
    {
        if (hwnd == GetShellWindow())
        {
            return true;
        }
        wchar_t className[64]{};
        const int length = GetClassNameW(hwnd, className, int(sizeof(className) / sizeof(className[0])));
        const QString name = QString::fromWCharArray(className, qMax(0, length));
        return name == QStringLiteral("Progman") || name == QStringLiteral("WorkerW");
    }

    // 物理像素矩形按所在显示器的缩放比例换算为全局逻辑坐标
    QRect toLogicalRect(const RECT& bounds)
    {
        const HMONITOR monitor = MonitorFromRect(&bounds, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);
        if (monitor == nullptr || !GetMonitorInfoW(monitor, &monitorInfo))
        {
            return QRect();
        }
        for (QScreen* screen : QGuiApplication::screens())
        {
            const QNativeInterface::QWindowsScreen* nativeScreen = screen->nativeInterface<QNativeInterface::QWindowsScreen>();
            if (nativeScreen == nullptr || nativeScreen->handle() != monitor)
            {
                continue;
            }
            const qreal ratio = screen->devicePixelRatio();
            const QPointF offset(bounds.left - monitorInfo.rcMonitor.left, bounds.top - monitorInfo.rcMonitor.top);
            const QPointF topLeft = QPointF(screen->geometry().topLeft()) + offset / ratio;
            const QSize size(qRound((bounds.right - bounds.left) / ratio), qRound((bounds.bottom - bounds.top) / ratio));
            return QRect(topLeft.toPoint(), size);
        }
        return QRect();
    }

    struct CollectContext
    {
        QList<QRect>* rects{nullptr};
        bool includeOwnWindows{true};
    };

BOOL CALLBACK collectWindow(HWND hwnd, LPARAM parameter)
    {
        const CollectContext* context = reinterpret_cast<const CollectContext*>(parameter);
        if (!IsWindowVisible(hwnd) || IsIconic(hwnd) || isDesktopWindow(hwnd))
        {
            return TRUE;
        }
        if (!context->includeOwnWindows)
        {
            DWORD processId = 0;
            GetWindowThreadProcessId(hwnd, &processId);
            if (processId == GetCurrentProcessId())
            {
                return TRUE;
            }
        }
        if ((GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) != 0)
        {
            return TRUE;
        }
        // 被 DWM 隐藏的窗口（其他虚拟桌面等）IsWindowVisible 仍为 true
        DWORD cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked != 0)
        {
            return TRUE;
        }
        // DWMWA_EXTENDED_FRAME_BOUNDS 不含窗口阴影，取不到时退回 GetWindowRect
        RECT bounds{};
        if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &bounds, sizeof(bounds))) && !GetWindowRect(hwnd, &bounds))
        {
            return TRUE;
        }
        const QRect rect = toLogicalRect(bounds);
        if (rect.width() >= kMinimumWindowSize && rect.height() >= kMinimumWindowSize)
        {
            context->rects->append(rect);
        }
        return TRUE;
    }
#endif
} // namespace

namespace ShadScreenWindowList
{
    QList<QRect> windowRects(bool includeOwnWindows)
    {
        QList<QRect> rects;
#if defined(Q_OS_MACOS)
        const pid_t ownProcessId = getpid();
        // 坐标以主屏左上角为原点、单位为点，与 Qt 全局逻辑坐标一致
        const CFArrayRef windows = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
        if (windows == nullptr)
        {
            return rects;
        }
        const CFIndex count = CFArrayGetCount(windows);
        for (CFIndex index = 0; index < count; ++index)
        {
            const CFDictionaryRef info = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windows, index));
            // 第 0 层为普通窗口，菜单栏、程序坞等在更高的层
            int layer = -1;
            if (!readNumber(info, kCGWindowLayer, kCFNumberIntType, &layer) || layer != 0)
            {
                continue;
            }
            if (!includeOwnWindows)
            {
                int ownerProcessId = 0;
                if (readNumber(info, kCGWindowOwnerPID, kCFNumberIntType, &ownerProcessId) && ownerProcessId == ownProcessId)
                {
                    continue;
                }
            }
            double alpha = 1.0;
            if (readNumber(info, kCGWindowAlpha, kCFNumberDoubleType, &alpha) && alpha <= 0.0)
            {
                continue;
            }
            const CFDictionaryRef boundsDictionary = static_cast<CFDictionaryRef>(CFDictionaryGetValue(info, kCGWindowBounds));
            CGRect bounds = CGRectNull;
            if (boundsDictionary == nullptr || !CGRectMakeWithDictionaryRepresentation(boundsDictionary, &bounds))
            {
                continue;
            }
            const QRect rect(qRound(bounds.origin.x), qRound(bounds.origin.y), qRound(bounds.size.width), qRound(bounds.size.height));
            if (rect.width() >= kMinimumWindowSize && rect.height() >= kMinimumWindowSize)
            {
                rects.append(rect);
            }
        }
        CFRelease(windows);
#elif defined(Q_OS_WIN)
        CollectContext context;
        context.rects = &rects;
        context.includeOwnWindows = includeOwnWindows;
        EnumWindows(collectWindow, reinterpret_cast<LPARAM>(&context));
#else
        Q_UNUSED(includeOwnWindows);
#endif
        return rects;
    }
} // namespace ShadScreenWindowList
