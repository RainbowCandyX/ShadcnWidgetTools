#ifndef SHADCAROUSELDOTS_H
#define SHADCAROUSELDOTS_H

#include <QSize>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class ShadCarousel;

class ShadCarouselDotsPrivate;

class SHADCN_EXPORT ShadCarouselDots : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadCarouselDots)
    Q_PROPERTY(int dotSize READ getDotSize WRITE setDotSize)
    Q_PROPERTY(int spacing READ getSpacing WRITE setSpacing)

    public:
        explicit ShadCarouselDots(ShadCarousel* carousel = nullptr, QWidget* parent = nullptr);
        ~ShadCarouselDots() override;

        void setCarousel(ShadCarousel* carousel);
        ShadCarousel* getCarousel() const;

        void setDotSize(int size);
        int getDotSize() const;

        void setSpacing(int spacing);
        int getSpacing() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADCAROUSELDOTS_H
