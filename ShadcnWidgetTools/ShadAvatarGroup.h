#ifndef SHADAVATARGROUP_H
#define SHADAVATARGROUP_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QChildEvent;
class QEvent;
class QResizeEvent;
class ShadAvatar;

// 头像组，对应 shadcn AvatarGroup / AvatarGroupCount：头像横向排列并相互重叠 8px（-space-x-2），后加入的头像盖在前面的上方，
// 每个头像外圈有 2px 背景色 ring；末尾可显示计数（例如 "+3"）或图标；RTL 下从右向左排列
// 头像组统一设置成员头像与计数的尺寸，对应 group-has-data-[size]；加入后再单独修改头像尺寸会与组内布局不一致
class ShadAvatarGroupPrivate;

class SHADCN_EXPORT ShadAvatarGroup : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadAvatarGroup)
    Q_PROPERTY(ShadAvatarType::Size avatarSize READ getAvatarSize WRITE setAvatarSize)
    Q_PROPERTY(QString countText READ getCountText WRITE setCountText)

    public:
        explicit ShadAvatarGroup(QWidget* parent = nullptr);
        ~ShadAvatarGroup() override;

        // 添加头像，头像组接管头像的父子关系；返回头像的位置
        int addAvatar(ShadAvatar* avatar);
        // index 超出范围时添加到末尾
        int insertAvatar(int index, ShadAvatar* avatar);
        // 移除头像但不删除，头像的父组件置空，由调用方负责释放
        void removeAvatar(int index);
        int count() const;
        ShadAvatar* avatar(int index) const;

        // 默认 Medium
        void setAvatarSize(ShadAvatarType::Size size);
        ShadAvatarType::Size getAvatarSize() const;

        // 末尾计数的文字，例如 "+3"；与图标同时设置时显示图标
        void setCountText(const QString& text);
        QString getCountText() const;
        void setCountIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getCountIcon() const;
        // 移除计数
        void clearCount();
        bool hasCount() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        bool event(QEvent* event) override;
        void childEvent(QChildEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
};

#endif // SHADAVATARGROUP_H
