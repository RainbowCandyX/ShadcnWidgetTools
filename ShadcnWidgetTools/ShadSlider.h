#ifndef SHADSLIDER_H
#define SHADSLIDER_H

#include <QSize>
#include <QSlider>

#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

// 滑块，支持水平与垂直方向；拖动或点击轨道时按 singleStep 吸附
// 范围模式下有两个滑块：upper 即 QAbstractSlider::value()，lower 通过 lowerValue 访问，始终满足 lower <= upper
class ShadSliderPrivate;

class SHADCN_EXPORT ShadSlider : public QSlider
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadSlider)
    Q_PROPERTY(bool rangeMode READ isRangeMode WRITE setRangeMode)
    Q_PROPERTY(int lowerValue READ getLowerValue WRITE setLowerValue NOTIFY lowerValueChanged)

    public:
        explicit ShadSlider(QWidget* parent = nullptr);
        explicit ShadSlider(Qt::Orientation orientation, QWidget* parent = nullptr);
        ~ShadSlider() override;

        void setRangeMode(bool rangeMode);
        bool isRangeMode() const;

        // 范围模式下限制在 [minimum, value()]；外部把 value() 设得比 lowerValue 小时 lowerValue 会跟随下移
        void setLowerValue(int value);
        int getLowerValue() const;
        // 同时设置上下限，upper 小于 lower 时两者交换
        void setRangeValues(int lower, int upper);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void lowerValueChanged(int value);
        // 范围模式下任一端变化时发出
        void rangeValuesChanged(int lower, int upper);

    protected:
        void sliderChange(QAbstractSlider::SliderChange change) override;
        void keyPressEvent(QKeyEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
};

#endif // SHADSLIDER_H
