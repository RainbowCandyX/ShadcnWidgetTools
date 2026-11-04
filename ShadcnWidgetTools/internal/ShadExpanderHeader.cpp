#include "ShadExpanderHeader.h"

#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // px-4 py-3 gap-2
    constexpr int kPaddingX = 16;
    constexpr int kPaddingY = 12;
    constexpr int kMinHeight = 56;
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 8;
    constexpr qreal kHoverAlpha = 0.5;
} // namespace

ShadExpanderHeader::ShadExpanderHeader(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
#ifdef Q_OS_MACOS
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setMinimumHeight(kMinHeight);
    _layout = new QHBoxLayout(this);
    _layout->setContentsMargins(kPaddingX, kPaddingY, kPaddingX + kIconSize + kIconGap, kPaddingY);
    _layout->setSpacing(kIconGap);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadExpanderHeader::~ShadExpanderHeader()
{
}

QHBoxLayout* ShadExpanderHeader::contentLayout() const
{
    return _layout;
}

void ShadExpanderHeader::setProgress(qreal progress)
{
    _progress = qBound(0.0, progress, 1.0);
    update();
}

void ShadExpanderHeader::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        _isPressed = true;
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadExpanderHeader::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && _isPressed)
    {
        _isPressed = false;
        if (rect().contains(event->position().toPoint()))
        {
            Q_EMIT clicked();
        }
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadExpanderHeader::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space) && !event->isAutoRepeat())
    {
        Q_EMIT clicked();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadExpanderHeader::enterEvent(QEnterEvent* event)
{
    _isHovered = true;
    update();
    QWidget::enterEvent(event);
}

void ShadExpanderHeader::leaveEvent(QEvent* event)
{
    _isHovered = false;
    update();
    QWidget::leaveEvent(event);
}

void ShadExpanderHeader::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QWidget::focusInEvent(event);
    update();
}

void ShadExpanderHeader::focusOutEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QWidget::focusOutEvent(event);
    update();
}

void ShadExpanderHeader::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF bounds(rect());
    const qreal radius = qMax(0, shadTheme->getRadius(ShadThemeType::Large) - 1);
    // 展开后只有上方圆角
    const QRectF shape = _progress > 0.0 ? bounds.adjusted(0, 0, 0, radius) : bounds;
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    painter.setClipRect(bounds);
    if (_isHovered && isEnabled())
    {
        fillRoundedRect(&painter, shape, radius, withAlpha(shadTheme->getColor(ShadThemeType::Muted), kHoverAlpha));
    }

    const QRectF logicalIconRect(bounds.width() - kPaddingX - kIconSize, (bounds.height() - kIconSize) / 2.0, kIconSize, kIconSize);
    const QRectF iconRect = visualRect(layoutDirection(), bounds, logicalIconRect);
    painter.save();
    painter.translate(iconRect.center());
    painter.rotate(180.0 * _progress);
    painter.translate(-iconRect.center());
    ShadIcon::paint(&painter, iconRect, ShadIconType::ChevronDown, shadTheme->getColor(ShadThemeType::MutedForeground), kIconSize);
    painter.restore();

    if (hasFocus() && _isFocusVisible)
    {
        const QRectF box = bounds.adjusted(kFocusRingWidth, kFocusRingWidth, -kFocusRingWidth, -kFocusRingWidth);
        const qreal ringRadius = qMax<qreal>(0.0, radius - kFocusRingWidth);
        strokeRoundedRect(&painter, box, ringRadius, shadTheme->getColor(ShadThemeType::Ring));
        drawFocusRing(&painter, box, ringRadius, focusRingColor(false));
    }
}
