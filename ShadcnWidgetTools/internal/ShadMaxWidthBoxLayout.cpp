#include "ShadMaxWidthBoxLayout.h"

#include <QMargins>
#include <QWidget>
#include <QtGlobal>

ShadMaxWidthBoxLayout::ShadMaxWidthBoxLayout(QBoxLayout::Direction direction, QWidget* parent)
    : QBoxLayout(direction, parent)
{
}

ShadMaxWidthBoxLayout::~ShadMaxWidthBoxLayout()
{
}

int ShadMaxWidthBoxLayout::heightForWidth(int width) const
{
    const QWidget* widget = parentWidget();
    if (widget == nullptr)
    {
        return QBoxLayout::heightForWidth(width);
    }
    // QLayout::totalHeightForWidth 已扣除组件自身的 contentsMargins，最大宽度同样扣除后再比较
    const QMargins widgetMargins = widget->contentsMargins();
    const int maximumWidth = widget->maximumWidth() - widgetMargins.left() - widgetMargins.right();
    return QBoxLayout::heightForWidth(qMin(width, maximumWidth));
}
