#ifndef SHADAVATARPRIVATE_H
#define SHADAVATARPRIVATE_H

#include <QColor>
#include <QPixmap>
#include <QRectF>
#include <QString>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QPainter;
class ShadAvatar;

class ShadAvatarPrivate
{
    SHAD_D_CREATE(ShadAvatar)

    public:
        ShadAvatarPrivate();
        ~ShadAvatarPrivate();

        // ring-2 的宽度，也是组件四周预留的边距
        static constexpr int kRingWidth = 2;
        // 徽章图标 [&>svg]:size-2
        static constexpr int kBadgeIconSize = 8;

        // 直径：size-6 / size-8 / size-10
        static int diameter(ShadAvatarType::Size size);
        // 后备文字字号：text-xs / text-sm
        static int fallbackPixelSize(ShadAvatarType::Size size);
        // 后备图标与头像组计数图标的边长：size-3 / size-4 / size-5
        static int iconSize(ShadAvatarType::Size size);
        // 徽章直径：size-2 / size-2.5 / size-3
        static int badgeDiameter(ShadAvatarType::Size size);
        // 在 rect 外侧绘制 2px 背景色圆环，对应 ring-2 ring-background
        static void drawRing(QPainter* painter, const QRectF& rect, qreal radius);

        // 圆角半径：Circle 为半边长，Rounded 为 rounded-lg
        qreal cornerRadius(qreal side) const;
        // 按尺寸、外形与缩放比例生成裁剪后的图片，结果会缓存
        QPixmap shapedImage(qreal devicePixelRatio);
        void invalidateCache();

        ShadAvatarType::Size _size{ShadAvatarType::Medium};
        ShadAvatarType::Shape _shape{ShadAvatarType::Circle};
        QPixmap _image;
        QPixmap _cache;
        qreal _cacheDevicePixelRatio{0.0};
        QString _fallbackText;
        ShadIconType::IconName _fallbackIcon{ShadIconType::None};
        bool _isGrayscale{false};
        bool _isBadgeVisible{false};
        ShadIconType::IconName _badgeIcon{ShadIconType::None};
        // 无效时使用 primary
        QColor _badgeColor;
        // 放入头像组时显示 ring，由 ShadAvatarGroup 设置
        bool _isGroupRingVisible{false};
};

#endif // SHADAVATARPRIVATE_H
