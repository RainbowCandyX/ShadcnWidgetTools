#ifndef SHADITEM_H
#define SHADITEM_H

#include <QPixmap>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

// 列表项，对应 shadcn Item：一行由媒体区、内容（标题与描述）、操作区组成，上下可加页眉与页脚
// rounded-md border text-sm，三种样式（ShadItemType::Variant）与三种尺寸（ShadItemType::Size）
// 主行垂直居中；有描述时媒体区改为顶部对齐并下移 2px。标题单行、描述最多两行，超出时尾部省略
// 页眉与页脚各占一整行，与主行的间距和主行内的间距相同
// 可点击（setClickable，对应作为链接使用的 Item）：悬停 bg-muted，可通过 Tab 聚焦并显示焦点环，单击或按 Enter / 空格发出 clicked
// 四周始终预留 3px 焦点环边距，可点击与不可点击的列表项边缘对齐；宽度随父布局拉伸（w-full），高度随宽度变化
class ShadItemPrivate;

class SHADCN_EXPORT ShadItem : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadItem)
    Q_PROPERTY(ShadItemType::Variant variant READ getVariant WRITE setVariant)
    Q_PROPERTY(ShadItemType::Size size READ getSize WRITE setSize)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(bool clickable READ isClickable WRITE setClickable)

    public:
        explicit ShadItem(QWidget* parent = nullptr);
        ShadItem(const QString& title, const QString& description = QString(), QWidget* parent = nullptr);
        ~ShadItem() override;

        // 默认 Default
        void setVariant(ShadItemType::Variant variant);
        ShadItemType::Variant getVariant() const;
        // 默认 Medium
        void setSize(ShadItemType::Size size);
        ShadItemType::Size getSize() const;

        // 标题与描述，为空时不显示
        void setTitle(const QString& title);
        QString getTitle() const;
        void setDescription(const QString& description);
        QString getDescription() const;

        // 媒体区，图标、图片与组件三者互相替换
        // 图标对应 variant="icon"，传入 None 移除
        void setMediaIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getMediaIcon() const;
        // 图片对应 variant="image"，按尺寸为 40 / 32 / 24 的圆角正方形；传入空图移除
        void setMediaPixmap(const QPixmap& pixmap);
        QPixmap getMediaPixmap() const;
        // 组件对应 variant="default"，例如头像；列表项接管父子关系，替换时旧组件被删除，传入 nullptr 移除
        void setMediaWidget(QWidget* widget);
        QWidget* getMediaWidget() const;

        // 操作区（ItemActions）：按添加顺序横排，间距 gap-2；列表项接管父子关系
        // 函数名避开 QWidget::addAction / removeAction（QAction 的接口）
        void addActionWidget(QWidget* widget);
        // 从操作区移除但不删除，父组件置空，由调用方负责释放
        void removeActionWidget(QWidget* widget);

        // 页眉 / 页脚（ItemHeader / ItemFooter）：占一整行；列表项接管父子关系，替换时旧组件被删除，传入 nullptr 移除
        void setHeaderWidget(QWidget* widget);
        QWidget* getHeaderWidget() const;
        void setFooterWidget(QWidget* widget);
        QWidget* getFooterWidget() const;
        // 便捷写法：页眉为 text-sm font-medium 文字，页脚为 text-sm text-muted-foreground 文字，自动换行；传入空字符串移除
        void setHeaderText(const QString& text);
        void setFooterText(const QString& text);

        // 默认 false
        void setClickable(bool clickable);
        bool isClickable() const;

    Q_SIGNALS:
        // 可点击时单击列表项（不包括操作区等子组件上的点击），或获得焦点时按 Enter / 空格
        void clicked();

    protected:
        void changeEvent(QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADITEM_H
