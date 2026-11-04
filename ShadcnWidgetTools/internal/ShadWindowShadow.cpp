#include "ShadWindowShadow.h"

#ifdef Q_OS_WIN
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QtMath>

#include <cmath>
#include <cstddef>
#include <cwchar>
#include <iterator>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace
{
    constexpr wchar_t kShadowClassName[] = L"ShadWindowShadow";

    // 注册阴影窗口的窗口类，进程内只注册一次；同名窗口类已存在时直接使用
    bool registerShadowClass()
    {
        static const bool isRegistered = []()
        {
            WNDCLASSEXW windowClass{};
            windowClass.cbSize = sizeof(windowClass);
            windowClass.lpfnWndProc = ::DefWindowProcW;
            windowClass.hInstance = ::GetModuleHandleW(nullptr);
            windowClass.lpszClassName = kShadowClassName;
            if (::RegisterClassExW(&windowClass) == 0)
            {
                const DWORD error = ::GetLastError();
                if (error != ERROR_CLASS_ALREADY_EXISTS)
                {
                    qWarning("注册窗口阴影的窗口类失败，RegisterClassExW 错误码: %lu", error);
                    return false;
                }
            }
            return true;
        }();
        return isRegistered;
    }

    // 主窗口销毁时系统会连带销毁它拥有的阴影窗口，句柄之后可能被复用；只操作本进程内阴影窗口类的窗口
    bool isShadowWindow(HWND hwnd)
    {
        if (hwnd == nullptr || !::IsWindow(hwnd))
        {
            return false;
        }
        DWORD processId = 0;
        ::GetWindowThreadProcessId(hwnd, &processId);
        if (processId != ::GetCurrentProcessId())
        {
            return false;
        }
        wchar_t className[64] = {};
        if (::GetClassNameW(hwnd, className, static_cast<int>(std::size(className))) == 0)
        {
            return false;
        }
        return std::wcscmp(className, kShadowClassName) == 0;
    }
} // namespace

ShadWindowShadow::ShadWindowShadow()
{
}

ShadWindowShadow::~ShadWindowShadow()
{
    destroyWindow();
}

void ShadWindowShadow::update(quintptr ownerWinId, bool isAllowed, int cornerRadius, qreal devicePixelRatio, qreal opacity, bool isActive)
{
    const HWND owner = reinterpret_cast<HWND>(ownerWinId);
    if (!isAllowed || owner == nullptr || !::IsWindow(owner) || !::IsWindowVisible(owner) || ::IsIconic(owner) || ::IsZoomed(owner))
    {
        hide();
        return;
    }
    if (!ensureWindow(ownerWinId))
    {
        return;
    }
    RECT ownerRect{};
    if (!::GetWindowRect(owner, &ownerRect))
    {
        qWarning("获取窗口位置失败，GetWindowRect 错误码: %lu", ::GetLastError());
        hide();
        return;
    }
    const int margin = qCeil(kMargin * devicePixelRatio);
    const QSize ownerSize(ownerRect.right - ownerRect.left, ownerRect.bottom - ownerRect.top);
    const QRect shadowRect(ownerRect.left - margin, ownerRect.top - margin, ownerSize.width() + 2 * margin, ownerSize.height() + 2 * margin);
    const int alpha = qBound(0, qRound(255.0 * opacity * (isActive ? 1.0 : kInactiveOpacity)), 255);
    const HWND hwnd = reinterpret_cast<HWND>(_hwnd);
    if (ownerSize != _renderedOwnerSize || cornerRadius != _renderedRadius || !qFuzzyCompare(devicePixelRatio, _renderedDevicePixelRatio))
    {
        if (!render(shadowRect, margin, cornerRadius * devicePixelRatio, devicePixelRatio, alpha))
        {
            hide();
            return;
        }
        _renderedOwnerSize = ownerSize;
        _renderedRadius = cornerRadius;
        _renderedDevicePixelRatio = devicePixelRatio;
    }
    else if (shadowRect.topLeft() != _shadowRect.topLeft() || alpha != _alpha)
    {
        // 内容不变，只移动位置或调整整体不透明度
        POINT position{shadowRect.x(), shadowRect.y()};
        BLENDFUNCTION blend{AC_SRC_OVER, 0, static_cast<BYTE>(alpha), AC_SRC_ALPHA};
        if (!::UpdateLayeredWindow(hwnd, nullptr, &position, nullptr, nullptr, nullptr, 0, &blend, ULW_ALPHA))
        {
            qWarning("更新窗口阴影失败，UpdateLayeredWindow 错误码: %lu", ::GetLastError());
        }
    }
    _shadowRect = shadowRect;
    _alpha = alpha;
    if (!_isShown)
    {
        ::ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        _isShown = true;
    }
}

