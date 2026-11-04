#include "ShadBadge.h"

#include <QColor>
#include <QEvent>
#include <QFontMetricsF>
#include <QHideEvent>
#include <QIcon>
#include <QObject>
#include <QPaintEvent>
#include <QShowEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QVariantAnimation>
#include <QtMath>

#include "ShadBadgePrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 尺寸对应 rounded-md border px-2 py-0.5 text-xs gap-1 [&>svg]:size-3
    constexpr int kPaddingX = 8;
    constexpr int kPaddingY = 2;
    constexpr int kBorderWidth = 1;
    constexpr int kLineHeight = 16;
    constexpr int kIconSize = 12;
    constexpr int kContentGap = 4;
} // namespace

ShadBadge::ShadBadge(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadBadgePrivate())
{
    Q_D(ShadBadge);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 12, QFont::Medium);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadBadge::ShadBadge(const QString& text, QWidget* parent)
    : ShadBadge(parent)
{
    setText(text);
}

ShadBadge::ShadBadge(const QString& text, ShadBadgeType::Variant variant, QWidget* parent)
    : ShadBadge(parent)
{
    setText(text);
    setVariant(variant);
}

ShadBadge::~ShadBadge()
{
    Q_D(ShadBadge);
    // 私有数据先于子对象释放，断开捕获 d 的连接
    if (d->_spinAnimation != nullptr)
    {
        QObject::disconnect(d->_spinAnimation, nullptr, this, nullptr);
    }
}

void ShadBadge::setText(const QString& text)
{
    Q_D(ShadBadge);
    if (d->_text == text)
    {
        return;
    }
    d->_text = text;
    updateGeometry();
    update();
    Q_EMIT textChanged();
}

QString ShadBadge::getText() const
{
    Q_D(const ShadBadge);
    return d->_text;
}

void ShadBadge::setVariant(ShadBadgeType::Variant variant)
{
    Q_D(ShadBadge);
    if (d->_variant == variant)
    {
        return;
    }
    d->_variant = variant;
    update();
    Q_EMIT variantChanged();
}

ShadBadgeType::Variant ShadBadge::getVariant() const
{
    Q_D(const ShadBadge);
    return d->_variant;
}

void ShadBadge::setShadIcon(ShadIconType::IconName icon)
{
    Q_D(ShadBadge);
    if (d->_shadIcon == icon)
    {
        return;
    }
    d->_shadIcon = icon;
    d->updateSpin();
    updateGeometry();
    update();
}

ShadIconType::IconName ShadBadge::getShadIcon() const
{
    Q_D(const ShadBadge);
    return d->_shadIcon;
}

QSize ShadBadge::sizeHint() const
{
    Q_D(const ShadBadge);
    const int contentWidth = qCeil(ShadStyleHelper::iconTextWidth(font(), d->_text, d->_shadIcon != ShadIconType::None, kContentGap, kIconSize));
    const int lineHeight = qMax(kLineHeight, qCeil(QFontMetricsF(font()).height()));
    return QSize(contentWidth + 2 * (kPaddingX + kBorderWidth), lineHeight + 2 * (kPaddingY + kBorderWidth));
}

QSize ShadBadge::minimumSizeHint() const
{
    return sizeHint();
}

void ShadBadge::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QWidget::changeEvent(event);
}

void ShadBadge::showEvent(QShowEvent* event)
{
    Q_D(ShadBadge);
    QWidget::showEvent(event);
    d->updateSpin();
}

void ShadBadge::hideEvent(QHideEvent* event)
{
    Q_D(ShadBadge);
    QWidget::hideEvent(event);
    d->updateSpin();
}

void ShadBadge::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadBadge);
    using namespace ShadStyleHelper;
    using namespace ShadThemeType;
    const QRectF bodyRect(rect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QColor transparent(0, 0, 0, 0);
    QColor background;
    QColor textColor;
    QColor borderColor = transparent;
    switch (d->_variant)
    {
        case ShadBadgeType::Secondary:
            background = shadTheme->getColor(Secondary);
            textColor = shadTheme->getColor(SecondaryForeground);
            break;
        case ShadBadgeType::Destructive:
            // 对应 bg-destructive text-white dark:bg-destructive/60
            background = shadTheme->isDark() ? withAlpha(shadTheme->getColor(Destructive), 0.6) : shadTheme->getColor(Destructive);
            textColor = shadTheme->getColor(DestructiveForeground);
            break;
        case ShadBadgeType::Outline:
            background = transparent;
            textColor = shadTheme->getColor(Foreground);
            borderColor = shadTheme->getColor(Border);
            break;
        case ShadBadgeType::Default:
        default:
            background = shadTheme->getColor(Primary);
            textColor = shadTheme->getColor(PrimaryForeground);
            break;
    }

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    fillRoundedRect(&painter, bodyRect, radius, background);
    strokeRoundedRect(&painter, bodyRect, radius, borderColor);
    const QRectF contentRect = bodyRect.adjusted(kPaddingX + kBorderWidth, 0, -(kPaddingX + kBorderWidth), 0);
    drawIconText(&painter, contentRect, d->_shadIcon, QIcon(), d->_text, font(), textColor, kContentGap, Qt::AlignHCenter, kIconSize, d->_spinAngle, layoutDirection());
}
