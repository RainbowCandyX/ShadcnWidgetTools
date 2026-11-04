#include "ShadTreeViewStyle.h"

#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QStyleOption>
#include <QWidget>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 拖放指示线宽度
    constexpr qreal kDropIndicatorWidth = 2.0;
} // namespace

ShadTreeViewStyle::ShadTreeViewStyle(QWidget* treeView)
    : QProxyStyle()
{
    setParent(treeView);
}

ShadTreeViewStyle::~ShadTreeViewStyle()
{
}

void ShadTreeViewStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    switch (element)
    {
        case PE_PanelItemViewRow:
        case PE_PanelItemViewItem:
        case PE_FrameFocusRect:
        case PE_IndicatorBranch:
            // 由 ShadTreeView 与其委托绘制
            return;
        case PE_IndicatorItemViewItemDrop:
            {
                const QColor primary = shadTheme->getColor(ShadThemeType::Primary);
                const QRectF rect(option->rect);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing, true);
                if (option->rect.height() == 0)
                {
                    // 插入位置：横线，两端留出圆角
                    painter->setPen(QPen(primary, kDropIndicatorWidth, Qt::SolidLine, Qt::RoundCap));
                    painter->drawLine(QPointF(rect.left() + kDropIndicatorWidth, rect.top()), QPointF(rect.right() - kDropIndicatorWidth, rect.top()));
                }
                else
                {
                    // 放到节点上：节点区域的圆角描边
                    ShadStyleHelper::strokeRoundedRect(painter, rect, shadTheme->getRadius(ShadThemeType::Medium), primary, kDropIndicatorWidth);
                }
                painter->restore();
                return;
            }
        default:
            break;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}
