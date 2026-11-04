#ifndef SHADSCREENSHOTPIN_H
#define SHADSCREENSHOTPIN_H

#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QSizeF>
#include <QString>
#include <QWidget>

class QEnterEvent;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QTimer;
class QWheelEvent;
class ShadContextMenu;

class ShadScreenshotPin : public QWidget
{
    Q_OBJECT

    public:
        ShadScreenshotPin(const QImage& image, const QPoint& globalPosition);
        ~ShadScreenshotPin() override;

    Q_SIGNALS:
        void saved(const QString& filePath);
        void saveFailed(const QString& filePath);

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void setScale(qreal scale, const QPointF& anchor);
        void copyImage();
        void saveImage();
        void rebuildMenu();

        QImage _image;
        QSizeF _baseSize;
        qreal _scale{1.0};
        QPoint _pressGlobalPosition;
        QPoint _pressWindowPosition;
        bool _isDragging{false};
        bool _isHovered{false};
        QTimer* _hintTimer{nullptr};
        ShadContextMenu* _menu{nullptr};
};

#endif
