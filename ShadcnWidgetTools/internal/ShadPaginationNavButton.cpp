#include "ShadPaginationNavButton.h"

#include <QColor>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // default 尺寸 h-9；图标 size-4，图标与文字 gap-2
    constexpr int kButtonHeight = 36;
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 8;
    // 箭头一侧 pl-2 / pr-2，文字一侧 has-[>svg]:px-3
    constexpr int kIconSidePadding = 8;
    constexpr int kTextSidePadding = 12;
} // namespace

ShadPaginationNavButton::ShadPaginationNavButton(bool isNext, QWidget* parent)
    : ShadButton(QString(), ShadButtonType::Ghost, parent), _isNext(isNext)
{
    setButtonSize(ShadButtonType::Medium);
}

ShadPaginationNavButton::~ShadPaginationNavButton()
{
}

void ShadPaginationNavButton::setLabel(const QString& label)
{
    _label = label;
    setAccessibleName(label);
    if (!_isLabelVisible)
    {
        setToolTip(label);
    }
    updateGeometry();
    update();
}

QString ShadPaginationNavButton::getLabel() const
{
    return _label;
}

void ShadPaginationNavButton::setLabelVisible(bool visible)
{
    _isLabelVisible = visible;
    // 只显示图标时用提示说明按钮用途
    setToolTip(visible ? QString() : _label);
    updateGeometry();
    update();
}

bool ShadPaginationNavButton::isLabelVisible() const
{
    return _isLabelVisible;
}

QSize ShadPaginationNavButton::sizeHint() const
{
    const int ring = ShadStyleHelper::kFocusRingWidth;
    int width = kButtonHeight;
    if (_isLabelVisible && !_label.isEmpty())
    {
        width = kIconSidePadding + kIconSize + kIconGap + ShadStyleHelper::textWidth(font(), _label) + kTextSidePadding;
    }
    return QSize(width + 2 * ring, kButtonHeight + 2 * ring);
}

QSize ShadPaginationNavButton::minimumSizeHint() const
{
    return sizeHint();
}

void ShadPaginationNavButton::paintEvent(QPaintEvent* event)
{
    // 背景、悬停与焦点环由 ShadButton 绘制（本按钮没有文字与图标）
    ShadButton::paintEvent(event);
    using namespace ShadStyleHelper;
    const QRect bodyRect = insetRect(rect(), kFocusRingWidth).toRect();
    const Qt::LayoutDirection direction = layoutDirection();
    const bool isRightToLeft = direction == Qt::RightToLeft;
    // 上一页指向起始侧、下一页指向结束侧，RTL 时方向翻转
    const ShadIconType::IconName icon = (_isNext != isRightToLeft) ? ShadIconType::ChevronRight : ShadIconType::ChevronLeft;
    const QColor color = shadTheme->getColor(ShadThemeType::Foreground);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    if (!isEnabled())
    {
        painter.setOpacity(kDisabledOpacity);
    }
    if (!_isLabelVisible || _label.isEmpty())
    {
        ShadIcon::paint(&painter, QRectF(bodyRect), icon, color, kIconSize);
        return;
    }
    // 按 LTR 计算：上一页为 [图标][文字]，下一页为 [文字][图标]，再在按钮内镜像
    const int textWidth = ShadStyleHelper::textWidth(font(), _label);
    const int iconTop = bodyRect.top() + (bodyRect.height() - kIconSize) / 2;
    QRect iconRect;
    QRect textRect;
    if (_isNext)
    {
        textRect = QRect(bodyRect.left() + kTextSidePadding, bodyRect.top(), textWidth, bodyRect.height());
        iconRect = QRect(textRect.right() + 1 + kIconGap, iconTop, kIconSize, kIconSize);
    }
    else
    {
        iconRect = QRect(bodyRect.left() + kIconSidePadding, iconTop, kIconSize, kIconSize);
        textRect = QRect(iconRect.right() + 1 + kIconGap, bodyRect.top(), textWidth, bodyRect.height());
    }
    ShadIcon::paint(&painter, QRectF(visualRect(direction, bodyRect, iconRect)), icon, color, kIconSize);
    painter.setFont(font());
    painter.setPen(color);
    painter.drawText(visualRect(direction, bodyRect, textRect), Qt::AlignCenter, _label);
}
