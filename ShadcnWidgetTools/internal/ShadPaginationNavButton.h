#ifndef SHADPAGINATIONNAVBUTTON_H
#define SHADPAGINATIONNAVBUTTON_H

#include <QSize>
#include <QString>

#include "ShadButton.h"

class QPaintEvent;
class QWidget;

// 分页的上一页 / 下一页按钮，组件库内部使用，对应 PaginationPrevious / PaginationNext
// ghost 变体 default 尺寸（h-9），背景、悬停、焦点环由 ShadButton 绘制，箭头与文字由本类绘制：
// 上一页为 ChevronLeft + 文字（pl-2 pr-3），下一页为文字 + ChevronRight（pl-3 pr-2）；不显示文字时为 36x36 图标按钮
// RTL 下箭头方向翻转（cn-rtl-flip），箭头始终位于各自的外侧
class ShadPaginationNavButton : public ShadButton
{
    Q_OBJECT

    public:
        ShadPaginationNavButton(bool isNext, QWidget* parent = nullptr);
        ~ShadPaginationNavButton() override;

        void setLabel(const QString& label);
        QString getLabel() const;
        void setLabelVisible(bool visible);
        bool isLabelVisible() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        bool _isNext{false};
        bool _isLabelVisible{true};
        QString _label;
};

#endif // SHADPAGINATIONNAVBUTTON_H
