#ifndef SHADSCREENWINDOWLIST_H
#define SHADSCREENWINDOWLIST_H

#include <QList>
#include <QRect>

// 屏幕上可见窗口的位置，组件库内部使用
namespace ShadScreenWindowList
{
    // 可见的普通窗口，按层叠顺序从前到后，全局逻辑坐标；仅 macOS 与 Windows 支持，其他平台返回空列表
    QList<QRect> windowRects(bool includeOwnWindows);
} // namespace ShadScreenWindowList

#endif // SHADSCREENWINDOWLIST_H
