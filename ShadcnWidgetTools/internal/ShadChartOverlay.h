#ifndef SHADCHARTOVERLAY_H
#define SHADCHARTOVERLAY_H

#include <QWidget>

class QEvent;
class QMouseEvent;
class QPaintEvent;
class QWheelEvent;
class ShadChartPrivate;

// 图表的覆盖层：位于绘图层之上，绘制文字与悬停内容，鼠标事件交给图表的私有数据处理
class ShadChartOverlay : public QWidget
{
    Q_OBJECT

    public:
        ShadChartOverlay(ShadChartPrivate* chartPrivate, QWidget* parent);
        ~ShadChartOverlay() override;

    protected:
        bool event(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void leaveEvent(QEvent* event) override;

    private:
        ShadChartPrivate* _chartPrivate{nullptr};
};

#endif // SHADCHARTOVERLAY_H
