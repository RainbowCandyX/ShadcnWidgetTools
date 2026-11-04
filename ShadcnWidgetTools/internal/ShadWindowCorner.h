#ifndef SHADWINDOWCORNER_H
#define SHADWINDOWCORNER_H

#include <QWidget>
#include <QtCore/qnamespace.h>

class QPaintEvent;

// 无边框窗口的圆角：放在窗口的一个角上，位于所有子组件之上
// 把窗口圆角矩形以外的像素清成透明（边缘按覆盖率部分清除，带抗锯齿），再补画圆弧上的 1px 边框
// 贴着窗口角落的子组件（例如侧栏）画满到直角时，圆角仍然保持；窗口必须设置 WA_TranslucentBackground，不接收鼠标事件
class ShadWindowCorner : public QWidget
{
    Q_OBJECT

    public:
        ShadWindowCorner(Qt::Corner corner, QWidget* window);
        ~ShadWindowCorner() override;

        // 按窗口尺寸与圆角半径放到对应的角上，并重绘
        void place(int radius);

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        Qt::Corner _corner{Qt::TopLeftCorner};
        int _radius{0};
};

#endif // SHADWINDOWCORNER_H
