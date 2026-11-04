#ifndef SHADMAXWIDTHBOXLAYOUT_H
#define SHADMAXWIDTHBOXLAYOUT_H

#include <QBoxLayout>

class QWidget;

// 按所在组件的最大宽度计算 heightForWidth 的 QBoxLayout，组件库内部使用
// QWidgetItem 计算带布局组件的高度时直接调用布局的 heightForWidth，传入的是外层分配的宽度，不考虑组件的最大宽度；
// 组件设置固定或最大宽度后，内部自动换行的文字会按更宽的宽度算出更少的行数，高度偏小。这里先把宽度夹到最大宽度以内再计算
class ShadMaxWidthBoxLayout : public QBoxLayout
{
    public:
        ShadMaxWidthBoxLayout(QBoxLayout::Direction direction, QWidget* parent);
        ~ShadMaxWidthBoxLayout() override;

        int heightForWidth(int width) const override;
};

#endif // SHADMAXWIDTHBOXLAYOUT_H
