#include "ShadDockOverlay.h"

#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QSizeF>
#include <QtCore/qnamespace.h>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr qreal kGuideSize = 40.0;
    constexpr qreal kGuideGap = 6.0;
    constexpr qreal kEdgeMargin = 16.0;
    constexpr qreal kIconSize = 22.0;
    constexpr qreal kIconRadius = 3.0;
    constexpr qreal kIconStroke = 1.5;
    constexpr qreal kEdgeRatio = 0.3;
    constexpr qreal kSideRatio = 0.5;
    constexpr qreal kPartAlpha = 0.35;
    constexpr qreal kHoverPartAlpha = 0.9;
    constexpr qreal kPreviewInset = 2.0;
    constexpr qreal kPreviewAlpha = 0.15;
    constexpr qreal kPreviewBorder = 2.0;

    constexpr ShadDockType::DockArea kSideAreas[] = {ShadDockType::LeftArea, ShadDockType::RightArea, ShadDockType::TopArea, ShadDockType::BottomArea};
    constexpr ShadDockType::DockArea kAllAreas[] = {ShadDockType::CenterArea, ShadDockType::LeftArea, ShadDockType::RightArea, ShadDockType::TopArea, ShadDockType::BottomArea};
} // namespace

ShadDockOverlay::ShadDockOverlay(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
}

ShadDockOverlay::~ShadDockOverlay()
{
}

void ShadDockOverlay::setGroupRect(const QRect& rect, bool isCenterAllowed)
{
    if (_groupRect == rect && _isCenterAllowed == isCenterAllowed)
    {
        return;
    }
    _groupRect = rect;
    _isCenterAllowed = isCenterAllowed;
    update();
}

ShadDockOverlay::Hit ShadDockOverlay::hitTest(const QPoint& pos) const
{
    Hit hit;
    for (const ShadDockType::DockArea area : kAllAreas)
    {
        if (isGuideVisible(false, area) && guideRect(false, area).contains(pos))
        {
            hit.isValid = true;
            hit.area = area;
            return hit;
        }
    }
    for (const ShadDockType::DockArea area : kSideAreas)
    {
        if (isGuideVisible(true, area) && guideRect(true, area).contains(pos))
        {
            hit.isValid = true;
            hit.isEdge = true;
            hit.area = area;
            return hit;
        }
    }
    return hit;
}

void ShadDockOverlay::setHover(const Hit& hit)
{
    if (_hover.isValid == hit.isValid && _hover.isEdge == hit.isEdge && _hover.area == hit.area)
    {
        return;
    }
    _hover = hit;
    update();
}

void ShadDockOverlay::setPreview(const QRect& rect)
{
    if (_preview == rect)
    {
        return;
    }
    _preview = rect;
    update();
}

ShadDockType::DockArea ShadDockOverlay::physicalArea(ShadDockType::DockArea area, Qt::LayoutDirection direction)
{
    if (direction != Qt::RightToLeft)
    {
        return area;
    }
    if (area == ShadDockType::LeftArea)
    {
        return ShadDockType::RightArea;
    }
    return area == ShadDockType::RightArea ? ShadDockType::LeftArea : area;
}

void ShadDockOverlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    if (!_preview.isEmpty())
    {
        const QColor primary = shadTheme->getColor(ShadThemeType::Primary);
        const QRectF preview = QRectF(_preview).adjusted(kPreviewInset, kPreviewInset, -kPreviewInset, -kPreviewInset);
        fillRoundedRect(&painter, preview, radius, withAlpha(primary, kPreviewAlpha));
        strokeRoundedRect(&painter, preview, radius, primary, kPreviewBorder);
    }
    for (const ShadDockType::DockArea area : kSideAreas)
    {
        if (isGuideVisible(true, area))
        {
            paintGuide(&painter, true, area);
        }
    }
    for (const ShadDockType::DockArea area : kAllAreas)
    {
        if (isGuideVisible(false, area))
        {
            paintGuide(&painter, false, area);
        }
    }
}

