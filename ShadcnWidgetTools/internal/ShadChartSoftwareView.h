#ifndef SHADCHARTSOFTWAREVIEW_H
#define SHADCHARTSOFTWAREVIEW_H

#include <QWidget>

class QPaintEvent;
class ShadChartPrivate;

// 图表的 CPU 绘制层：GPU 不可用时代替 QRhi 层
class ShadChartSoftwareView : public QWidget
{
    Q_OBJECT

    public:
        ShadChartSoftwareView(ShadChartPrivate* chartPrivate, QWidget* parent);
        ~ShadChartSoftwareView() override;

    Q_SIGNALS:
        // 每次绘制完成后发出
        void framePainted();

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        ShadChartPrivate* _chartPrivate{nullptr};
};

#endif // SHADCHARTSOFTWAREVIEW_H
