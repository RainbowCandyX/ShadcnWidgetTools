#include "ShadMarker.h"

#include <QColor>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QVariant>
#include <QVariantAnimation>
#include <QtGlobal>

#include "ShadIcon.h"
#include "ShadMarkerPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    void paintIcon(QPainter* painter, const QRectF& rect, ShadIconType::IconName icon, const QColor& color, qreal rotation)
    {
        painter->save();
        const QPointF center = rect.center();
        painter->translate(center);
        painter->rotate(rotation);
        painter->translate(-center);
        ShadIcon::paint(painter, rect, icon, color, ShadMarkerPrivate::kIconSize);
        painter->restore();
    }
}

ShadMarker::ShadMarker(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadMarkerPrivate())
{
    Q_D(ShadMarker);
    d->q_ptr = this;
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    setFocusPolicy(Qt::NoFocus);
    d->_hoverTransition = new ShadTransition(this);

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

    d->_shimmerAnimation = new QVariantAnimation(this);
    d->_shimmerAnimation->setStartValue(0.0);
    d->_shimmerAnimation->setEndValue(1.0);
    d->_shimmerAnimation->setDuration(ShadStyleHelper::kShimmerDurationMs);
    d->_shimmerAnimation->setLoopCount(-1);
    d->_shimmerAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(d->_shimmerAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_shimmerProgress = value.toReal();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadMarker::ShadMarker(const QString& text, QWidget* parent)
    : ShadMarker(parent)
{
    setText(text);
}

ShadMarker::ShadMarker(ShadIconType::IconName icon, const QString& text, QWidget* parent)
    : ShadMarker(parent)
{
    setIcon(icon);
    setText(text);
}

ShadMarker::~ShadMarker()
{
}

void ShadMarker::setVariant(ShadMarkerType::Variant variant)
{
    Q_D(ShadMarker);
    if (d->_variant == variant)
    {
        return;
    }
    d->_variant = variant;
    d->placeIconWidget();
    updateGeometry();
    update();
}

ShadMarkerType::Variant ShadMarker::getVariant() const
{
    Q_D(const ShadMarker);
    return d->_variant;
}

void ShadMarker::setText(const QString& text)
{
    Q_D(ShadMarker);
    if (d->_text == text)
    {
        return;
    }
    d->_text = text;
    setAccessibleName(text);
    d->updateAnimations();
    d->placeIconWidget();
    updateGeometry();
    update();
}

QString ShadMarker::getText() const
{
    Q_D(const ShadMarker);
    return d->_text;
}

void ShadMarker::setIcon(ShadIconType::IconName icon)
{
    Q_D(ShadMarker);
    if (d->_icon == icon)
    {
        return;
    }
    d->_icon = icon;
    d->updateAnimations();
    d->placeIconWidget();
    updateGeometry();
    update();
}

ShadIconType::IconName ShadMarker::getIcon() const
{
    Q_D(const ShadMarker);
    return d->_icon;
}

void ShadMarker::setIconWidget(QWidget* widget)
{
    Q_D(ShadMarker);
    if (d->_iconWidget == widget)
    {
        return;
    }
    if (d->_iconWidget != nullptr)
    {
        d->_iconWidget->hide();
        d->_iconWidget->deleteLater();
    }
    d->_iconWidget = widget;
    if (widget != nullptr)
    {
        widget->setParent(this);
        widget->setAttribute(Qt::WA_TransparentForMouseEvents);
        d->placeIconWidget();
        widget->show();
    }
    d->updateAnimations();
    updateGeometry();
    update();
}

QWidget* ShadMarker::getIconWidget() const
{
    Q_D(const ShadMarker);
    return d->_iconWidget;
}

void ShadMarker::setShimmer(bool shimmer)
{
    Q_D(ShadMarker);
    if (d->_isShimmer == shimmer)
    {
        return;
    }
    d->_isShimmer = shimmer;
    d->updateAnimations();
    update();
}

bool ShadMarker::isShimmer() const
{
    Q_D(const ShadMarker);
    return d->_isShimmer;
}

void ShadMarker::setClickable(bool clickable)
{
    Q_D(ShadMarker);
    if (d->_isClickable == clickable)
    {
        return;
    }
    d->_isClickable = clickable;
    setFocusPolicy(clickable ? Qt::StrongFocus : Qt::NoFocus);
    if (clickable)
    {
        setCursor(Qt::PointingHandCursor);
        d->_hoverTransition->jumpTo(underMouse() ? 1.0 : 0.0);
    }
    else
    {
        unsetCursor();
        d->_isPressed = false;
        d->_hoverTransition->jumpTo(0.0);
    }
    update();
}

bool ShadMarker::isClickable() const
{
    Q_D(const ShadMarker);
    return d->_isClickable;
}

void ShadMarker::setUrl(const QUrl& url)
{
    Q_D(ShadMarker);
    if (d->_url == url)
    {
        return;
    }
    d->_url = url;
    if (!url.isEmpty())
    {
        setClickable(true);
    }
    updateGeometry();
    update();
}

QUrl ShadMarker::getUrl() const
{
    Q_D(const ShadMarker);
    return d->_url;
}

bool ShadMarker::hasHeightForWidth() const
{
    return true;
}

int ShadMarker::heightForWidth(int width) const
{
    Q_D(const ShadMarker);
    return d->computeGeometry(width).height;
}

QSize ShadMarker::sizeHint() const
{
    Q_D(const ShadMarker);
    const int width = d->naturalWidth();
    return QSize(width, heightForWidth(width));
}

QSize ShadMarker::minimumSizeHint() const
{
    Q_D(const ShadMarker);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const int iconWidth = d->hasIcon() ? ShadMarkerPrivate::kIconSize : 0;
    return QSize(iconWidth + margin, ShadMarkerPrivate::kIconSize + margin);
}

void ShadMarker::enterEvent(QEnterEvent* event)
{
    Q_D(ShadMarker);
    if (d->_isClickable)
    {
        d->_hoverTransition->animateTo(1.0);
    }
    QWidget::enterEvent(event);
}

void ShadMarker::leaveEvent(QEvent* event)
{
    Q_D(ShadMarker);
    d->_hoverTransition->animateTo(0.0);
    QWidget::leaveEvent(event);
}

void ShadMarker::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadMarker);
    if (!d->_isClickable || event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    d->_isPressed = true;
    event->accept();
}

void ShadMarker::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadMarker);
    if (!d->_isPressed || event->button() != Qt::LeftButton)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    d->_isPressed = false;
    event->accept();
    if (rect().contains(event->position().toPoint()))
    {
        d->activate();
    }
}

