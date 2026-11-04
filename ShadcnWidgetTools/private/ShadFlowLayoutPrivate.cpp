#include "ShadFlowLayoutPrivate.h"

#include <QGuiApplication>
#include <QLayoutItem>
#include <QList>
#include <QMargins>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QWidget>
#include <QtGlobal>

#include "ShadFlowLayout.h"
#include "ShadStyleHelper.h"

ShadFlowLayoutPrivate::ShadFlowLayoutPrivate()
{
}

ShadFlowLayoutPrivate::~ShadFlowLayoutPrivate()
{
}

int ShadFlowLayoutPrivate::doLayout(const QRect& rect, bool isTestOnly) const
{
    Q_Q(const ShadFlowLayout);
    const QMargins margins = q->contentsMargins();
    const QRect effectiveRect = rect.marginsRemoved(margins);

    // 隐藏的组件不占位置
    QList<QLayoutItem*> visibleItems;
    for (QLayoutItem* item : _items)
    {
        if (!item->isEmpty())
        {
            visibleItems.append(item);
        }
    }

    int y = effectiveRect.y();
    int lineCount = 0;
    int lineStart = 0;
    while (lineStart < visibleItems.size())
    {
        // 确定这一行放哪些组件：每行至少放一个，加上下一个会超出宽度时换行
        int lineEnd = lineStart;
        int lineWidth = 0;
        int lineHeight = 0;
        while (lineEnd < visibleItems.size())
        {
            const QSize itemSize = visibleItems.at(lineEnd)->sizeHint();
            const int nextWidth = lineEnd == lineStart ? itemSize.width() : lineWidth + _horizontalSpacing + itemSize.width();
            if (lineEnd > lineStart && nextWidth > effectiveRect.width())
            {
                break;
            }
            lineWidth = nextWidth;
            lineHeight = qMax(lineHeight, itemSize.height());
            ++lineEnd;
        }

        if (!isTestOnly)
        {
            // 自定义布局不会自动镜像：按 LTR 计算后，RTL 时在 effectiveRect 内水平镜像，组件从右向左排列
            const QWidget* parentWidget = q->parentWidget();
            const Qt::LayoutDirection direction = parentWidget != nullptr ? parentWidget->layoutDirection() : QGuiApplication::layoutDirection();
            int x = effectiveRect.x();
            for (int index = lineStart; index < lineEnd; ++index)
            {
                QLayoutItem* item = visibleItems.at(index);
                const QSize itemSize = item->sizeHint();
                // 同一行内垂直居中
                const QRect logicalRect(QPoint(x, y + (lineHeight - itemSize.height()) / 2), itemSize);
                item->setGeometry(ShadStyleHelper::visualRect(direction, effectiveRect, logicalRect));
                x += itemSize.width() + _horizontalSpacing;
            }
        }

        y += lineHeight + _verticalSpacing;
        ++lineCount;
        lineStart = lineEnd;
    }

    const int contentHeight = lineCount > 0 ? y - _verticalSpacing - effectiveRect.y() : 0;
    return contentHeight + margins.top() + margins.bottom();
}
