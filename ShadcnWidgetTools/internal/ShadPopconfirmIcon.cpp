#include "ShadPopconfirmIcon.h"

#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSize>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kIconSize = 16;
}

ShadPopconfirmIcon::ShadPopconfirmIcon(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(sizeHint());
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadPopconfirmIcon::~ShadPopconfirmIcon()
{
}

void ShadPopconfirmIcon::setIcon(ShadIconType::IconName icon)
{
    if (_icon == icon)
    {
        return;
    }
    _icon = icon;
    setVisible(icon != ShadIconType::None);
    update();
}

ShadIconType::IconName ShadPopconfirmIcon::getIcon() const
{
    return _icon;
}

void ShadPopconfirmIcon::setDestructive(bool destructive)
{
    if (_isDestructive == destructive)
    {
        return;
    }
    _isDestructive = destructive;
    update();
}

void ShadPopconfirmIcon::setLineHeight(int height)
{
    const int lineHeight = qMax(kIconSize, height);
    if (_lineHeight == lineHeight)
    {
        return;
    }
    _lineHeight = lineHeight;
    setFixedSize(sizeHint());
}

QSize ShadPopconfirmIcon::sizeHint() const
{
    return QSize(kIconSize, _lineHeight);
}

void ShadPopconfirmIcon::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (_icon == ShadIconType::None)
    {
        return;
    }
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    const QColor color = shadTheme->getColor(_isDestructive ? ShadThemeType::Destructive : ShadThemeType::Foreground);
    const QRectF iconRect(0.0, (height() - kIconSize) / 2.0, kIconSize, kIconSize);
    ShadIcon::paint(&painter, iconRect, _icon, color, kIconSize);
}