void ShadWindowShadow::hide()
{
    if (!_isShown)
    {
        return;
    }
    _isShown = false;
    const HWND hwnd = reinterpret_cast<HWND>(_hwnd);
    if (isShadowWindow(hwnd))
    {
        ::ShowWindow(hwnd, SW_HIDE);
    }
}

bool ShadWindowShadow::ensureWindow(quintptr ownerWinId)
{
    if (_ownerWinId == ownerWinId && isShadowWindow(reinterpret_cast<HWND>(_hwnd)))
    {
        return true;
    }
    destroyWindow();
    if (!registerShadowClass())
    {
        return false;
    }
    // WS_EX_TRANSPARENT 让鼠标穿透到下方窗口，WS_EX_TOOLWINDOW 不显示在任务栏与 Alt+Tab 中，WS_EX_NOACTIVATE 不抢焦点
    const HWND hwnd = ::CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kShadowClassName, L"", WS_POPUP, 0, 0, 0, 0, reinterpret_cast<HWND>(ownerWinId), nullptr, ::GetModuleHandleW(nullptr), nullptr);
    if (hwnd == nullptr)
    {
        qWarning("创建窗口阴影失败，CreateWindowExW 错误码: %lu", ::GetLastError());
        return false;
    }
    _hwnd = reinterpret_cast<quintptr>(hwnd);
    _ownerWinId = ownerWinId;
    return true;
}

void ShadWindowShadow::destroyWindow()
{
    const HWND hwnd = reinterpret_cast<HWND>(_hwnd);
    if (isShadowWindow(hwnd) && !::DestroyWindow(hwnd))
    {
        qWarning("销毁窗口阴影失败，DestroyWindow 错误码: %lu", ::GetLastError());
    }
    _hwnd = 0;
    _ownerWinId = 0;
    _shadowRect = QRect();
    _renderedOwnerSize = QSize();
    _renderedRadius = -1;
    _renderedDevicePixelRatio = 0.0;
    _alpha = -1;
    _isShown = false;
}

bool ShadWindowShadow::render(const QRect& shadowRect, int margin, qreal radius, qreal devicePixelRatio, int alpha)
{
    const int width = shadowRect.width();
    const int height = shadowRect.height();
    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    // 高度取负值表示自上而下存储，与 QImage 的行序一致
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    const HDC screenDc = ::GetDC(nullptr);
    if (screenDc == nullptr)
    {
        qWarning("获取屏幕设备上下文失败，GetDC 错误码: %lu", ::GetLastError());
        return false;
    }
    const HDC memoryDc = ::CreateCompatibleDC(screenDc);
    void* bits = nullptr;
    const HBITMAP bitmap = memoryDc != nullptr ? ::CreateDIBSection(memoryDc, &bitmapInfo, DIB_RGB_COLORS, &bits, nullptr, 0) : nullptr;
    bool isUpdated = false;
    if (bitmap != nullptr && bits != nullptr)
    {
        const HGDIOBJ oldBitmap = ::SelectObject(memoryDc, bitmap);
        {
            // 32 位 DIB 的 BGRA 字节序与 Format_ARGB32_Premultiplied 一致，UpdateLayeredWindow 要求预乘 alpha
            QImage image(static_cast<uchar*>(bits), width, height, width * 4, QImage::Format_ARGB32_Premultiplied);
            paintShadow(image, margin, radius, devicePixelRatio);
        }
        POINT position{shadowRect.x(), shadowRect.y()};
        SIZE size{width, height};
        POINT source{0, 0};
        BLENDFUNCTION blend{AC_SRC_OVER, 0, static_cast<BYTE>(alpha), AC_SRC_ALPHA};
        isUpdated = ::UpdateLayeredWindow(reinterpret_cast<HWND>(_hwnd), screenDc, &position, &size, memoryDc, &source, 0, &blend, ULW_ALPHA) != FALSE;
        if (!isUpdated)
        {
            qWarning("绘制窗口阴影失败，UpdateLayeredWindow 错误码: %lu", ::GetLastError());
        }
        ::SelectObject(memoryDc, oldBitmap);
    }
    else
    {
        qWarning("创建窗口阴影位图失败，错误码: %lu", ::GetLastError());
    }
    if (bitmap != nullptr)
    {
        ::DeleteObject(bitmap);
    }
    if (memoryDc != nullptr)
    {
        ::DeleteDC(memoryDc);
    }
    ::ReleaseDC(nullptr, screenDc);
    return isUpdated;
}

