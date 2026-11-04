#ifndef SHADATTACHMENT_H
#define SHADATTACHMENT_H

#include <QList>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QContextMenuEvent;
class QEnterEvent;
class QEvent;
class QFocusEvent;
class QHideEvent;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;
class ShadButton;

// 附件，对应 shadcn Attachment：图标区（Lucide 图标或图片）+ 标题与描述 + 结束侧的操作按钮
// 横向排列时最小宽度 160（min-w-40）；纵向排列时宽度固定为 96，有文字时为 120，操作按钮浮在右上角
// 四周预留焦点环边距，与本库其他可聚焦组件一致
class ShadAttachmentPrivate;

class SHADCN_EXPORT ShadAttachment : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadAttachment)
    Q_PROPERTY(ShadAttachmentType::State state READ getState WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(ShadAttachmentType::Size attachmentSize READ getAttachmentSize WRITE setAttachmentSize)
    Q_PROPERTY(Qt::Orientation orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(bool clickable READ isClickable WRITE setClickable)
    Q_PROPERTY(QUrl url READ getUrl WRITE setUrl)
    Q_PROPERTY(bool openExternalLinks READ getOpenExternalLinks WRITE setOpenExternalLinks)

    public:
        explicit ShadAttachment(QWidget* parent = nullptr);
        ShadAttachment(ShadIconType::IconName icon, const QString& title, const QString& description = QString(), QWidget* parent = nullptr);
        ~ShadAttachment() override;

        // 默认 Done
        void setState(ShadAttachmentType::State state);
        ShadAttachmentType::State getState() const;

        void setAttachmentSize(ShadAttachmentType::Size size);
        ShadAttachmentType::Size getAttachmentSize() const;

        // 默认 Qt::Horizontal
        void setOrientation(Qt::Orientation orientation);
        Qt::Orientation getOrientation() const;

        // 图标区显示的 Lucide 图标（variant="icon"），LoaderCircle 会自动旋转（对应 Spinner）
        // 设置了图片时显示图片；图标与图片都没有时不显示图标区
        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        // 图标区显示的图片（variant="image"），裁剪为正方形铺满；传入空图片移除
        // 状态不是 Done 或 Idle 时图片半透明（opacity-60）
        void setImage(const QPixmap& image);
        QPixmap getImage() const;

        // 标题与描述放不下时尾部省略，为空时不显示对应行
        void setTitle(const QString& title);
        QString getTitle() const;
        void setDescription(const QString& description);
        QString getDescription() const;

        // 添加一个 ghost 样式的图标按钮（对应 AttachmentAction，size="icon-xs"），按添加顺序排在结束侧
        // 返回的按钮由附件持有，通过 clicked 信号处理操作；toolTip 对应 aria-label
        ShadButton* addAction(ShadIconType::IconName icon, const QString& toolTip);
        QList<ShadButton*> getActions() const;
        // 删除全部操作按钮
        void clearActions();

        // 整张卡片可点击（对应 AttachmentTrigger）：悬停背景变为 bg-muted/50，可通过 Tab 聚焦，回车或空格触发 clicked
        // 点击操作按钮不会触发 clicked
        void setClickable(bool clickable);
        bool isClickable() const;

        // 把卡片作为链接（对应 AttachmentTrigger render={<a href>}），网址与 QUrl::fromLocalFile 的本地文件都可以
        // 设置后卡片自动可点击，点击依次发出 clicked、linkActivated，再用系统程序打开；右键菜单可打开或复制链接
        // 没有设置 toolTip 时悬停显示链接地址；传入空 QUrl 移除链接，可点击状态恢复为 setClickable 的设置
        void setUrl(const QUrl& url);
        QUrl getUrl() const;
        // 是否用 QDesktopServices 自动打开链接，默认 true；关闭后只发出 linkActivated，由调用方处理（例如程序内预览）
        void setOpenExternalLinks(bool open);
        bool getOpenExternalLinks() const;

        // 按本地文件填充卡片：标题为文件名，描述为类型与大小（例如 "PDF · 2.4 MB"），按扩展名选择图标，
        // 图片文件按缩略图尺寸解码后显示为图片；不修改状态与链接。文件不存在或不是普通文件时返回 false 且不修改卡片
        bool setFile(const QString& path);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void stateChanged(ShadAttachmentType::State state);
        // setClickable(true) 或设置了链接后，点击卡片或按下回车 / 空格时发出
        void clicked();
        // 设置了链接时，在 clicked 之后、打开链接之前发出
        void linkActivated(const QUrl& url);

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        void changeEvent(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADATTACHMENT_H
