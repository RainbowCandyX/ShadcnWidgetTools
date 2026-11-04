#ifndef SHADAVATAR_H
#define SHADAVATAR_H

#include <QColor>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QPaintEvent;

// 头像，对应 shadcn Avatar：圆形或圆角方形图片，图片为空或加载失败时显示后备文字或图标
// 图片按 object-cover 裁剪，四周有 1px 细线（after:border）
// 组件四周预留 2px，用于绘制徽章与头像组中的 ring，实际尺寸为直径加 4
class ShadAvatarPrivate;

class SHADCN_EXPORT ShadAvatar : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadAvatar)
    Q_PROPERTY(ShadAvatarType::Size avatarSize READ getAvatarSize WRITE setAvatarSize)
    Q_PROPERTY(ShadAvatarType::Shape shape READ getShape WRITE setShape)
    Q_PROPERTY(QString fallbackText READ getFallbackText WRITE setFallbackText)
    Q_PROPERTY(bool grayscale READ isGrayscale WRITE setGrayscale)
    Q_PROPERTY(bool badgeVisible READ isBadgeVisible WRITE setBadgeVisible)
    Q_PROPERTY(QColor badgeColor READ getBadgeColor WRITE setBadgeColor)

    public:
        explicit ShadAvatar(QWidget* parent = nullptr);
        ShadAvatar(const QString& fallbackText, QWidget* parent = nullptr);
        ~ShadAvatar() override;

        // 默认 Medium（32px）；放入 ShadAvatarGroup 后由头像组统一设置
        void setAvatarSize(ShadAvatarType::Size size);
        ShadAvatarType::Size getAvatarSize() const;
        // 默认 Circle；徽章始终为圆形
        void setShape(ShadAvatarType::Shape shape);
        ShadAvatarType::Shape getShape() const;

        // 对应 AvatarImage；传入空图片时显示后备内容
        void setImage(const QPixmap& image);
        QPixmap getImage() const;
        // 从本地文件或 qrc 资源加载图片，加载失败时清空图片并显示后备内容，返回是否加载成功；不支持网络地址
        bool setImagePath(const QString& path);
        bool hasImage() const;

        // 对应 AvatarFallback：bg-muted text-muted-foreground，文字 text-sm（Small 为 text-xs），通常为姓名缩写
        void setFallbackText(const QString& text);
        QString getFallbackText() const;
        // 后备图标（Lucide），设置后代替后备文字显示；传入 None 移除
        void setFallbackIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getFallbackIcon() const;

        // 图片显示为灰度，对应 className="grayscale"
        void setGrayscale(bool grayscale);
        bool isGrayscale() const;

        // 对应 AvatarBadge：位于结束侧下方（RTL 下为左下角）的圆点，bg-primary，外圈 2px 背景色 ring；直径 Small 8 / Medium 10 / Large 12
        void setBadgeVisible(bool visible);
        bool isBadgeVisible() const;
        // 徽章中的图标，8px，primary-foreground 色；Small 尺寸不显示图标（与 shadcn 一致）
        void setBadgeIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getBadgeIcon() const;
        // 自定义徽章颜色，例如在线状态的绿色；传入无效颜色恢复为 primary
        void setBadgeColor(const QColor& color);
        QColor getBadgeColor() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        // 头像组设置成员头像的 ring
        friend class ShadAvatarGroupPrivate;
};

#endif // SHADAVATAR_H
