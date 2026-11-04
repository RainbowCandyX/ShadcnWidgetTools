#ifndef SHADWATERMARK_H
#define SHADWATERMARK_H

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QSize>
#include <QString>
#include <QWidget>
#include <QtGlobal>

#include "ShadProperty.h"

class QEvent;
class QObject;
class QPaintEvent;

class ShadWatermarkPrivate;

class SHADCN_EXPORT ShadWatermark : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadWatermark)
    Q_PROPERTY(QString text READ getText WRITE setText)
    Q_PROPERTY(qreal rotation READ getRotation WRITE setRotation)
    Q_PROPERTY(QSize gap READ getGap WRITE setGap)
    Q_PROPERTY(QPoint offset READ getOffset WRITE setOffset)
    Q_PROPERTY(int fontPixelSize READ getFontPixelSize WRITE setFontPixelSize)
    Q_PROPERTY(QColor color READ getColor WRITE setColor)

    public:
        explicit ShadWatermark(QWidget* target);
        ~ShadWatermark() override;

        QWidget* getTarget() const;

        void setText(const QString& text);
        QString getText() const;

        void setImage(const QImage& image);
        QImage getImage() const;

        void setImageSize(const QSize& size);
        QSize getImageSize() const;

        void setRotation(qreal degrees);
        qreal getRotation() const;

        void setGap(const QSize& gap);
        QSize getGap() const;

        void setOffset(const QPoint& offset);
        QPoint getOffset() const;

        void setFontPixelSize(int pixelSize);
        int getFontPixelSize() const;

        void setColor(const QColor& color);
        QColor getColor() const;

        void setOpacity(qreal opacity);
        qreal getOpacity() const;

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif
