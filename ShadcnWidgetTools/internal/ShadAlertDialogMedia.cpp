#include "ShadAlertDialogMedia.h"

#include <QColor>
#include <QMargins>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QRect>
#include <QRectF>
#include <QSizeF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>

#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // size-16
    constexpr int kMediaSize = 64;
    // *:[svg]:size-8
    constexpr int kIconSize = 32;
    // mb-2
    constexpr int kBottomMargin = 8;
} // namespace

ShadAlertDialogMedia::ShadAlertDialogMedia(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setContentsMargins(0, 0, 0, kBottomMargin);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadAlertDialogMedia::~ShadAlertDialogMedia()
{
}

void ShadAlertDialogMedia::setIcon(ShadIconType::IconName icon)
{
    _icon = icon;
    update();
}

ShadIconType::IconName ShadAlertDialogMedia::getIcon() const
{
    return _icon;
}

void ShadAlertDialogMedia::setPixmap(const QPixmap& pixmap)
{
    _pixmap = pixmap;
    update();
}

QPixmap ShadAlertDialogMedia::getPixmap() const
{
    return _pixmap;
}

void ShadAlertDialogMedia::setVariant(ShadAlertDialogType::MediaVariant variant)
{
    _variant = variant;
    update();
}

ShadAlertDialogType::MediaVariant ShadAlertDialogMedia::getVariant() const
{
    return _variant;
}

bool ShadAlertDialogMedia::hasContent() const
{
    return _icon != ShadIconType::None || !_pixmap.isNull();
}

QSize ShadAlertDialogMedia::sizeHint() const
{
    const QMargins margins = contentsMargins();
    return QSize(kMediaSize + margins.left() + margins.right(), kMediaSize + margins.top() + margins.bottom());
}

QSize ShadAlertDialogMedia::minimumSizeHint() const
{
    return sizeHint();
}

void ShadAlertDialogMedia::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    const QRectF mediaRect(contentsRect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    if (!_pixmap.isNull())
    {
        // object-cover：按物理像素缩放到填满，再从中间裁剪
        const QSize pixelSize = (mediaRect.size() * devicePixelRatioF()).toSize();
        QPixmap scaled = _pixmap.scaled(pixelSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        scaled.setDevicePixelRatio(1.0);
        const QRectF sourceRect((scaled.width() - pixelSize.width()) / 2, (scaled.height() - pixelSize.height()) / 2, pixelSize.width(), pixelSize.height());
        painter.setClipPath(ShadStyleHelper::roundedRectPath(mediaRect, radius));
        painter.drawPixmap(mediaRect, scaled, sourceRect);
        return;
    }
    const bool isDestructive = _variant == ShadAlertDialogType::Destructive;
    const QColor destructive = shadTheme->getColor(ShadThemeType::Destructive);
    const QColor background = isDestructive ? ShadStyleHelper::withAlpha(destructive, shadTheme->isDark() ? 0.2 : 0.1) : shadTheme->getColor(ShadThemeType::Muted);
    ShadStyleHelper::fillRoundedRect(&painter, mediaRect, radius, background);
    if (_icon != ShadIconType::None)
    {
        const QColor iconColor = isDestructive ? destructive : shadTheme->getColor(ShadThemeType::Foreground);
        ShadIcon::paint(&painter, mediaRect, _icon, iconColor, kIconSize);
    }
}
