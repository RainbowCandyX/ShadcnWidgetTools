#ifndef SHADIMAGEVIEWERBUTTON_H
#define SHADIMAGEVIEWERBUTTON_H

#include <QAbstractButton>
#include <QSize>

#include "ShadIconType.h"

class QEnterEvent;
class QEvent;
class QPaintEvent;
class ShadTransition;

// 图片预览的按钮，组件库内部使用：Round 用于关闭与切换，Square 用于工具栏
// 不接收焦点，键盘操作由 ShadImageViewer 统一处理
class ShadImageViewerButton : public QAbstractButton
{
    Q_OBJECT

    public:
        enum Shape
        {
            Round = 0,
            Square,
        };

        ShadImageViewerButton(ShadIconType::IconName icon, Shape shape, QWidget* parent = nullptr);
        ~ShadImageViewerButton() override;

        void setShadIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getShadIcon() const;

        QSize sizeHint() const override;

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        ShadIconType::IconName _icon{ShadIconType::None};
        Shape _shape{Round};
        ShadTransition* _hoverTransition{nullptr};
};

#endif // SHADIMAGEVIEWERBUTTON_H
