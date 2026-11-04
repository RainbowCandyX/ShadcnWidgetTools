#include "ShadPlacement.h"

#include <QGuiApplication>
#include <QScreen>
#include <QtGlobal>

namespace ShadPlacement
{
    namespace
    {
        ShadPlacementType::Side oppositeSide(ShadPlacementType::Side side)
        {
            switch (side)
            {
                case ShadPlacementType::Top:
                    return ShadPlacementType::Bottom;
                case ShadPlacementType::Bottom:
                    return ShadPlacementType::Top;
                case ShadPlacementType::Left:
                    return ShadPlacementType::Right;
                case ShadPlacementType::Right:
                    break;
            }
            return ShadPlacementType::Left;
        }

        // 主方向上该侧可用的空间
        int spaceOnSide(const QRect& anchor, const QRect& available, ShadPlacementType::Side side, int offset)
        {
            switch (side)
            {
                case ShadPlacementType::Top:
                    return anchor.top() - offset - (available.top() + kScreenMargin);
                case ShadPlacementType::Bottom:
                    return available.bottom() + 1 - kScreenMargin - (anchor.bottom() + 1 + offset);
                case ShadPlacementType::Left:
                    return anchor.left() - offset - (available.left() + kScreenMargin);
                case ShadPlacementType::Right:
                    break;
            }
            return available.right() + 1 - kScreenMargin - (anchor.right() + 1 + offset);
        }
    } // namespace

    Result place(const QRect& anchor, const QSize& size, ShadPlacementType::Side side, ShadPlacementType::Align align, int offset, Qt::LayoutDirection direction)
    {
        const bool isRightToLeft = direction == Qt::RightToLeft;
        // Left / Right 在 RTL 下互换
        ShadPlacementType::Side physicalSide = side;
        if (isRightToLeft && (side == ShadPlacementType::Left || side == ShadPlacementType::Right))
        {
            physicalSide = oppositeSide(side);
        }
        const QScreen* screen = QGuiApplication::screenAt(anchor.center());
        if (screen == nullptr)
        {
            screen = QGuiApplication::primaryScreen();
        }
        const QRect available = screen != nullptr ? screen->availableGeometry() : QRect();
        const bool isVerticalSide = physicalSide == ShadPlacementType::Top || physicalSide == ShadPlacementType::Bottom;
        if (available.isValid())
        {
            const int needed = isVerticalSide ? size.height() : size.width();
            const ShadPlacementType::Side opposite = oppositeSide(physicalSide);
            if (spaceOnSide(anchor, available, physicalSide, offset) < needed && spaceOnSide(anchor, available, opposite, offset) > spaceOnSide(anchor, available, physicalSide, offset))
            {
                physicalSide = opposite;
            }
        }

        QRect rect(QPoint(0, 0), size);
        switch (physicalSide)
        {
            case ShadPlacementType::Top:
                {
                    rect.moveBottom(anchor.top() - offset - 1);
                    break;
                }
            case ShadPlacementType::Bottom:
                {
                    rect.moveTop(anchor.bottom() + 1 + offset);
                    break;
                }
            case ShadPlacementType::Left:
                {
                    rect.moveRight(anchor.left() - offset - 1);
                    break;
                }
            case ShadPlacementType::Right:
                {
                    rect.moveLeft(anchor.right() + 1 + offset);
                    break;
                }
        }
        if (isVerticalSide)
        {
            // 起始侧对齐：LTR 左对齐，RTL 右对齐
            const bool alignLeft = (align == ShadPlacementType::Start) != isRightToLeft;
            if (align == ShadPlacementType::Center)
            {
                rect.moveLeft(anchor.center().x() - size.width() / 2);
            }
            else
            {
                rect.moveLeft(alignLeft ? anchor.left() : anchor.right() + 1 - size.width());
            }
        }
        else
        {
            if (align == ShadPlacementType::Center)
            {
                rect.moveTop(anchor.center().y() - size.height() / 2);
            }
            else
            {
                rect.moveTop(align == ShadPlacementType::Start ? anchor.top() : anchor.bottom() + 1 - size.height());
            }
        }
        if (available.isValid())
        {
            // 交叉方向限制在屏幕内；主方向翻转后仍放不下时也限制，避免超出屏幕
            rect.moveLeft(qBound(available.left() + kScreenMargin, rect.left(), qMax(available.left() + kScreenMargin, available.right() + 1 - kScreenMargin - size.width())));
            rect.moveTop(qBound(available.top() + kScreenMargin, rect.top(), qMax(available.top() + kScreenMargin, available.bottom() + 1 - kScreenMargin - size.height())));
        }
        return Result{rect, physicalSide};
    }
} // namespace ShadPlacement
