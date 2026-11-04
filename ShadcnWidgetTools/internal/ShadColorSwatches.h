#ifndef SHADCOLORSWATCHES_H
#define SHADCOLORSWATCHES_H

#include <QColor>
#include <QList>
#include <QRect>
#include <QSize>
#include <QWidget>

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPoint;

// 颜色选择器的色块网格，组件库内部使用：基本颜色与自定义颜色共用
// 每格 32x32，中间为直径 24 的圆形色块，1px border；无效颜色表示空槽，显示为虚线圆
// 悬停时外圈 2px border 色圆环；可选中模式下选中的色块外圈为 2px primary 圆环
// 整个网格作为一个焦点，键盘聚焦时当前色块显示焦点环；方向键移动（RTL 下左右对调），Home / End 跳到首尾，Enter / 空格激活
// RTL 下从右向左排列
class ShadColorSwatches : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadColorSwatches(QWidget* parent = nullptr);
        ~ShadColorSwatches() override;

        // 无效颜色为空槽
        void setColors(const QList<QColor>& colors);
        QList<QColor> getColors() const;
        void setColorAt(int index, const QColor& color);
        QColor colorAt(int index) const;
        int count() const;

        // 每行色块数，默认 12
        void setColumns(int columns);
        int getColumns() const;

        // 可选中模式：点击或激活的色块保持选中，默认 false
        void setSelectable(bool selectable);
        bool isSelectable() const;
        // -1 表示没有选中
        void setSelectedIndex(int index);
        int getSelectedIndex() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    Q_SIGNALS:
        // 点击色块，或获得焦点时按 Enter / 空格
        void swatchActivated(int index);
        void selectedIndexChanged(int index);

    protected:
        void changeEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        // 色块所在格子的实际位置（RTL 时水平镜像）
        QRect cellRect(int index) const;
        int indexAt(const QPoint& position) const;
        void activate(int index);
        void setCursorIndex(int index);

        QList<QColor> _colors;
        int _columns{12};
        bool _isSelectable{false};
        int _selectedIndex{-1};
        int _hoveredIndex{-1};
        // 键盘导航所在的色块
        int _cursorIndex{0};
        bool _isFocusVisible{false};
};

#endif // SHADCOLORSWATCHES_H
