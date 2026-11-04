#ifndef SHADAVATARGROUPCOUNT_H
#define SHADAVATARGROUPCOUNT_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"

class QPaintEvent;

// 头像组末尾的计数，组件库内部使用，对应 AvatarGroupCount：bg-muted text-muted-foreground rounded-full text-sm ring-2 ring-background
// 尺寸与头像组一致，图标为 size-3 / size-4 / size-5；四周与头像一样预留 2px 绘制 ring
class ShadAvatarGroupCount : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadAvatarGroupCount(QWidget* parent = nullptr);
        ~ShadAvatarGroupCount() override;

        void setAvatarSize(ShadAvatarType::Size size);
        // 文字与图标同时设置时显示图标
        void setText(const QString& text);
        QString getText() const;
        void setIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getIcon() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        ShadAvatarType::Size _size{ShadAvatarType::Medium};
        QString _text;
        ShadIconType::IconName _icon{ShadIconType::None};
};

#endif // SHADAVATARGROUPCOUNT_H
