#include "ShadResizableHandle.h"

#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QSplitter>

#include "ShadIcon.h"
#include "ShadResizable.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // withHandle 抓手：w-3 h-4 rounded-xs border bg-border，图标 size-2.5
    constexpr int kGripShortSide = 12;
    constexpr int kGripLongSide = 16;
    constexpr qreal kGripRadius = 2.0;
    constexpr int kGripIconSize = 10;
} // namespace

ShadResizableHandle::ShadResizableHandle(Qt::Orientation orientation, QSplitter* parent)
    : QSplitterHandle(orientation, parent)
{
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadResizableHandle::~ShadResizableHandle()
{
}

void ShadResizableHandle::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    const QColor borderColor = shadTheme->getColor(ShadThemeType::Border);
    // 面板左右排列时手柄为竖线，上下排列时为横线
    const bool isVerticalLine = orientation() == Qt::Horizontal;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
    if (isVerticalLine)
    {
        painter.fillRect(QRect((width() - 1) / 2, 0, 1, height()), borderColor);
    }
    else
    {
        painter.fillRect(QRect(0, (height() - 1) / 2, width(), 1), borderColor);
    }

    const ShadResizable* resizable = qobject_cast<const ShadResizable*>(splitter());
    if (resizable == nullptr || !resizable->isGripVisible())
    {
        return;
    }
    // 上下排列时抓手旋转 90 度（对应 data-[panel-group-direction=vertical]:rotate-90）
    const int gripWidth = isVerticalLine ? kGripShortSide : kGripLongSide;
    const int gripHeight = isVerticalLine ? kGripLongSide : kGripShortSide;
    const QRectF gripRect((width() - gripWidth) / 2, (height() - gripHeight) / 2, gripWidth, gripHeight);
    ShadStyleHelper::fillRoundedRect(&painter, gripRect, kGripRadius, borderColor);
    ShadStyleHelper::strokeRoundedRect(&painter, gripRect, kGripRadius, borderColor);
    const ShadIconType::IconName gripIcon = isVerticalLine ? ShadIconType::GripVertical : ShadIconType::GripHorizontal;
    ShadIcon::paint(&painter, gripRect, gripIcon, shadTheme->getColor(ShadThemeType::Foreground), kGripIconSize);
}