void ShadWindowShadow::paintShadow(QImage& image, int margin, qreal radius, qreal devicePixelRatio)
{
    image.fill(Qt::transparent);
    const int ownerWidth = image.width() - 2 * margin;
    const int ownerHeight = image.height() - 2 * margin;
    if (ownerWidth <= 0 || ownerHeight <= 0)
    {
        return;
    }
    // 阴影形状为窗口圆角矩形向下偏移 kOffsetY，按到形状边缘的有符号距离 d（外正内负）计算覆盖率
    // 圆角矩形经高斯模糊后，边缘附近的覆盖率为 erfc(d / (σ√2)) / 2
    const qreal halfWidth = ownerWidth / 2.0;
    const qreal halfHeight = ownerHeight / 2.0;
    const qreal shapeRadius = qBound(0.0, radius, qMin(halfWidth, halfHeight));
    const qreal innerHalfWidth = halfWidth - shapeRadius;
    const qreal innerHalfHeight = halfHeight - shapeRadius;
    const qreal centerX = margin + halfWidth;
    const qreal centerY = margin + halfHeight + kOffsetY * devicePixelRatio;
    const qreal distanceScale = 1.0 / (kBlur * devicePixelRatio * std::sqrt(2.0));
    const qreal maxAlpha = 255.0 * kAlpha;
    const auto shadowPixel = [=](qreal px, qreal py) -> quint32
    {
        const qreal qx = qAbs(px) - innerHalfWidth;
        const qreal qy = qAbs(py) - innerHalfHeight;
        const qreal distance = std::hypot(qMax(qx, 0.0), qMax(qy, 0.0)) + qMin(qMax(qx, qy), 0.0) - shapeRadius;
        const int alpha = qBound(0, qRound(maxAlpha * 0.5 * std::erfc(distance * distanceScale)), 255);
        // 黑色预乘后只剩 alpha 分量
        return static_cast<quint32>(alpha) << 24;
    };
    const int width = image.width();
    const int rightStart = margin + ownerWidth;
    // 窗口直边两侧的阴影只与 x 有关，算一行后复用
    std::vector<quint32> sideRow(static_cast<std::size_t>(width), 0);
    for (int x = 0; x < width; ++x)
    {
        if (x < margin || x >= rightStart)
        {
            sideRow[static_cast<std::size_t>(x)] = shadowPixel(x + 0.5 - centerX, 0.0);
        }
    }
    for (int y = 0; y < image.height(); ++y)
    {
        quint32* line = reinterpret_cast<quint32*>(image.scanLine(y));
        const qreal py = y + 0.5 - centerY;
        if (y >= margin && y < margin + ownerHeight && qAbs(py) <= innerHalfHeight)
        {
            // 窗口范围内的像素随后会被清除，只填左右两侧
            for (int x = 0; x < margin; ++x)
            {
                line[x] = sideRow[static_cast<std::size_t>(x)];
            }
            for (int x = rightStart; x < width; ++x)
            {
                line[x] = sideRow[static_cast<std::size_t>(x)];
            }
            continue;
        }
        for (int x = 0; x < width; ++x)
        {
            line[x] = shadowPixel(x + 0.5 - centerX, py);
        }
    }
    // 清除窗口圆角矩形内的阴影：阴影窗口位于主窗口之上，不能遮住窗口内容；边缘按覆盖率部分清除，与窗口自身的抗锯齿边缘衔接
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    QPainterPath windowPath;
    windowPath.addRoundedRect(QRectF(margin, margin, ownerWidth, ownerHeight), radius, radius);
    painter.fillPath(windowPath, Qt::transparent);
}
#endif
