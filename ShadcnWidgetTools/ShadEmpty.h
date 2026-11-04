#ifndef SHADEMPTY_H
#define SHADEMPTY_H

#include <QSize>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QEvent;
class QObject;
class QPaintEvent;
class QResizeEvent;
class QVBoxLayout;

// 空状态组件，没有数据时显示的居中占位内容，由媒体、标题、描述与操作区组成
class ShadEmptyPrivate;

class SHADCN_EXPORT ShadEmpty : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadEmpty)
    Q_PROPERTY(ShadEmptyType::MediaVariant mediaVariant READ getMediaVariant WRITE setMediaVariant)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(bool borderVisible READ isBorderVisible WRITE setBorderVisible)

    public:
        explicit ShadEmpty(QWidget* parent = nullptr);
        ShadEmpty(ShadIconType::IconName icon, const QString& title, const QString& description = QString(), QWidget* parent = nullptr);
        ~ShadEmpty() override;

        // 图标媒体的样式，默认 Icon；设置了媒体组件时不起作用
        void setMediaVariant(ShadEmptyType::MediaVariant variant);
        ShadEmptyType::MediaVariant getMediaVariant() const;

        // 媒体区图标，为 None 且没有媒体组件时不显示媒体区
        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        // 自定义媒体组件，取代图标；旧组件会被删除
        void setMediaWidget(QWidget* widget);
        QWidget* getMediaWidget() const;

        // 标题，为空时不显示
        void setTitle(const QString& title);
        QString getTitle() const;

        // 描述，支持富文本链接，为空时不显示
        void setDescription(const QString& description);
        QString getDescription() const;

        // 虚线边框，默认不显示
        void setBorderVisible(bool visible);
        bool isBorderVisible() const;

        // 操作区布局，没有可见内容时不占空间
        QVBoxLayout* contentLayout() const;

        bool hasHeightForWidth() const override;
        int heightForWidth(int width) const override;
        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 点击描述中的链接时发出，不会自动打开链接
        void linkActivated(const QString& link);

    protected:
        bool event(QEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADEMPTY_H
