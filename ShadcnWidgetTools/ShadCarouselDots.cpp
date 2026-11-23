#include "ShadCarouselDots.h"

#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtMath>

#include "ShadCarousel.h"
#include "ShadCarouselDotsPrivate.h"
#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadCarouselDots::ShadCarouselDots(ShadCarousel* carousel, QWidget* parent)
    : QWidget(parent), d_ptr(new ShadCarouselDotsPrivate())
{
    Q_D(ShadCarouselDots);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setFocusPolicy(Qt::TabFocus);
    setMouseTracking(true);
    d->_animation = new QVariantAnimation(this);
    d->_animation->setDuration(ShadCarouselDotsPrivate::kAnimationDurationMs);
    d->_animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_animation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_position = value.toReal();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    setCarousel(carousel);
}

ShadCarouselDots::~ShadCarouselDots()
{
    Q_D(ShadCarouselDots);
    d->_animation->stop();
    QObject::disconnect(d->_animation, nullptr, this, nullptr);
    d->unbind();
}

void ShadCarouselDots::setCarousel(ShadCarousel* carousel)
{
    Q_D(ShadCarouselDots);
    if (d->_carousel == carousel && (carousel != nullptr || d->_connections.isEmpty()))
    {
        return;
    }
    d->unbind();
    d->bind(carousel);
    d->syncPosition(false);
    updateGeometry();
    update();
}

ShadCarousel* ShadCarouselDots::getCarousel() const
{
    Q_D(const ShadCarouselDots);
    return d->_carousel.data();
}

void ShadCarouselDots::setDotSize(int size)
{
    Q_D(ShadCarouselDots);
    const int dotSize = qMax(2, size);
    if (d->_dotSize == dotSize)
    {
        return;
    }
    d->_dotSize = dotSize;
    updateGeometry();
    update();
}

int ShadCarouselDots::getDotSize() const
{
    Q_D(const ShadCarouselDots);
    return d->_dotSize;
}

void ShadCarouselDots::setSpacing(int spacing)
{
    Q_D(ShadCarouselDots);
    const int dotSpacing = qMax(0, spacing);
    if (d->_spacing == dotSpacing)
    {
        return;
    }
    d->_spacing = dotSpacing;
    updateGeometry();
    update();
}

int ShadCarouselDots::getSpacing() const
{
    Q_D(const ShadCarouselDots);
    return d->_spacing;
}

QSize ShadCarouselDots::sizeHint() const
{
    Q_D(const ShadCarouselDots);
    return QSize(qCeil(d->contentWidth(d->dotCount())) + 2 * ShadCarouselDotsPrivate::kPadding, d->_dotSize + 2 * ShadCarouselDotsPrivate::kPadding);
}

QSize ShadCarouselDots::minimumSizeHint() const
{
    return sizeHint();
}

void ShadCarouselDots::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadCarouselDots::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadCarouselDots);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    update();
    QWidget::focusInEvent(event);
}

void ShadCarouselDots::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadCarouselDots);
    d->_isFocusVisible = false;
    update();
    QWidget::focusOutEvent(event);
}

void ShadCarouselDots::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadCarouselDots);
    if (d->_carousel.isNull() || d->dotCount() <= 1)
    {
        QWidget::keyPressEvent(event);
        return;
    }
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    const int previousKey = isRightToLeft ? Qt::Key_Right : Qt::Key_Left;
    const int nextKey = isRightToLeft ? Qt::Key_Left : Qt::Key_Right;
    if (event->key() == previousKey)
    {
        d->_carousel->scrollPrev();
    }
    else if (event->key() == nextKey)
    {
        d->_carousel->scrollNext();
    }
    else if (event->key() == Qt::Key_Home)
    {
        d->_carousel->scrollTo(0);
    }
    else if (event->key() == Qt::Key_End)
    {
        d->_carousel->scrollTo(d->dotCount() - 1);
    }
    else
    {
        QWidget::keyPressEvent(event);
        return;
    }
    event->accept();
}

void ShadCarouselDots::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadCarouselDots);
    d->setHoveredIndex(d->dotIndexAt(event->position()));
    QWidget::mouseMoveEvent(event);
}

void ShadCarouselDots::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadCarouselDots);
    const int index = event->button() == Qt::LeftButton ? d->dotIndexAt(event->position()) : -1;
    if (index < 0 || d->_carousel.isNull())
    {
        QWidget::mousePressEvent(event);
        return;
    }
    d->_carousel->scrollTo(index);
    event->accept();
}

void ShadCarouselDots::leaveEvent(QEvent* event)
{
    Q_D(ShadCarouselDots);
    d->setHoveredIndex(-1);
    QWidget::leaveEvent(event);
}

void ShadCarouselDots::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadCarouselDots);
    using namespace ShadStyleHelper;
    const int count = d->dotCount();
    if (count <= 1)
    {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    const QColor inactiveColor = shadTheme->getColor(ShadThemeType::Border);
    const QColor hoveredColor = withAlpha(shadTheme->getColor(ShadThemeType::MutedForeground), 0.5);
    const QColor activeColor = shadTheme->getColor(ShadThemeType::Primary);
    const qreal radius = d->_dotSize / 2.0;
    for (int index = 0; index < count; ++index)
    {
        const QColor baseColor = index == d->_hoveredIndex ? hoveredColor : inactiveColor;
        fillRoundedRect(&painter, d->dotRect(index), radius, mix(baseColor, activeColor, d->weight(index)));
    }
    if (hasFocus() && d->_isFocusVisible)
    {
        const QRectF ringRect = d->contentRect().adjusted(-3.0, -3.0, 3.0, 3.0);
        drawFocusRing(&painter, ringRect, ringRect.height() / 2.0, focusRingColor(false));
    }
}
