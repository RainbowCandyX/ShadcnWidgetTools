#ifndef SHADITEMMEDIA_H
#define SHADITEMMEDIA_H

#include <QPixmap>
#include <QPointer>
#include <QSize>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"

class QEvent;
class QObject;
class QPaintEvent;
class QResizeEvent;

// 列表项的媒体区，组件库内部使用，对应 ItemMedia，三种内容互相替换：
// 图标（variant="icon"）：16px Lucide 图标，前景色，没有底色
// 图片（variant="image"）：Medium / Small / ExtraSmall 为 40 / 32 / 24 的正方形，rounded-sm，按 object-cover 缩放裁剪
// 组件（variant="default"）：任意组件（例如头像），按其 sizeHint 居中放置，媒体区接管父子关系
// 顶部偏移通过 contentsMargins 实现，对应有描述时的 translate-y-0.5
class ShadItemMedia : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadItemMedia(QWidget* parent = nullptr);
        ~ShadItemMedia() override;

        void setIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getIcon() const;
        void setPixmap(const QPixmap& pixmap);
        QPixmap getPixmap() const;
        // 替换时旧组件被删除，传入 nullptr 移除
        void setWidget(QWidget* widget);
        QWidget* getWidget() const;

        void setItemSize(ShadItemType::Size size);
        void setTopOffset(int offset);
        // 设置了图标、图片或组件
        bool hasContent() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;
        bool eventFilter(QObject* watched, QEvent* event) override;

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        // 移除当前组件（不影响图标与图片）
        void releaseWidget();
        // 内容区尺寸，不含 contentsMargins
        QSize contentSize() const;
        void placeWidget();

        ShadIconType::IconName _icon{ShadIconType::None};
        QPixmap _pixmap;
        QPointer<QWidget> _widget;
        ShadItemType::Size _size{ShadItemType::Medium};
};

#endif // SHADITEMMEDIA_H
