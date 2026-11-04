#include "ShadAvatar.h"

#include <QEvent>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>

#include "ShadAvatarPrivate.h"
#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadAvatar::ShadAvatar(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadAvatarPrivate())
{
    Q_D(ShadAvatar);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // 圆角随主题变化，缓存的图片需重新生成
    connect(shadTheme, &ShadTheme::themeChanged, this, [this, d]()
    {
        if (d->_shape == ShadAvatarType::Rounded)
        {
            d->invalidateCache();
        }
        update();
    });
}

ShadAvatar::ShadAvatar(const QString& fallbackText, QWidget* parent)
    : ShadAvatar(parent)
{
    setFallbackText(fallbackText);
}

ShadAvatar::~ShadAvatar()
{
}

void ShadAvatar::setAvatarSize(ShadAvatarType::Size size)
{
    Q_D(ShadAvatar);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    d->invalidateCache();
    updateGeometry();
    update();
}

ShadAvatarType::Size ShadAvatar::getAvatarSize() const
{
    Q_D(const ShadAvatar);
    return d->_size;
}

void ShadAvatar::setShape(ShadAvatarType::Shape shape)
{
    Q_D(ShadAvatar);
    if (d->_shape == shape)
    {
        return;
    }
    d->_shape = shape;
    d->invalidateCache();
    update();
}

ShadAvatarType::Shape ShadAvatar::getShape() const
{
    Q_D(const ShadAvatar);
    return d->_shape;
}

void ShadAvatar::setImage(const QPixmap& image)
{
    Q_D(ShadAvatar);
    d->_image = image;
    d->invalidateCache();
    update();
}

QPixmap ShadAvatar::getImage() const
{
    Q_D(const ShadAvatar);
    return d->_image;
}

bool ShadAvatar::setImagePath(const QString& path)
{
    QPixmap image;
    const bool isLoaded = !path.isEmpty() && image.load(path);
    setImage(isLoaded ? image : QPixmap());
    return isLoaded;
}

bool ShadAvatar::hasImage() const
{
    Q_D(const ShadAvatar);
    return !d->_image.isNull();
}

void ShadAvatar::setFallbackText(const QString& text)
{
    Q_D(ShadAvatar);
    d->_fallbackText = text;
    update();
}

QString ShadAvatar::getFallbackText() const
{
    Q_D(const ShadAvatar);
    return d->_fallbackText;
}

void ShadAvatar::setFallbackIcon(ShadIconType::IconName icon)
{
    Q_D(ShadAvatar);
    d->_fallbackIcon = icon;
    update();
}

ShadIconType::IconName ShadAvatar::getFallbackIcon() const
{
    Q_D(const ShadAvatar);
    return d->_fallbackIcon;
}

void ShadAvatar::setGrayscale(bool grayscale)
{
    Q_D(ShadAvatar);
    if (d->_isGrayscale == grayscale)
    {
        return;
    }
    d->_isGrayscale = grayscale;
    d->invalidateCache();
    update();
}

bool ShadAvatar::isGrayscale() const
{
    Q_D(const ShadAvatar);
    return d->_isGrayscale;
}

void ShadAvatar::setBadgeVisible(bool visible)
{
    Q_D(ShadAvatar);
    d->_isBadgeVisible = visible;
    update();
}

bool ShadAvatar::isBadgeVisible() const
{
    Q_D(const ShadAvatar);
    return d->_isBadgeVisible;
}

void ShadAvatar::setBadgeIcon(ShadIconType::IconName icon)
{
    Q_D(ShadAvatar);
    d->_badgeIcon = icon;
    update();
}

ShadIconType::IconName ShadAvatar::getBadgeIcon() const
{
    Q_D(const ShadAvatar);
    return d->_badgeIcon;
}

void ShadAvatar::setBadgeColor(const QColor& color)
{
    Q_D(ShadAvatar);
    d->_badgeColor = color;
    update();
}

