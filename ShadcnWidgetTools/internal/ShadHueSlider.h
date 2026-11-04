#ifndef SHADHUESLIDER_H
#define SHADHUESLIDER_H

#include <QSize>
#include <QWidget>
#include <QtGlobal>

class QEnterEvent;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class ShadTransition;

// 颜色选择器的色相滑条，组件库内部使用
// 轨道高 12、rounded-full，从左到右为 0 到 360 度的彩虹渐变；方向不随布局方向镜像
// 滑块外观与 ShadSlider 相同：16px 白色圆形，border-primary shadow-sm，悬停或键盘聚焦时外圈 ring-4 ring-ring/50
// 左右各预留 4px 绘制 ring；可视宽度为 sizeHint 宽度减 8，与 ShadColorArea 的可视宽度相同
// 键盘：左右方向键每次 1 度，按住 Shift 为 10 度；Home / End 跳到两端
class ShadHueSlider : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadHueSlider(QWidget* parent = nullptr);
        ~ShadHueSlider() override;

        // 色相 0 到 1，不发出信号
        void setHue(qreal hue);
        qreal getHue() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 通过鼠标或键盘改变时发出
        void hueChanged(qreal hue);

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void updateFromPosition(qreal x);
        void applyUserHue(qreal hue);
        // 悬停或键盘聚焦时显示 ring
        void updateRing();

        qreal _hue{0.0};
        bool _isHovered{false};
        bool _isFocusVisible{false};
        ShadTransition* _ringTransition{nullptr};
};

#endif // SHADHUESLIDER_H