QRectF ShadDockOverlay::guideRect(bool isEdge, ShadDockType::DockArea logicalArea) const
{
    const ShadDockType::DockArea area = physicalArea(logicalArea, layoutDirection());
    if (isEdge)
    {
        const qreal centerX = (width() - kGuideSize) / 2.0;
        const qreal centerY = (height() - kGuideSize) / 2.0;
        switch (area)
        {
            case ShadDockType::LeftArea:
                return QRectF(kEdgeMargin, centerY, kGuideSize, kGuideSize);
            case ShadDockType::RightArea:
                return QRectF(width() - kEdgeMargin - kGuideSize, centerY, kGuideSize, kGuideSize);
            case ShadDockType::TopArea:
                return QRectF(centerX, kEdgeMargin, kGuideSize, kGuideSize);
            case ShadDockType::BottomArea:
                return QRectF(centerX, height() - kEdgeMargin - kGuideSize, kGuideSize, kGuideSize);
            default:
                return QRectF();
        }
    }
    const QRectF center(QRectF(_groupRect).center() - QPointF(kGuideSize / 2.0, kGuideSize / 2.0), QSizeF(kGuideSize, kGuideSize));
    const qreal step = kGuideSize + kGuideGap;
    switch (area)
    {
        case ShadDockType::LeftArea:
            return center.translated(-step, 0.0);
        case ShadDockType::RightArea:
            return center.translated(step, 0.0);
        case ShadDockType::TopArea:
            return center.translated(0.0, -step);
        case ShadDockType::BottomArea:
            return center.translated(0.0, step);
        default:
            return center;
    }
}

bool ShadDockOverlay::isGuideVisible(bool isEdge, ShadDockType::DockArea area) const
{
    if (!isEdge)
    {
        return !_groupRect.isEmpty() && (area != ShadDockType::CenterArea || _isCenterAllowed);
    }
    if (area == ShadDockType::CenterArea)
    {
        return false;
    }
    // 与面板组指引重叠时让位
    const QRectF edgeRect = guideRect(true, area);
    for (const ShadDockType::DockArea groupArea : kAllAreas)
    {
        if (isGuideVisible(false, groupArea) && guideRect(false, groupArea).intersects(edgeRect))
        {
            return false;
        }
    }
    return true;
}

void ShadDockOverlay::paintGuide(QPainter* painter, bool isEdge, ShadDockType::DockArea area) const
{
    using namespace ShadStyleHelper;
    const QRectF box = guideRect(isEdge, area);
    const bool isHover = _hover.isValid && _hover.isEdge == isEdge && _hover.area == area;
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QColor color = shadTheme->getColor(isHover ? ShadThemeType::Primary : ShadThemeType::MutedForeground);
    drawShadowSm(painter, box, radius);
    fillRoundedRect(painter, box, radius, shadTheme->getColor(ShadThemeType::Popover));
    strokeRoundedRect(painter, box, radius, shadTheme->getColor(isHover ? ShadThemeType::Primary : ShadThemeType::Border));
    const QRectF icon(box.center() - QPointF(kIconSize / 2.0, kIconSize / 2.0), QSizeF(kIconSize, kIconSize));
    const qreal ratio = isEdge ? kEdgeRatio : kSideRatio;
    QRectF part = icon;
    switch (physicalArea(area, layoutDirection()))
    {
        case ShadDockType::LeftArea:
            part.setWidth(icon.width() * ratio);
            break;
        case ShadDockType::RightArea:
            part.setLeft(icon.right() - icon.width() * ratio);
            break;
        case ShadDockType::TopArea:
            part.setHeight(icon.height() * ratio);
            break;
        case ShadDockType::BottomArea:
            part.setTop(icon.bottom() - icon.height() * ratio);
            break;
        default:
            break;
    }
    fillRoundedRect(painter, part, kIconRadius, withAlpha(color, isHover ? kHoverPartAlpha : kPartAlpha));
    strokeRoundedRect(painter, icon, kIconRadius, color, kIconStroke);
}
