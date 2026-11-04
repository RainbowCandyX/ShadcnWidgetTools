#include "ShadPropertyGridGroupHeader.h"

#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kHeight = 32;
    constexpr int kFontSize = 14;
    constexpr int kPaddingX = 8;
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 4;
    constexpr qreal kRestAlpha = 0.5;
} // namespace

ShadPropertyGridGroupHeader::ShadPropertyGridGroupHeader(QWidget* parent)
    : QAbstractButton(parent)
{
    setCheckable(true);
    setChecked(true);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover);
    setFixedHeight(kHeight);
    ShadStyleHelper::applyFont(this, kFontSize, QFont::Medium);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadPropertyGridGroupHeader::~ShadPropertyGridGroupHeader()
{
}

QSize ShadPropertyGridGroupHeader::sizeHint() const
{
    return QSize(2 * kPaddingX + kIconSize + kIconGap + ShadStyleHelper::textWidth(font(), text()), kHeight);
}

QSize ShadPropertyGridGroupHeader::minimumSizeHint() const
{
    return QSize(2 * kPaddingX + kIconSize, kHeight);
}

void ShadPropertyGridGroupHeader::enterEvent(QEnterEvent* event)
{
    update();
    QAbstractButton::enterEvent(event);
}

void ShadPropertyGridGroupHeader::leaveEvent(QEvent* event)
{
    update();
    QAbstractButton::leaveEvent(event);
}

void ShadPropertyGridGroupHeader::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        update();
    }
    QAbstractButton::changeEvent(event);
}

void ShadPropertyGridGroupHeader::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    const Qt::LayoutDirection direction = layoutDirection();
    const bool isRightToLeft = direction == Qt::RightToLeft;
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    const QColor muted = shadTheme->getColor(ShadThemeType::Muted);
    painter.fillRect(rect(), underMouse() ? muted : ShadStyleHelper::withAlpha(muted, kRestAlpha));
    painter.fillRect(QRect(0, height() - 1, width(), 1), shadTheme->getColor(ShadThemeType::Border));
    ShadIconType::IconName icon = ShadIconType::ChevronDown;
    if (!isChecked())
    {
        icon = isRightToLeft ? ShadIconType::ChevronLeft : ShadIconType::ChevronRight;
    }
    const QRect iconRect = ShadStyleHelper::visualRect(direction, rect(), QRect(kPaddingX, (height() - kIconSize) / 2, kIconSize, kIconSize));
    ShadIcon::paint(&painter, QRectF(iconRect), icon, shadTheme->getColor(ShadThemeType::MutedForeground), kIconSize);
    const int textLeft = kPaddingX + kIconSize + kIconGap;
    const QRect textRect = ShadStyleHelper::visualRect(direction, rect(), QRect(textLeft, 0, qMax(0, width() - textLeft - kPaddingX), height()));
    painter.setFont(font());
    painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
    const Qt::Alignment alignment = ShadStyleHelper::visualAlignment(direction, Qt::AlignLeft) | Qt::AlignVCenter;
    painter.drawText(textRect, static_cast<int>(alignment), ShadStyleHelper::elideText(font(), text(), textRect.width()));
}
