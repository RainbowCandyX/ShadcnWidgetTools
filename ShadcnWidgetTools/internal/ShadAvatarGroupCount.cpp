#include "ShadAvatarGroupCount.h"

#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>

#include "ShadAvatarPrivate.h"
#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // text-sm，三种尺寸相同
    constexpr int kTextPixelSize = 14;
} // namespace

ShadAvatarGroupCount::ShadAvatarGroupCount(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadAvatarGroupCount::~ShadAvatarGroupCount()
{
}

void ShadAvatarGroupCount::setAvatarSize(ShadAvatarType::Size size)
{
    if (_size == size)
    {
        return;
    }
    _size = size;
    updateGeometry();
    update();
}

void ShadAvatarGroupCount::setText(const QString& text)
{
    _text = text;
    update();
}

QString ShadAvatarGroupCount::getText() const
{
    return _text;
}

void ShadAvatarGroupCount::setIcon(ShadIconType::IconName icon)
{
    _icon = icon;
    update();
}

ShadIconType::IconName ShadAvatarGroupCount::getIcon() const
{
    return _icon;
}

QSize ShadAvatarGroupCount::sizeHint() const
{
    const int side = ShadAvatarPrivate::diameter(_size) + 2 * ShadAvatarPrivate::kRingWidth;
    return QSize(side, side);
}

QSize ShadAvatarGroupCount::minimumSizeHint() const
{
    return sizeHint();
}

void ShadAvatarGroupCount::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    const qreal diameter = ShadAvatarPrivate::diameter(_size);
    const QRectF circleRect(ShadAvatarPrivate::kRingWidth, ShadAvatarPrivate::kRingWidth, diameter, diameter);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
    ShadAvatarPrivate::drawRing(&painter, circleRect, diameter / 2.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(shadTheme->getColor(ShadThemeType::Muted));
    painter.drawEllipse(circleRect);
    const QColor foreground = shadTheme->getColor(ShadThemeType::MutedForeground);
    if (_icon != ShadIconType::None)
    {
        ShadIcon::paint(&painter, circleRect, _icon, foreground, ShadAvatarPrivate::iconSize(_size));
        return;
    }
    QFont textFont = font();
    textFont.setPixelSize(kTextPixelSize);
    textFont.setWeight(QFont::Normal);
    painter.setFont(textFont);
    painter.setPen(foreground);
    painter.drawText(circleRect, Qt::AlignCenter, _text);
}
