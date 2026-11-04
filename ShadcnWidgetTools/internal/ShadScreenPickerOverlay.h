#ifndef SHADSCREENPICKEROVERLAY_H
#define SHADSCREENPICKEROVERLAY_H

#include <QColor>
#include <QImage>
#include <QList>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QWidget>

class QEnterEvent;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPainter;
class QPointF;
class QScreen;

// 屏幕取色时覆盖单块屏幕的窗口，组件库内部使用，由 ShadScreenColorPicker 创建
// 显示取色开始时截取的屏幕画面（冻结），上面覆盖 bg-black/70 遮罩表示正在取色；放大镜与取到的颜色使用原始截图，不受遮罩影响
// 高亮光标所在的窗口：窗口内不画遮罩，边缘描边
// 光标旁显示带红色十字的放大镜：周围 11x11 个物理像素各放大为 12x12，带像素网格；下方标签第一行为 R / G / B 三个数值标签，第二行为色块与 #RRGGBB
// 靠近屏幕边缘时放大镜翻到光标的另一侧；取色按物理像素计算，高 DPI 屏幕上同样能对准单个像素
// 左键单击或 Enter / 空格取色，方向键把目标像素移动 1 个物理像素；Esc 或右键取消
class ShadScreenPickerOverlay : public QWidget
{
    Q_OBJECT

    public:
        // parent 为发起取色的窗口：覆盖窗口作为它的子窗口，不会被它打开的模态对话框屏蔽
        ShadScreenPickerOverlay(QScreen* screen, const QPixmap& screenshot, QWidget* parent);
        ~ShadScreenPickerOverlay() override;

        // 按全局光标位置初始化目标像素，光标不在本屏幕时不显示放大镜
        void syncWithCursor();
        // 可高亮的窗口，全局逻辑坐标，按层叠顺序从前到后
        void setWindowRects(const QList<QRect>& rects);

    Q_SIGNALS:
        void colorPicked(const QColor& color);
        void canceled();

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        // 逻辑坐标转换为截图中的物理像素
        QPoint pixelAt(const QPointF& position) const;
        // 物理像素的颜色，超出截图时为无效颜色
        QColor colorAt(const QPoint& pixel) const;
        // 设置目标像素并只重绘放大镜前后的区域
        void setTargetPixel(const QPoint& pixel);
        void updateHighlight();
        // 放大镜（含下方的颜色标签）的逻辑坐标区域
        QRect loupeRect() const;
        void paintLoupe(QPainter* painter) const;

        QPixmap _screenshot;
        QImage _image;
        qreal _ratio{1.0};
        QPoint _targetPixel;
        bool _hasTarget{false};
        // 本覆盖窗口局部坐标
        QList<QRect> _windowRects;
        QRect _highlightRect;
};

#endif // SHADSCREENPICKEROVERLAY_H
