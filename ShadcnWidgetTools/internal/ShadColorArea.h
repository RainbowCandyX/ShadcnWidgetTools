#ifndef SHADCOLORAREA_H
#define SHADCOLORAREA_H

#include <QSize>
#include <QWidget>
#include <QtGlobal>

class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPointF;

// 颜色选择器的饱和度与明度方块，组件库内部使用
// 背景为当前色相，横向从左到右饱和度 0 到 1，纵向从上到下明度 1 到 0；方向不随布局方向镜像
// 可视区域 240x240，rounded-md，1px 边框；四周预留 3px 焦点环边距
// 取色点为白色圆环；鼠标按下或拖动时发出 saturationValueChanged
// 键盘：左右方向键调整饱和度、上下方向键调整明度，每次 1%，按住 Shift 为 10%
class ShadColorArea : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadColorArea(QWidget* parent = nullptr);
        ~ShadColorArea() override;

        // 色相 0 到 1，只改变背景，不发出信号
        void setHue(qreal hue);
        // 饱和度与明度 0 到 1，不发出信号
        void setSaturationValue(qreal saturation, qreal value);
        qreal getSaturation() const;
        qreal getValue() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 通过鼠标或键盘改变时发出
        void saturationValueChanged(qreal saturation, qreal value);

    protected:
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        // 按鼠标位置设置饱和度与明度并发出信号
        void updateFromPosition(const QPointF& position);
        void applyUserValue(qreal saturation, qreal value);

        qreal _hue{0.0};
        qreal _saturation{0.0};
        qreal _value{1.0};
        bool _isFocusVisible{false};
};

#endif // SHADCOLORAREA_H
