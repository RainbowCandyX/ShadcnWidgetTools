#ifndef SHADATTACHMENTPRIVATE_H
#define SHADATTACHMENTPRIVATE_H

#include <QFont>
#include <QImage>
#include <QList>
#include <QPixmap>
#include <QPointer>
#include <QRect>
#include <QRectF>
#include <QString>
#include <QUrl>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QPainter;
class QVariantAnimation;
class ShadAttachment;
class ShadButton;
class ShadDropdownMenu;
class ShadTransition;

class ShadAttachmentPrivate
{
    SHAD_D_CREATE(ShadAttachment)

    public:
        ShadAttachmentPrivate();
        ~ShadAttachmentPrivate();

        // 各尺寸的内边距、间距与图标区尺寸
        struct SizeMetrics
        {
            // 有文字时 px / py，只有图标区时四周为 mediaPadding
            int paddingX;
            int paddingY;
            int mediaPadding;
            int gap;
            int mediaSize;
            int iconSize;
            int titlePixelSize;
            ShadThemeType::Radius radius;
            ShadThemeType::Radius mediaRadius;
        };

        // 卡片内各元素的逻辑位置（LTR，卡片所在的组件坐标系），绘制前用 visualRect 镜像
        struct ContentLayout
        {
            QRectF mediaRect;
            QRectF titleRect;
            QRectF descriptionRect;
            // 操作按钮可视区域的并集
            QRectF actionsRect;
        };

        // 横向最小宽度，对应 min-w-40
        static constexpr int kHorizontalMinWidth = 160;

        static SizeMetrics sizeMetrics(ShadAttachmentType::Size size);

        // 可点击：setClickable(true) 或设置了链接
        bool isInteractive() const;
        // 点击或键盘触发：发出 clicked，有链接时再发出 linkActivated 并按设置打开链接
        void activate();
        // 用系统程序打开链接，失败时写日志
        void openUrl() const;
        // 链接的显示文字：本地文件为本机路径，其余为完整网址
        QString urlDisplayText() const;
        // 文件的图标、类型文字与大小文字
        static ShadIconType::IconName iconForSuffix(const QString& suffix);
        static QString formatFileSize(qint64 bytes);
        // 按缩略图尺寸解码图片，失败时返回空图片并写日志
        static QImage loadThumbnail(const QString& path);

        bool hasMedia() const;
        bool hasContent() const;
        bool isVertical() const;
        int visibleActionCount() const;
        // 卡片区域（扣除焦点环边距）
        QRect cardRect() const;
        // 卡片可视尺寸（不含焦点环边距）
        QSize cardSize() const;
        int textBlockHeight() const;
        int contentTextWidth() const;
        int iconSize() const;
        ContentLayout computeLayout() const;
        QFont titleFont() const;
        QFont descriptionFont() const;

        // 按当前布局放置操作按钮
        void layoutActions();
        // 刷新尺寸、按钮位置、焦点策略与动画
        void updateContent();
        // 只在组件可见且需要时运行旋转与扫光动画；isShown 由 show / hide 事件显式传入
        void updateAnimations(bool isShown);
        void paintMedia(QPainter* painter, const QRectF& rect) const;
        void paintTitle(QPainter* painter, const QRectF& rect) const;

        ShadAttachmentType::State _state{ShadAttachmentType::Done};
        ShadAttachmentType::Size _size{ShadAttachmentType::Medium};
        Qt::Orientation _orientation{Qt::Horizontal};
        ShadIconType::IconName _icon{ShadIconType::None};
        QPixmap _image;
        quint64 _thumbnailSerial{0};
        QString _title;
        QString _description;
        QList<QPointer<ShadButton>> _actions;
        bool _isClickable{false};
        QUrl _url;
        bool _isOpenExternalLinks{true};
        // 链接的右键菜单，第一次使用时创建
        ShadDropdownMenu* _contextMenu{nullptr};
        bool _isPressed{false};
        bool _isFocusVisible{false};
        // 操作按钮通过键盘获得焦点时卡片同样显示焦点环（对应 focus-within）
        bool _isActionFocusVisible{false};
        ShadTransition* _hoverTransition{nullptr};
        // LoaderCircle 旋转角度，对应 animate-spin（1s 匀速一圈）
        QVariantAnimation* _spinAnimation{nullptr};
        qreal _spinAngle{0.0};
        // 扫光进度 0 到 1，对应 tw-shimmer 2s linear infinite
        QVariantAnimation* _shimmerAnimation{nullptr};
        qreal _shimmerProgress{0.0};
};

#endif // SHADATTACHMENTPRIVATE_H
