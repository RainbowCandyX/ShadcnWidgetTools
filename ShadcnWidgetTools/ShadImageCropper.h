#ifndef SHADIMAGECROPPER_H
#define SHADIMAGECROPPER_H

#include <QImage>
#include <QRect>
#include <QSize>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

class ShadImageCropperPrivate;

class SHADCN_EXPORT ShadImageCropper : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadImageCropper)
    Q_PROPERTY(qreal aspectRatio READ getAspectRatio WRITE setAspectRatio)
    Q_PROPERTY(ShadImageCropperType::Shape shape READ getShape WRITE setShape)

    public:
        explicit ShadImageCropper(QWidget* parent = nullptr);
        ~ShadImageCropper() override;

        void setImage(const QImage& image);
        QImage getImage() const;

        void setAspectRatio(qreal ratio);
        qreal getAspectRatio() const;

        void setShape(ShadImageCropperType::Shape shape);
        ShadImageCropperType::Shape getShape() const;

        void rotateLeft();
        void rotateRight();
        void flipHorizontal();
        void flipVertical();
        void reset();

        QRect getCropRect() const;
        void setCropRect(const QRect& rect);
        QImage croppedImage() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void cropRectChanged(const QRect& rect);
        void imageChanged();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void changeEvent(QEvent* event) override;
};

#endif // SHADIMAGECROPPER_H
