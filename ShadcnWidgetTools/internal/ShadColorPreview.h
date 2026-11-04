#ifndef SHADCOLORPREVIEW_H
#define SHADCOLORPREVIEW_H

#include <QColor>
#include <QSize>
#include <QWidget>

class QEvent;
class QMouseEvent;
class QPaintEvent;

// 颜色选择器的预览条，组件库内部使用：上半为当前颜色，下半为原始颜色，rounded-md，1px 边框
// 宽度随布局拉伸，可视高度 48，四周预留 3px 与输入框对齐；鼠标位于下半时为手形指针，点击下半发出 originalClicked
class ShadColorPreview : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadColorPreview(QWidget* parent = nullptr);
        ~ShadColorPreview() override;

        void setCurrentColor(const QColor& color);
        void setOriginalColor(const QColor& color);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        void originalClicked();

    protected:
        void leaveEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        // 位置是否位于下半的原始颜色区域
        bool isOriginalArea(int y) const;

        QColor _currentColor;
        QColor _originalColor;
};

#endif // SHADCOLORPREVIEW_H
