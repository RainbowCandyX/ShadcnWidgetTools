#ifndef SHADWINDOWEFFECT_H
#define SHADWINDOWEFFECT_H

#include <QImage>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QtGlobal>

#include "ShadDef.h"

class QPainter;
class QWidget;

// 窗口特效的平台实现
namespace ShadWindowEffect
{
    bool isSupported(ShadWindowType::Effect effect);
    // area 为空时为整个窗口，只有 macOS 使用
    void apply(QWidget* window, ShadWindowType::Effect effect, int cornerRadius, const QRect& area = QRect());
    qreal defaultOpacity(ShadWindowType::Effect effect);

    // 以 1920 像素宽的壁纸为准
    constexpr int kDefaultBlurRadius = 120;
    constexpr int kMaxBlurRadius = 200;

    // 空图片表示使用系统壁纸
    void setWallpaperSource(const QImage& image);
    void setBlurRadius(int radius);
    bool isBlurRadiusSupported(ShadWindowType::Effect effect);
    void requestDesktopCaptureAccess();
    // 没有可用图片时为空
    const QImage& wallpaper();
    QRectF wallpaperSourceRect(const QImage& image, const QRect& screenGeometry, const QRect& windowGeometry);
    void paintWallpaper(QPainter* painter, const QWidget* widget, const QRectF& rect, qreal radius);
    QString systemWallpaperPath();
} // namespace ShadWindowEffect

#endif // SHADWINDOWEFFECT_H
