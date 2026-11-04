#include "ShadToggle.h"

#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QIcon>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QtMath>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTogglePrivate.h"
#include "ShadTransition.h"

namespace
{
    constexpr int kIconSize = 16;
    // 图标与文字间距，对应 gap-2
    constexpr int kContentGap = 8;
} // namespace

ShadToggle::ShadToggle(QWidget* parent)
    : QAbstractButton(parent), d_ptr(new ShadTogglePrivate())
{
    Q_D(ShadToggle);
    d->q_ptr = this;
    d->_hoverTransition = new ShadTransition(this);
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadToggle::ShadToggle(const QString& text, QWidget* parent)
    : ShadToggle(parent)
{
    setText(text);
}

ShadToggle::ShadToggle(ShadIconType::IconName icon, QWidget* parent)
    : ShadToggle(parent)
{
    setShadIcon(icon);
}

ShadToggle::~ShadToggle()
{
}

void ShadToggle::setVariant(ShadToggleType::Variant variant)
{
    Q_D(ShadToggle);
    if (d->_variant == variant)
    {
        return;
    }
    d->_variant = variant;
    update();
    Q_EMIT variantChanged();
}

ShadToggleType::Variant ShadToggle::getVariant() const
{
    Q_D(const ShadToggle);
    return d->_variant;
}

void ShadToggle::setToggleSize(ShadToggleType::Size size)
{
    Q_D(ShadToggle);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    updateGeometry();
    update();
    Q_EMIT toggleSizeChanged();
}

ShadToggleType::Size ShadToggle::getToggleSize() const
{
    Q_D(const ShadToggle);
    return d->_size;
}

void ShadToggle::setShadIcon(ShadIconType::IconName icon)
{
    Q_D(ShadToggle);
    if (d->_shadIcon == icon)
    {
        return;
    }
    d->_shadIcon = icon;
    updateGeometry();
    update();
}

ShadIconType::IconName ShadToggle::getShadIcon() const
{
    Q_D(const ShadToggle);
    return d->_shadIcon;
}

void ShadToggle::setContentAlignment(Qt::Alignment alignment)
{
    Q_D(ShadToggle);
    d->_contentAlignment = alignment;
    update();
}

Qt::Alignment ShadToggle::getContentAlignment() const
{
    Q_D(const ShadToggle);
    return d->_contentAlignment;
}

QSize ShadToggle::sizeHint() const
{
    Q_D(const ShadToggle);
    const ShadTogglePrivate::SizeMetrics metrics = ShadTogglePrivate::sizeMetrics(d->_size);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const int contentWidth = qCeil(ShadStyleHelper::iconTextWidth(font(), text(), d->hasIcon(), kContentGap, kIconSize));
    return QSize(qMax(metrics.minWidth, contentWidth + 2 * metrics.paddingX) + margin, metrics.height + margin);
}

QSize ShadToggle::minimumSizeHint() const
{
    Q_D(const ShadToggle);
    const ShadTogglePrivate::SizeMetrics metrics = ShadTogglePrivate::sizeMetrics(d->_size);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    return QSize(metrics.minWidth + margin, metrics.height + margin);
}

void ShadToggle::enterEvent(QEnterEvent* event)
{
    Q_D(ShadToggle);
    if (isEnabled())
    {
        d->_hoverTransition->animateTo(1.0);
    }
    QAbstractButton::enterEvent(event);
}

void ShadToggle::leaveEvent(QEvent* event)
{
    Q_D(ShadToggle);
    d->_hoverTransition->animateTo(0.0);
    QAbstractButton::leaveEvent(event);
}

void ShadToggle::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadToggle);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QAbstractButton::focusInEvent(event);
    d->updateGroupMask();
    update();
}

void ShadToggle::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadToggle);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QAbstractButton::focusOutEvent(event);
    d->updateGroupMask();
    update();
}

void ShadToggle::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadToggle);
    QAbstractButton::resizeEvent(event);
    d->updateGroupMask();
}

void ShadToggle::changeEvent(QEvent* event)
{
    Q_D(ShadToggle);
    if (event->type() == QEvent::EnabledChange)
    {
        d->_hoverTransition->jumpTo(isEnabled() && underMouse() ? 1.0 : 0.0);
    }
    else if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QAbstractButton::changeEvent(event);
}

void ShadToggle::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadToggle);
    using namespace ShadStyleHelper;
    const ShadTogglePrivate::SizeMetrics metrics = ShadTogglePrivate::sizeMetrics(d->_size);
    const bool isOutline = d->_variant == ShadToggleType::Outline;
    const qreal progress = d->_hoverTransition->value();
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const bool showFocusRing = hasFocus() && d->_isFocusVisible;

    // 悬停配色对应 default: hover:bg-muted hover:text-muted-foreground；outline: hover:bg-accent hover:text-accent-foreground
    // 选中态 data-[state=on]:bg-accent data-[state=on]:text-accent-foreground 优先于 hover
    const QColor transparent(0, 0, 0, 0);
    const QColor hoverBackground = shadTheme->getColor(isOutline ? ShadThemeType::Accent : ShadThemeType::Muted);
    const QColor hoverText = shadTheme->getColor(isOutline ? ShadThemeType::AccentForeground : ShadThemeType::MutedForeground);
    const QColor background = isChecked() ? selectionColor(ShadThemeType::Accent) : mix(transparent, hoverBackground, progress);
    const QColor textColor = isChecked() ? shadTheme->getColor(ShadThemeType::AccentForeground) : mix(shadTheme->getColor(ShadThemeType::Foreground), hoverText, progress);

    // 在 ShadToggleGroup 中只有外侧两端圆角（first:rounded-l-md last:rounded-r-md），RTL 时第一个按钮在右侧
    using GroupPosition = ShadTogglePrivate::GroupPosition;
    const bool isGrouped = d->_groupPosition != GroupPosition::Standalone;
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    const bool roundStart = d->_groupPosition == GroupPosition::Standalone || d->_groupPosition == GroupPosition::First;
    const bool roundEnd = d->_groupPosition == GroupPosition::Standalone || d->_groupPosition == GroupPosition::Last;
    const bool roundLeft = isRightToLeft ? roundEnd : roundStart;
    const bool roundRight = isRightToLeft ? roundStart : roundEnd;
    // outline 变体在组内与前一个按钮重叠 1px 共用边框，自身起始侧边框不绘制（border-l-0，RTL 时为右边框）
    const bool skipStartBorder = isOutline && isGrouped && !roundStart;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    if (isOutline && !isGrouped)
    {
        drawShadowXs(&painter, bodyRect, radius);
    }
    painter.save();
    if (skipStartBorder)
    {
        painter.setClipRect(isRightToLeft ? QRectF(0, 0, bodyRect.right() - 1.0, height()) : QRectF(bodyRect.left() + 1.0, 0, width(), height()));
    }
    fillRoundedRect(&painter, bodyRect, radius, background, roundLeft, roundRight);
    if (isOutline)
    {
        strokeRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(showFocusRing ? ShadThemeType::Ring : ShadThemeType::Input), 1.0, roundLeft, roundRight);
    }
    painter.restore();
    if (showFocusRing)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(false), kFocusRingWidth, roundLeft, roundRight);
    }

    const QRectF contentRect = bodyRect.adjusted(metrics.paddingX, 0, -metrics.paddingX, 0);
    drawIconText(&painter, contentRect, d->_shadIcon, icon(), text(), font(), textColor, kContentGap, d->_contentAlignment, kIconSize, 0.0, layoutDirection());
}
