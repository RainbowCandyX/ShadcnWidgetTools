#ifndef SHADIMAGEVIEWER_H
#define SHADIMAGEVIEWER_H

#include <QList>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "ShadProperty.h"

class QCloseEvent;
class QContextMenuEvent;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

// 图片预览：用遮罩覆盖所在的顶层窗口显示图片，支持缩放、拖动、旋转、翻转与多图切换
class ShadImageViewerPrivate;

class SHADCN_EXPORT ShadImageViewer : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadImageViewer)
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(bool toolbarVisible READ isToolbarVisible WRITE setToolbarVisible)

    public:
        // parent 决定覆盖的顶层窗口
        explicit ShadImageViewer(QWidget* parent = nullptr);
        ~ShadImageViewer() override;

        // 图片文件路径，异步解码；与 setPixmaps 互相替换
        void setImagePaths(const QStringList& paths);
        QStringList getImagePaths() const;
        // 直接显示的图片；与 setImagePaths 互相替换
        void setPixmaps(const QList<QPixmap>& pixmaps);
        QList<QPixmap> getPixmaps() const;
        int getImageCount() const;

        // 当前图片序号，没有图片时为 -1
        void setCurrentIndex(int index);
        int getCurrentIndex() const;

        // 底部工具栏，默认显示
        void setToolbarVisible(bool visible);
        bool isToolbarVisible() const;

        // 从第 index 张开始预览，已打开时只切换图片
        void open(int index = 0);
        bool isOpen() const;

        // 创建预览并打开，关闭后自动释放
        static void preview(QWidget* parent, const QStringList& paths, int index = 0);

    Q_SIGNALS:
        void currentIndexChanged(int index);
        // 关闭动画结束后发出
        void closed();

    protected:
        bool event(QEvent* event) override;
        // close() 播放关闭动画
        void closeEvent(QCloseEvent* event) override;
        bool focusNextPrevChild(bool next) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void contextMenuEvent(QContextMenuEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADIMAGEVIEWER_H
