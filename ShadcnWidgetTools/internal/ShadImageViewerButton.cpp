#include "ShadImageViewerButton.h"

#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadIcon.h"
#include "ShadImageViewerToolbar.h"
#include "ShadStyleHelper.h"
#include "ShadTransition.h"

namespace
{
    constexpr int kRoundSize = 40;
    constexpr int kSquareSize = 32;
    constexpr int kRoundIconSize = 20;
    constexpr int kSquareIconSize = 16;
    constexpr qreal kSquareRadius = 6.0;
    constexpr qreal kDisabledOpacity = 0.35;
    const QColor kRoundHoverColor(39, 39, 42, 230);
    const QColor kSquareHoverColor(255, 255, 255, 31);
    const QColor kIconColor(255, 255, 255);
} // namespace

ShadImageViewerButton::ShadImageViewerButton(ShadIconType::IconName icon, Shape shape, QWidget* parent)
    : QAbstractButton(parent), _icon(icon), _shape(shape)
{
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
    const int size = _shape == Round ? kRoundSize : kSquareSize;
    setFixedSize(size, size);
    _hoverTransition = new ShadTransition(this);
}

ShadImageViewerButton::~ShadImageViewerButton()
{
}

void ShadImageViewerButton::setShadIcon(ShadIconType::IconName icon)
{
    if (_icon == icon)
    {
        return;
    }
    _icon = icon;
    update();
}

ShadIconType::IconName ShadImageViewerButton::getShadIcon() const
{
    return _icon;
}

QSize ShadImageViewerButton::sizeHint() const
{
    const int size = _shape == Round ? kRoundSize : kSquareSize;
    return QSize(size, size);
}

void ShadImageViewerButton::enterEvent(QEnterEvent* event)
{
    if (isEnabled())
    {
        _hoverTransition->animateTo(1.0);
    }
    QAbstractButton::enterEvent(event);
}

void ShadImageViewerButton::leaveEvent(QEvent* event)
{
    _hoverTransition->animateTo(0.0);
    QAbstractButton::leaveEvent(event);
}

void ShadImageViewerButton::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange && !isEnabled())
    {
        _hoverTransition->jumpTo(0.0);
    }
    QAbstractButton::changeEvent(event);
}

void ShadImageViewerButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (!isEnabled())
    {
        painter.setOpacity(kDisabledOpacity);
    }
    const qreal hover = _hoverTransition->value();
    const QRectF bounds = QRectF(rect());
    int iconSize = kSquareIconSize;
    if (_shape == Round)
    {
        iconSize = kRoundIconSize;
        const QColor background = ShadStyleHelper::mix(ShadImageViewerToolbar::backgroundColor(), kRoundHoverColor, hover);
        const qreal radius = bounds.height() / 2.0;
        ShadStyleHelper::fillRoundedRect(&painter, bounds, radius, background);
        ShadStyleHelper::strokeRoundedRect(&painter, bounds, radius, ShadImageViewerToolbar::borderColor());
    }
    else if (hover > 0.0)
    {
        ShadStyleHelper::fillRoundedRect(&painter, bounds, kSquareRadius, ShadStyleHelper::withAlpha(kSquareHoverColor, hover));
    }
    const QRectF iconRect((bounds.width() - iconSize) / 2.0, (bounds.height() - iconSize) / 2.0, iconSize, iconSize);
    ShadIcon::paint(&painter, iconRect, _icon, kIconColor, iconSize);
}
