#ifndef SHADSKELETON_H
#define SHADSKELETON_H

#include <QSize>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QHideEvent;
class QPaintEvent;
class QShowEvent;

// 骨架屏：内容加载完成前的占位
class ShadSkeletonPrivate;

class SHADCN_EXPORT ShadSkeleton : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSkeleton)
    Q_PROPERTY(int radius READ getRadius WRITE setRadius)
    Q_PROPERTY(bool circular READ isCircular WRITE setCircular)
    Q_PROPERTY(bool animated READ isAnimated WRITE setAnimated)

    public:
        explicit ShadSkeleton(QWidget* parent = nullptr);
        explicit ShadSkeleton(const QSize& size, QWidget* parent = nullptr);
        ~ShadSkeleton() override;

        void setRadius(int radius);
        int getRadius() const;

        // 圆形占位，优先于 radius
        void setCircular(bool circular);
        bool isCircular() const;

        void setAnimated(bool animated);
        bool isAnimated() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADSKELETON_H
