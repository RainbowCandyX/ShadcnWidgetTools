#include "ShadWindowEffect.h"

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QScreen>
#include <QSizeF>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <vector>

#include "ShadStyleHelper.h"

#ifdef Q_OS_WIN
#include <QOperatingSystemVersion>
#include <QSurface>
#include <QWindow>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#endif

namespace
{
    constexpr int kWallpaperWidth = 1920;
    // 先缩小到该模糊半径再模糊
    constexpr int kWorkRadius = 8;
    constexpr int kMinWorkWidth = 64;
    constexpr int kBlurPasses = 3;
    constexpr qreal kWallpaperOpacity = 0.7;

    struct WallpaperCache
    {
        QImage source;
        QImage loaded;
        QImage blurred;
        int blurRadius{ShadWindowEffect::kDefaultBlurRadius};
        bool isLoaded{false};
        bool isBlurred{false};
    };

    WallpaperCache& wallpaperCache()
    {
        static WallpaperCache cache;
        return cache;
    }

    void boxBlur(QImage& image, int radius, bool isHorizontal)
    {
        const int width = image.width();
        const int height = image.height();
        const int length = isHorizontal ? width : height;
        const int lines = isHorizontal ? height : width;
        const int window = 2 * radius + 1;
        const qsizetype stride = image.bytesPerLine() / static_cast<qsizetype>(sizeof(QRgb));
        QRgb* pixels = reinterpret_cast<QRgb*>(image.bits());
        std::vector<QRgb> buffer(static_cast<size_t>(length));
        for (int line = 0; line < lines; ++line)
        {
            auto pixelAt = [&](int index) -> QRgb& { return isHorizontal ? pixels[line * stride + index] : pixels[index * stride + line]; };
            for (int i = 0; i < length; ++i)
            {
                buffer[static_cast<size_t>(i)] = pixelAt(i);
            }
            int red = 0;
            int green = 0;
            int blue = 0;
            for (int i = -radius; i <= radius; ++i)
            {
                const QRgb pixel = buffer[static_cast<size_t>(qBound(0, i, length - 1))];
                red += qRed(pixel);
                green += qGreen(pixel);
                blue += qBlue(pixel);
            }
            for (int i = 0; i < length; ++i)
            {
                pixelAt(i) = qRgb(red / window, green / window, blue / window);
                const QRgb added = buffer[static_cast<size_t>(qMin(i + radius + 1, length - 1))];
                const QRgb removed = buffer[static_cast<size_t>(qMax(i - radius, 0))];
                red += qRed(added) - qRed(removed);
                green += qGreen(added) - qGreen(removed);
                blue += qBlue(added) - qBlue(removed);
            }
        }
    }

    QImage prepareWallpaper(const QImage& source)
    {
        if (source.isNull())
        {
            return QImage();
        }
        const QImage image = source.width() > kWallpaperWidth ? source.scaledToWidth(kWallpaperWidth, Qt::SmoothTransformation) : source;
        return image.convertToFormat(QImage::Format_RGB32);
    }

    // 多次盒式模糊近似高斯模糊
    QImage blurImage(const QImage& source, int radius)
    {
        if (source.isNull() || radius <= 0)
        {
            return source;
        }
        const qreal scale = static_cast<qreal>(source.width()) / kWallpaperWidth;
        const int sourceRadius = qMax(1, qRound(radius * scale));
        const int workWidth = qBound(qMin(kMinWorkWidth, source.width()), source.width() * kWorkRadius / sourceRadius, source.width());
        QImage image = source.scaledToWidth(workWidth, Qt::SmoothTransformation).convertToFormat(QImage::Format_RGB32);
        if (image.isNull() || image.height() <= 0)
        {
            return QImage();
        }
        const int workRadius = qMax(1, qRound(static_cast<qreal>(sourceRadius) * workWidth / source.width()));
        for (int pass = 0; pass < kBlurPasses; ++pass)
        {
            boxBlur(image, workRadius, true);
            boxBlur(image, workRadius, false);
        }
        return image;
    }

#ifdef Q_OS_WIN
    constexpr int kWindows11Build = 22000;
    constexpr int kWindows11Build22H2 = 22621;

    // 旧版 SDK 没有这些常量
    constexpr DWORD kDwmSystemBackdropType = 38;
    constexpr DWORD kDwmMicaEffect = 1029;
    constexpr int kBackdropAuto = 0;
    constexpr int kBackdropMainWindow = 2;
    constexpr int kBackdropTransientWindow = 3;
    constexpr int kBackdropTabbedWindow = 4;

    constexpr DWORD kWcaAccentPolicy = 19;
    constexpr int kAccentDisabled = 0;
    constexpr int kAccentEnableBlurBehind = 3;

    struct AccentPolicy
    {
        int accentState;
        int accentFlags;
        DWORD gradientColor;
        int animationId;
    };