void ShadMarker::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadMarker);
    const int key = event->key();
    if (d->_isClickable && event->modifiers() == Qt::NoModifier && (key == Qt::Key_Space || key == Qt::Key_Return || key == Qt::Key_Enter))
    {
        event->accept();
        d->activate();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadMarker::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadMarker);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    update();
    QWidget::focusInEvent(event);
}

void ShadMarker::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadMarker);
    if (event->reason() != Qt::PopupFocusReason && event->reason() != Qt::ActiveWindowFocusReason)
    {
        d->_isFocusVisible = false;
    }
    update();
    QWidget::focusOutEvent(event);
}

void ShadMarker::showEvent(QShowEvent* event)
{
    Q_D(ShadMarker);
    QWidget::showEvent(event);
    d->updateAnimations();
}

void ShadMarker::hideEvent(QHideEvent* event)
{
    Q_D(ShadMarker);
    QWidget::hideEvent(event);
    d->updateAnimations();
}

void ShadMarker::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadMarker);
    QWidget::resizeEvent(event);
    d->placeIconWidget();
}

void ShadMarker::changeEvent(QEvent* event)
{
    Q_D(ShadMarker);
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->placeIconWidget();
    }
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadMarker::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadMarker);
    using namespace ShadStyleHelper;
    const Qt::LayoutDirection direction = layoutDirection();
    const QRectF bounds(rect());
    const ShadMarkerPrivate::Layout layout = d->computeGeometry(width());
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    if (hasFocus() && d->_isFocusVisible)
    {
        drawFocusRing(&painter, insetRect(rect(), kFocusRingWidth), shadTheme->getRadius(ShadThemeType::Medium), focusRingColor(false));
    }
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    const QColor borderColor = shadTheme->getColor(ShadThemeType::Border);
    if (!layout.leadingRule.isEmpty())
    {
        painter.fillRect(visualRect(direction, bounds, layout.leadingRule), borderColor);
    }
    if (!layout.trailingRule.isEmpty())
    {
        painter.fillRect(visualRect(direction, bounds, layout.trailingRule), borderColor);
    }
    if (!layout.border.isEmpty())
    {
        painter.fillRect(visualRect(direction, bounds, layout.border), borderColor);
    }
    const QColor color = d->textColor();
    if (d->_iconWidget == nullptr && d->_icon != ShadIconType::None && !layout.icon.isEmpty())
    {
        paintIcon(&painter, visualRect(direction, bounds, layout.icon), d->_icon, color, d->_spinAngle);
    }
    const QFont font = d->textFont();
    const Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignVCenter;
    for (int index = 0; index < layout.lines.size(); ++index)
    {
        const QRectF lineRect = visualRect(direction, bounds, layout.lineRects.at(index));
        if (d->_isShimmer)
        {
            drawShimmerText(&painter, lineRect, layout.lines.at(index), font, color, d->_shimmerProgress, alignment, direction);
        }
        else
        {
            painter.setFont(font);
            painter.setPen(color);
            painter.drawText(lineRect, visualAlignment(direction, alignment), layout.lines.at(index));
        }
    }
}