QColor ShadAvatar::getBadgeColor() const
{
    Q_D(const ShadAvatar);
    return d->_badgeColor.isValid() ? d->_badgeColor : shadTheme->getColor(ShadThemeType::Primary);
}

QSize ShadAvatar::sizeHint() const
{
    Q_D(const ShadAvatar);
    const int side = ShadAvatarPrivate::diameter(d->_size) + 2 * ShadAvatarPrivate::kRingWidth;
    return QSize(side, side);
}

QSize ShadAvatar::minimumSizeHint() const
{
    return sizeHint();
}

void ShadAvatar::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadAvatar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadAvatar);
    using namespace ShadThemeType;
    const qreal diameter = ShadAvatarPrivate::diameter(d->_size);
    const QRectF circleRect(ShadAvatarPrivate::kRingWidth, ShadAvatarPrivate::kRingWidth, diameter, diameter);
    const qreal radius = d->cornerRadius(diameter);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);

    // 头像组中的 ring-2 ring-background
    if (d->_isGroupRingVisible)
    {
        ShadAvatarPrivate::drawRing(&painter, circleRect, radius);
    }

    if (hasImage())
    {
        painter.drawPixmap(circleRect.topLeft(), d->shapedImage(devicePixelRatioF()));
    }
    else
    {
        // AvatarFallback：bg-muted text-muted-foreground，外形与头像一致
        painter.setPen(Qt::NoPen);
        painter.setBrush(shadTheme->getColor(Muted));
        painter.drawRoundedRect(circleRect, radius, radius);
        const QColor foreground = shadTheme->getColor(MutedForeground);
        if (d->_fallbackIcon != ShadIconType::None)
        {
            ShadIcon::paint(&painter, circleRect, d->_fallbackIcon, foreground, ShadAvatarPrivate::iconSize(d->_size));
        }
        else if (!d->_fallbackText.isEmpty())
        {
            QFont fallbackFont = font();
            fallbackFont.setPixelSize(ShadAvatarPrivate::fallbackPixelSize(d->_size));
            fallbackFont.setWeight(QFont::Normal);
            painter.setFont(fallbackFont);
            painter.setPen(foreground);
            painter.drawText(circleRect, Qt::AlignCenter, d->_fallbackText);
        }
    }

    // after:border after:border-border after:mix-blend-darken dark:after:mix-blend-lighten：1px 细线画在外形内侧
    painter.save();
    painter.setCompositionMode(shadTheme->isDark() ? QPainter::CompositionMode_Lighten : QPainter::CompositionMode_Darken);
    painter.setPen(QPen(shadTheme->getColor(Border), 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(circleRect.adjusted(0.5, 0.5, -0.5, -0.5), radius - 0.5, radius - 0.5);
    painter.restore();

    // AvatarBadge：absolute right-0 bottom-0 rounded-full ring-2 ring-background，RTL 下位于左下角
    if (d->_isBadgeVisible)
    {
        const qreal badgeDiameter = ShadAvatarPrivate::badgeDiameter(d->_size);
        const QRectF logicalBadgeRect(circleRect.right() - badgeDiameter, circleRect.bottom() - badgeDiameter, badgeDiameter, badgeDiameter);
        const QRectF badgeRect = ShadStyleHelper::visualRect(layoutDirection(), QRectF(rect()), logicalBadgeRect);
        ShadAvatarPrivate::drawRing(&painter, badgeRect, badgeDiameter / 2.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(getBadgeColor());
        painter.drawEllipse(badgeRect);
        // size="sm" 时 [&>svg]:hidden
        if (d->_badgeIcon != ShadIconType::None && d->_size != ShadAvatarType::Small)
        {
            ShadIcon::paint(&painter, badgeRect, d->_badgeIcon, shadTheme->getColor(PrimaryForeground), ShadAvatarPrivate::kBadgeIconSize);
        }
    }
}