    struct WindowCompositionAttributeData
    {
        DWORD attribute;
        PVOID data;
        SIZE_T size;
    };

    using SetWindowCompositionAttributeFunc = BOOL(WINAPI*)(HWND, WindowCompositionAttributeData*);

    int windowsBuild()
    {
        return QOperatingSystemVersion::current().microVersion();
    }

    void setDwmAttribute(HWND hwnd, DWORD attribute, const void* value, DWORD size)
    {
        const HRESULT hr = ::DwmSetWindowAttribute(hwnd, attribute, value, size);
        if (FAILED(hr))
        {
            qWarning("设置窗口特效失败，属性 %lu，HRESULT: 0x%08lx", static_cast<unsigned long>(attribute), static_cast<unsigned long>(hr));
        }
    }

    void setAccentBlur(HWND hwnd, bool isEnabled)
    {
        static const SetWindowCompositionAttributeFunc setWindowCompositionAttribute = reinterpret_cast<SetWindowCompositionAttributeFunc>(reinterpret_cast<void*>(::GetProcAddress(::GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute")));
        if (setWindowCompositionAttribute == nullptr)
        {
            return;
        }
        AccentPolicy policy{isEnabled ? kAccentEnableBlurBehind : kAccentDisabled, 0, 0, 0};
        WindowCompositionAttributeData data{kWcaAccentPolicy, &policy, sizeof(policy)};
        if (!setWindowCompositionAttribute(hwnd, &data))
        {
            qWarning("设置窗口模糊失败，错误码: %lu", ::GetLastError());
        }
    }
#endif
} // namespace

namespace ShadWindowEffect
{
    bool isSupported(ShadWindowType::Effect effect)
    {
#ifdef Q_OS_WASM
        return effect == ShadWindowType::NoEffect;
#endif
        switch (effect)
        {
            case ShadWindowType::NoEffect:
            case ShadWindowType::Wallpaper:
                return true;
            case ShadWindowType::Blur:
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
                return true;
#else
                return false;
#endif
            case ShadWindowType::Mica:
#ifdef Q_OS_WIN
                return windowsBuild() >= kWindows11Build;
#else
                return false;
#endif
            case ShadWindowType::Acrylic:
            case ShadWindowType::MicaAlt:
#ifdef Q_OS_WIN
                return windowsBuild() >= kWindows11Build22H2;
#else
                return false;
#endif
        }
        return false;
    }

    qreal defaultOpacity(ShadWindowType::Effect effect)
    {
        switch (effect)
        {
            case ShadWindowType::NoEffect:
                return 1.0;
            case ShadWindowType::Wallpaper:
                return kWallpaperOpacity;
            case ShadWindowType::Blur:
#ifdef Q_OS_MACOS
                return 0.3;
#else
                return 0.6;
#endif
            case ShadWindowType::Acrylic:
            case ShadWindowType::Mica:
            case ShadWindowType::MicaAlt:
                return 0.0;
        }
        return 1.0;
    }

    void setWallpaperSource(const QImage& image)
    {
        WallpaperCache& cache = wallpaperCache();
        cache.source = image;
        cache.loaded = QImage();
        cache.blurred = QImage();
        cache.isLoaded = false;
        cache.isBlurred = false;
    }

    void setBlurRadius(int radius)
    {
        WallpaperCache& cache = wallpaperCache();
        radius = qBound(0, radius, kMaxBlurRadius);
        if (cache.blurRadius == radius)
        {
            return;
        }
        cache.blurRadius = radius;
        cache.blurred = QImage();
        cache.isBlurred = false;
    }

    bool isBlurRadiusSupported(ShadWindowType::Effect effect)
    {
#ifdef Q_OS_MACOS
        return effect == ShadWindowType::Wallpaper || effect == ShadWindowType::Blur;
#else
        return effect == ShadWindowType::Wallpaper;
#endif
    }

#ifndef Q_OS_MACOS
    void requestDesktopCaptureAccess()
    {
    }
#endif

    const QImage& wallpaper()
    {
        WallpaperCache& cache = wallpaperCache();
        if (!cache.isLoaded)
        {
            cache.isLoaded = true;
            QImage source = cache.source;
            if (source.isNull())
            {
                const QString path = systemWallpaperPath();
                if (!path.isEmpty() && !source.load(path))
                {
                    qWarning("读取系统壁纸失败: %s", qUtf8Printable(path));
                }
            }
            cache.loaded = prepareWallpaper(source);
        }
        if (!cache.isBlurred)
        {
            cache.isBlurred = true;
            cache.blurred = blurImage(cache.loaded, cache.blurRadius);
        }
        return cache.blurred;
    }

    QRectF wallpaperSourceRect(const QImage& image, const QRect& screenGeometry, const QRect& windowGeometry)
    {
        if (image.isNull() || screenGeometry.isEmpty())
        {
            return QRectF();
        }
        const qreal scale = qMax(static_cast<qreal>(screenGeometry.width()) / image.width(), static_cast<qreal>(screenGeometry.height()) / image.height());
        const QPointF offset((image.width() * scale - screenGeometry.width()) / 2.0, (image.height() * scale - screenGeometry.height()) / 2.0);
        QRectF sourceRect((QPointF(windowGeometry.topLeft() - screenGeometry.topLeft()) + offset) / scale, QSizeF(windowGeometry.size()) / scale);
        // 窗口超出屏幕时平移回图片内
        if (sourceRect.width() <= image.width())
        {
            sourceRect.moveLeft(qBound<qreal>(0.0, sourceRect.left(), image.width() - sourceRect.width()));
        }
        if (sourceRect.height() <= image.height())
        {
            sourceRect.moveTop(qBound<qreal>(0.0, sourceRect.top(), image.height() - sourceRect.height()));
        }
        return sourceRect;
    }

    void paintWallpaper(QPainter* painter, const QWidget* widget, const QRectF& rect, qreal radius)
    {
        const QImage& image = wallpaper();
        const QScreen* screen = widget->screen();
        if (image.isNull() || screen == nullptr)
        {
            return;
        }
        const QRect globalRect(widget->mapToGlobal(rect.topLeft().toPoint()), rect.size().toSize());
        const QRectF sourceRect = wallpaperSourceRect(image, screen->geometry(), globalRect);
        painter->save();
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
        if (radius > 0)
        {
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->setClipPath(ShadStyleHelper::roundedRectPath(rect, radius));
        }
        painter->drawImage(rect, image, sourceRect);
        painter->restore();
    }

#ifdef Q_OS_WIN
    void apply(QWidget* window, ShadWindowType::Effect effect, int cornerRadius, const QRect& area)
    {
        Q_UNUSED(cornerRadius);
        Q_UNUSED(area);
        const HWND hwnd = reinterpret_cast<HWND>(window->winId());
        const int build = windowsBuild();
        const bool isBackdrop = effect == ShadWindowType::Mica || effect == ShadWindowType::MicaAlt || effect == ShadWindowType::Acrylic;
        const bool isLayered = (::GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_LAYERED) != 0;
        if (!isLayered)
        {
            const MARGINS margins{isBackdrop ? 65536 : 0, 0, 0, 0};
            const HRESULT hr = ::DwmExtendFrameIntoClientArea(hwnd, &margins);
            if (FAILED(hr))
            {
                qWarning("扩展窗口边框失败，HRESULT: 0x%08lx", static_cast<unsigned long>(hr));
            }
        }
        if (build >= kWindows11Build22H2)
        {
            int backdrop = kBackdropAuto;
            if (effect == ShadWindowType::Mica)
            {
                backdrop = kBackdropMainWindow;
            }
            else if (effect == ShadWindowType::Acrylic)
            {
                backdrop = kBackdropTransientWindow;
            }
            else if (effect == ShadWindowType::MicaAlt)
            {
                backdrop = kBackdropTabbedWindow;
            }
            setDwmAttribute(hwnd, kDwmSystemBackdropType, &backdrop, sizeof(backdrop));
        }
        else if (build >= kWindows11Build)
        {
            const BOOL isMica = effect == ShadWindowType::Mica ? TRUE : FALSE;
            setDwmAttribute(hwnd, kDwmMicaEffect, &isMica, sizeof(isMica));
        }
        setAccentBlur(hwnd, effect == ShadWindowType::Blur);
        const QWindow* nativeWindow = window->windowHandle();
        if (effect == ShadWindowType::Blur && nativeWindow != nullptr && nativeWindow->surfaceType() != QSurface::RasterSurface)
        {
            RECT clientRect{};
            ::GetClientRect(hwnd, &clientRect);
            const HDC dc = ::GetDC(hwnd);
            if (dc == nullptr)
            {
                qWarning("获取窗口 DC 失败，错误码: %lu", ::GetLastError());
                return;
            }
            ::FillRect(dc, &clientRect, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
            ::ReleaseDC(hwnd, dc);
        }
    }

    QString systemWallpaperPath()
    {
        wchar_t path[MAX_PATH] = {};
        if (!::SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, path, 0))
        {
            qWarning("获取系统壁纸路径失败，错误码: %lu", ::GetLastError());
            return QString();
        }
        return QString::fromWCharArray(path);
    }
#elif !defined(Q_OS_MACOS)
    void apply(QWidget* window, ShadWindowType::Effect effect, int cornerRadius, const QRect& area)
    {
        Q_UNUSED(window);
        Q_UNUSED(effect);
        Q_UNUSED(cornerRadius);
        Q_UNUSED(area);
    }

    QString systemWallpaperPath()
    {
        return QString();
    }
#endif
} // namespace ShadWindowEffect
