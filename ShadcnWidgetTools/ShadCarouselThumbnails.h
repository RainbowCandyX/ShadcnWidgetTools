#ifndef SHADCAROUSELTHUMBNAILS_H
#define SHADCAROUSELTHUMBNAILS_H

#include <QPixmap>
#include <QSize>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;
class QWheelEvent;
class ShadCarousel;

class ShadCarouselThumbnailsPrivate;

class SHADCN_EXPORT ShadCarouselThumbnails : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadCarouselThumbnails)
    Q_PROPERTY(int thumbnailSize READ getThumbnailSize WRITE setThumbnailSize)
    Q_PROPERTY(int spacing READ getSpacing WRITE setSpacing)
    Q_PROPERTY(bool navigationVisible READ isNavigationVisible WRITE setNavigationVisible)

    public:
        explicit ShadCarouselThumbnails(ShadCarousel* carousel = nullptr, QWidget* parent = nullptr);
        ~ShadCarouselThumbnails() override;

        void setCarousel(ShadCarousel* carousel);
        ShadCarousel* getCarousel() const;

        int addThumbnail(const QPixmap& pixmap);
        void setThumbnail(int index, const QPixmap& pixmap);
        QPixmap getThumbnail(int index) const;
        void removeThumbnail(int index);
        void clear();
        int count() const;

        void setThumbnailSize(int size);
        int getThumbnailSize() const;

        void setSpacing(int spacing);
        int getSpacing() const;

        void setNavigationVisible(bool visible);
        bool isNavigationVisible() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADCAROUSELTHUMBNAILS_H
