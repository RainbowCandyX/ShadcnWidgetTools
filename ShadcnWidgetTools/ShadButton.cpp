#include "ShadButton.h"

#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetricsF>
#include <QEasingCurve>
#include <QIcon>
#include <QKeyEvent>
#include <QLineF>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QVariant>
#include <QVariantAnimation>
#include <QtMath>

#include "ShadButtonPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // 对应 underline-offset-4
    constexpr qreal kUnderlineOffset = 4.0;
} // namespace

ShadButton::ShadButton(QWidget* parent)
    : QPushButton(parent), d_ptr(new ShadButtonPrivate())
{
    Q_D(ShadButton);
    d->q_ptr = this;
    d->_hoverTransition = new ShadTransition(this);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);
#ifdef Q_OS_MACOS
    // QPushButton 默认开启该属性，macOS 样式会在获得焦点时额外绘制系统焦点框；焦点环由组件自绘
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    // QPushButton 会从样式读取布局项边距（macOS 上不为 0），布局按缩小后的尺寸排列、再按放大后的矩形设置几何，
    // 会超出布局分配的区域被父组件裁剪；组件自绘且 sizeHint 已包含焦点环边距，直接使用控件矩形
    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadButton::ShadButton(const QString& text, QWidget* parent)
    : ShadButton(parent)
{
    setText(text);
}

ShadButton::ShadButton(const QString& text, ShadButtonType::Variant variant, QWidget* parent)
    : ShadButton(parent)
{
    setText(text);
    setVariant(variant);
}

ShadButton::~ShadButton()
{
}

void ShadButton::setVariant(ShadButtonType::Variant variant)
{
    Q_D(ShadButton);
    if (d->_variant == variant)
    {
        return;
    }
    d->_variant = variant;
    update();
    Q_EMIT variantChanged();
}

ShadButtonType::Variant ShadButton::getVariant() const
{
    Q_D(const ShadButton);
    return d->_variant;
}

void ShadButton::setButtonSize(ShadButtonType::Size size)
{
    Q_D(ShadButton);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    updateGeometry();
    update();
    Q_EMIT buttonSizeChanged();
}

ShadButtonType::Size ShadButton::getButtonSize() const
{
    Q_D(const ShadButton);
    return d->_size;
}

void ShadButton::setShadIcon(ShadIconType::IconName icon)
{
    Q_D(ShadButton);
    if (d->_shadIcon == icon)
    {
        return;
    }
    d->_shadIcon = icon;
    updateGeometry();
    update();
}

ShadIconType::IconName ShadButton::getShadIcon() const
{
    Q_D(const ShadButton);
    return d->_shadIcon;
}

void ShadButton::setContentAlignment(Qt::Alignment alignment)
{
    Q_D(ShadButton);
    d->_contentAlignment = alignment;
    update();
}

Qt::Alignment ShadButton::getContentAlignment() const
{
    Q_D(const ShadButton);
    return d->_contentAlignment;
}

void ShadButton::setLoading(bool loading)
{
    Q_D(ShadButton);
    if (d->_isLoading == loading)
    {
        return;
    }
    d->_isLoading = loading;
    if (loading)
    {
        if (d->_spinAnimation == nullptr)
        {
            d->_spinAnimation = new QVariantAnimation(this);
            d->_spinAnimation->setStartValue(0.0);
            d->_spinAnimation->setEndValue(360.0);
            d->_spinAnimation->setDuration(ShadStyleHelper::kSpinDurationMs);
            d->_spinAnimation->setLoopCount(-1);
            d->_spinAnimation->setEasingCurve(QEasingCurve::Linear);
            connect(d->_spinAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
            {
                d->_spinAngle = value.toReal();
                update();
            });
        }
        d->_spinAnimation->start();
    }
    else if (d->_spinAnimation != nullptr)
    {
        d->_spinAnimation->stop();
        d->_spinAngle = 0.0;
    }
    updateGeometry();
    update();
    Q_EMIT loadingChanged();
}

bool ShadButton::isLoading() const
{
    Q_D(const ShadButton);
    return d->_isLoading;
}

void ShadButton::setRoundedFull(bool roundedFull)
{
    Q_D(ShadButton);
    if (d->_isRoundedFull == roundedFull)
    {
        return;
    }
    d->_isRoundedFull = roundedFull;
    update();
}

bool ShadButton::isRoundedFull() const
{
    Q_D(const ShadButton);
    return d->_isRoundedFull;
}

QSize ShadButton::sizeHint() const
{
    Q_D(const ShadButton);
    const ShadButtonPrivate::SizeMetrics metrics = ShadButtonPrivate::sizeMetrics(d->_size);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    if (metrics.iconOnly)
    {
        return QSize(metrics.height + margin, metrics.height + margin);
    }
    const bool hasIcon = d->hasIcon();
    const int contentWidth = qCeil(ShadStyleHelper::iconTextWidth(d->contentFont(), text(), hasIcon, metrics.gap, metrics.iconSize));
    const int padding = hasIcon ? metrics.paddingXWithIcon : metrics.paddingX;
    return QSize(contentWidth + 2 * padding + margin, metrics.height + margin);
}

QSize ShadButton::minimumSizeHint() const
{
    Q_D(const ShadButton);
    const ShadButtonPrivate::SizeMetrics metrics = ShadButtonPrivate::sizeMetrics(d->_size);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    if (metrics.iconOnly)
    {
        return QSize(metrics.height + margin, metrics.height + margin);
    }
    const bool hasIcon = d->hasIcon();
    const int padding = hasIcon ? metrics.paddingXWithIcon : metrics.paddingX;
    return QSize(2 * padding + (hasIcon ? metrics.iconSize : 0) + margin, metrics.height + margin);
}

bool ShadButton::event(QEvent* event)
{
    Q_D(ShadButton);
    // 加载中忽略助记符等快捷键触发的点击
    if (d->_isLoading && event->type() == QEvent::Shortcut)
    {
        event->accept();
        return true;
    }
    return QPushButton::event(event);
}

bool ShadButton::hitButton(const QPoint& pos) const
{
    Q_D(const ShadButton);
    if (d->_isLoading)
    {
        return false;
    }
    return QPushButton::hitButton(pos);
}

void ShadButton::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadButton);
    if (d->_isLoading && ShadButtonPrivate::isActivationKey(event->key()))
    {
        event->accept();
        return;
    }
    QPushButton::keyPressEvent(event);
}

void ShadButton::keyReleaseEvent(QKeyEvent* event)
{
    Q_D(ShadButton);
    if (d->_isLoading && ShadButtonPrivate::isActivationKey(event->key()))
    {
        event->accept();
        return;
    }
    QPushButton::keyReleaseEvent(event);
}

void ShadButton::enterEvent(QEnterEvent* event)
{
    Q_D(ShadButton);
    if (isEnabled())
    {
        d->_hoverTransition->animateTo(1.0);
    }
    QPushButton::enterEvent(event);
}

void ShadButton::leaveEvent(QEvent* event)
{
    Q_D(ShadButton);
    d->_hoverTransition->animateTo(0.0);
    QPushButton::leaveEvent(event);
}

void ShadButton::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadButton);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QPushButton::focusInEvent(event);
    update();
}

void ShadButton::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadButton);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QPushButton::focusOutEvent(event);
    update();
}

void ShadButton::changeEvent(QEvent* event)
{
    Q_D(ShadButton);
    if (event->type() == QEvent::EnabledChange)
    {
        d->_hoverTransition->jumpTo(isEnabled() && underMouse() ? 1.0 : 0.0);
    }
    else if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QPushButton::changeEvent(event);
}

void ShadButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadButton);
    using namespace ShadStyleHelper;
    const ShadButtonPrivate::VariantColors colors = ShadButtonPrivate::variantColors(d->_variant);
    const ShadButtonPrivate::SizeMetrics metrics = ShadButtonPrivate::sizeMetrics(d->_size);
    const qreal progress = d->_hoverTransition->value();
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = d->_isRoundedFull ? bodyRect.height() / 2.0 : shadTheme->getRadius(ShadThemeType::Medium);
    const bool showFocusRing = hasFocus() && d->_isFocusVisible;
    // 放入 ShadButtonGroup 时与相邻组件拼接的边为直角，起始侧不描边
    const GroupEdges edges = groupEdges(this);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    // 背景、边框与焦点环
    if (colors.hasShadow)
    {
        drawShadowXs(&painter, bodyRect, radius, edges);
    }
    fillRoundedRect(&painter, bodyRect, radius, mix(colors.background, colors.hoverBackground, progress), edges);
    QColor borderColor = colors.border;
    if (showFocusRing && borderColor.alpha() > 0)
    {
        borderColor = shadTheme->getColor(ShadThemeType::Ring);
    }
    strokeRoundedRect(&painter, bodyRect, radius, borderColor, edges);
    if (showFocusRing)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(d->_variant == ShadButtonType::Destructive), edges);
    }

    // 图标与文字
    const QColor textColor = mix(colors.text, colors.hoverText, progress);
    const int padding = metrics.iconOnly ? 0 : (d->hasIcon() ? metrics.paddingXWithIcon : metrics.paddingX);
    const QRectF contentRect = bodyRect.adjusted(padding, 0, -padding, 0);
    const Qt::Alignment alignment = metrics.iconOnly ? Qt::AlignHCenter : d->_contentAlignment;
    // 加载中不绘制 QIcon 回退图标，只绘制旋转的加载图标
    const QIcon fallbackIcon = d->_isLoading ? QIcon() : icon();
    const QRectF textRect = drawIconText(&painter, contentRect, d->displayIcon(), fallbackIcon, text(), d->contentFont(), textColor, metrics.gap, alignment, metrics.iconSize, d->_spinAngle, layoutDirection());
    if (colors.underlineOnHover && !textRect.isNull() && isEnabled() && underMouse())
    {
        const QFontMetricsF fontMetrics(d->contentFont());
        const qreal baseline = textRect.center().y() + (fontMetrics.ascent() - fontMetrics.descent()) / 2.0;
        const qreal underlineY = qRound(baseline + kUnderlineOffset) - 0.5;
        painter.setPen(QPen(textColor, 1.0));
        painter.drawLine(QLineF(textRect.left(), underlineY, textRect.right(), underlineY));
    }
}
