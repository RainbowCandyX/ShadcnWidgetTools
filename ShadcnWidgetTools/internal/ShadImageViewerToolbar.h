#ifndef SHADIMAGEVIEWERTOOLBAR_H
#define SHADIMAGEVIEWERTOOLBAR_H

#include <QColor>
#include <QString>
#include <QWidget>

#include "ShadIconType.h"

class QHBoxLayout;
class QLabel;
class QPaintEvent;
class ShadImageViewerButton;

// 图片预览底部的工具栏，组件库内部使用：计数文字、分隔线与按钮横向排列
class ShadImageViewerToolbar : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadImageViewerToolbar(QWidget* parent = nullptr);
        ~ShadImageViewerToolbar() override;

        // 工具栏与圆形按钮共用的底色与边框
        static QColor backgroundColor();
        static QColor borderColor();

        // 在末尾添加按钮，由工具栏持有
        ShadImageViewerButton* addButton(ShadIconType::IconName icon);
        QLabel* addLabel(const QString& widestText);

        // 计数文字，隐藏时分隔线一起隐藏
        void setCounterText(const QString& text);
        void setCounterVisible(bool visible);

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        QHBoxLayout* _layout{nullptr};
        QLabel* _counterLabel{nullptr};
        QWidget* _separator{nullptr};
};

#endif // SHADIMAGEVIEWERTOOLBAR_H
