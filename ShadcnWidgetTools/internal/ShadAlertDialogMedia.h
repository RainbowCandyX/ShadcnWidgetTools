#ifndef SHADALERTDIALOGMEDIA_H
#define SHADALERTDIALOGMEDIA_H

#include <QPixmap>
#include <QSize>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"

class QPaintEvent;

// 警告对话框的媒体区，组件库内部使用，对应 AlertDialogMedia：size-16 rounded-md bg-muted，图标 size-8，下方 mb-2
// 图片按 object-cover 缩放裁剪并裁成圆角；同时设置图标与图片时显示图片
// 下方的 mb-2 通过 contentsMargins 实现，绘制区域为 contentsRect()
class ShadAlertDialogMedia : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadAlertDialogMedia(QWidget* parent = nullptr);
        ~ShadAlertDialogMedia() override;

        void setIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getIcon() const;
        void setPixmap(const QPixmap& pixmap);
        QPixmap getPixmap() const;
        void setVariant(ShadAlertDialogType::MediaVariant variant);
        ShadAlertDialogType::MediaVariant getVariant() const;

        // 设置了图标或图片
        bool hasContent() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        ShadIconType::IconName _icon{ShadIconType::None};
        QPixmap _pixmap;
        ShadAlertDialogType::MediaVariant _variant{ShadAlertDialogType::Default};
};

#endif // SHADALERTDIALOGMEDIA_H
