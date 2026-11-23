#include "ShadCarouselThumbnails.h"

#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QVariant>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtMath>

#include <initializer_list>

#include "ShadButton.h"
#include "ShadCarousel.h"
#include "ShadCarouselThumbnailsPrivate.h"
#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadCarouselThumbnails::ShadCarouselThumbnails(ShadCarousel* carousel, QWidget* parent)
    : QWidget(parent), d_ptr(new ShadCarouselThumbnailsPrivate())
{
    Q_D(ShadCarouselThumbnails);
    d->q_ptr = this;
    d->_prevButton = new ShadButton(this);
    d->_nextButton = new ShadButton(this);
    for (ShadButton* button : {d->_prevButton, d->_nextButton})
    {
        button->setVariant(ShadButtonType::Outline);
        button->setButtonSize(ShadButtonType::IconSmall);
        button->setRoundedFull(true);
        button->hide();
    }
    d->_prevButton->setAccessibleName(tr("上一张"));
    d->_nextButton->setAccessibleName(tr("下一张"));
    connect(d->_prevButton, &ShadButton::clicked, this, [d]()
    {
        if (!d->_carousel.isNull())
        {
            d->_carousel->scrollPrev();
        }
    });
    connect(d->_nextButton, &ShadButton::clicked, this, [d]()
    {
        if (!d->_carousel.isNull())
        {
            d->_carousel->scrollNext();
        }
    });
    d->_scrollAnimation = new QVariantAnimation(this);
    d->_scrollAnimation->setDuration(ShadCarouselThumbnailsPrivate::kScrollDurationMs);
    d->_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_scrollAnimation, &QVariantAnimation::valueChanged, this, [d](const QVariant& value) { d->setScroll(value.toReal()); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setFocusPolicy(Qt::TabFocus);
    setMouseTracking(true);
    setCarousel(carousel);
}

ShadCarouselThumbnails::~ShadCarouselThumbnails()
{
    Q_D(ShadCarouselThumbnails);
    d->_scrollAnimation->stop();
    QObject::disconnect(d->_scrollAnimation, nullptr, this, nullptr);
    QObject::disconnect(d->_prevButton, nullptr, this, nullptr);
    QObject::disconnect(d->_nextButton, nullptr, this, nullptr);
    d->unbind();
}

void ShadCarouselThumbnails::setCarousel(ShadCarousel* carousel)
{
    Q_D(ShadCarouselThumbnails);
    if (d->_carousel == carousel && (carousel != nullptr || d->_connections.isEmpty()))
    {
        d->updateButtons();
        return;
    }
    d->unbind();
    d->bind(carousel);
    d->refresh();
}

ShadCarousel* ShadCarouselThumbnails::getCarousel() const
{
    Q_D(const ShadCarouselThumbnails);
    return d->_carousel.data();
}

int ShadCarouselThumbnails::addThumbnail(const QPixmap& pixmap)
{
    Q_D(ShadCarouselThumbnails);
    d->_pixmaps.append(pixmap);
    d->refresh();
    return int(d->_pixmaps.size()) - 1;
}

void ShadCarouselThumbnails::setThumbnail(int index, const QPixmap& pixmap)
{
    Q_D(ShadCarouselThumbnails);
    if (index < 0 || index >= d->_pixmaps.size())
    {
        return;
    }
    d->_pixmaps[index] = pixmap;
    update();
}

QPixmap ShadCarouselThumbnails::getThumbnail(int index) const
{
    Q_D(const ShadCarouselThumbnails);
    return d->_pixmaps.value(index);
}

void ShadCarouselThumbnails::removeThumbnail(int index)
{
    Q_D(ShadCarouselThumbnails);
    if (index < 0 || index >= d->_pixmaps.size())
    {
        return;
    }
    d->_pixmaps.removeAt(index);
    d->refresh();
}

void ShadCarouselThumbnails::clear()
{
    Q_D(ShadCarouselThumbnails);
    if (d->_pixmaps.isEmpty())
    {
        return;
    }
    d->_pixmaps.clear();
    d->refresh();
}

int ShadCarouselThumbnails::count() const
{
    Q_D(const ShadCarouselThumbnails);
    return int(d->_pixmaps.size());
}

void ShadCarouselThumbnails::setThumbnailSize(int size)
{
    Q_D(ShadCarouselThumbnails);
    const int thumbnailSize = qMax(16, size);
    if (d->_thumbnailSize == thumbnailSize)
    {
        return;
    }
    d->_thumbnailSize = thumbnailSize;
    d->refresh();
}

int ShadCarouselThumbnails::getThumbnailSize() const
{
    Q_D(const ShadCarouselThumbnails);
    return d->_thumbnailSize;
}

void ShadCarouselThumbnails::setSpacing(int spacing)
{
    Q_D(ShadCarouselThumbnails);
    const int thumbnailSpacing = qMax(0, spacing);
    if (d->_spacing == thumbnailSpacing)
    {
        return;
    }
    d->_spacing = thumbnailSpacing;
    d->refresh();
}

int ShadCarouselThumbnails::getSpacing() const
{
    Q_D(const ShadCarouselThumbnails);
    return d->_spacing;
}

void ShadCarouselThumbnails::setNavigationVisible(bool visible)
{
    Q_D(ShadCarouselThumbnails);
    if (d->_isNavigationVisible == visible)
    {
        return;
    }
    d->_isNavigationVisible = visible;
    d->refresh();
}

bool ShadCarouselThumbnails::isNavigationVisible() const
{
    Q_D(const ShadCarouselThumbnails);
    return d->_isNavigationVisible;
}

QSize ShadCarouselThumbnails::sizeHint() const
{
    Q_D(const ShadCarouselThumbnails);
    const int contentWidth = qMax(d->_thumbnailSize, qCeil(d->contentWidth()));
    const int height = qMax(d->_thumbnailSize + 2 * ShadCarouselThumbnailsPrivate::kPadding, d->_isNavigationVisible ? d->_prevButton->sizeHint().height() : 0);
    return QSize(contentWidth + 2 * ShadCarouselThumbnailsPrivate::kPadding + 2 * d->navigationExtent(), height);
}

QSize ShadCarouselThumbnails::minimumSizeHint() const
{
    Q_D(const ShadCarouselThumbnails);
    return QSize(d->_thumbnailSize + 2 * ShadCarouselThumbnailsPrivate::kPadding + 2 * d->navigationExtent(), sizeHint().height());
}

void ShadCarouselThumbnails::changeEvent(QEvent* event)
{
    Q_D(ShadCarouselThumbnails);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->layoutButtons();
        d->updateButtons();
        update();
    }
    else if (event->type() == QEvent::EnabledChange)
    {
        update();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        d->_prevButton->setAccessibleName(tr("上一张"));
        d->_nextButton->setAccessibleName(tr("下一张"));
    }
    QWidget::changeEvent(event);
}

void ShadCarouselThumbnails::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadCarouselThumbnails);
    QWidget::resizeEvent(event);
    d->layoutButtons();
    d->ensureCurrentVisible(false);
}

void ShadCarouselThumbnails::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadCarouselThumbnails);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    update();
    QWidget::focusInEvent(event);
}

void ShadCarouselThumbnails::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadCarouselThumbnails);
    d->_isFocusVisible = false;
    update();
    QWidget::focusOutEvent(event);
}

void ShadCarouselThumbnails::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadCarouselThumbnails);
    const int count = int(d->_pixmaps.size());
    if (d->_carousel.isNull() || count == 0)
    {
        QWidget::keyPressEvent(event);
        return;
    }
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    const int previousKey = isRightToLeft ? Qt::Key_Right : Qt::Key_Left;
    const int nextKey = isRightToLeft ? Qt::Key_Left : Qt::Key_Right;
    const int current = d->currentIndex();
    int target = current;
    if (event->key() == previousKey)
    {
        target = qMax(0, current - 1);
    }
    else if (event->key() == nextKey)
    {
        target = qMin(count - 1, current + 1);
    }
    else if (event->key() == Qt::Key_Home)
    {
        target = 0;
    }
    else if (event->key() == Qt::Key_End)
    {
        target = count - 1;
    }
    else
    {
        QWidget::keyPressEvent(event);
        return;
    }
    if (target != current)
    {
        d->_carousel->scrollTo(target);
    }
    event->accept();
}

void ShadCarouselThumbnails::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadCarouselThumbnails);
    d->setHoveredIndex(d->thumbnailIndexAt(event->position()));
    QWidget::mouseMoveEvent(event);
}

void ShadCarouselThumbnails::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadCarouselThumbnails);
    const int index = event->button() == Qt::LeftButton ? d->thumbnailIndexAt(event->position()) : -1;
    if (index < 0)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    d->_pressedIndex = index;
    update();
    event->accept();
}

void ShadCarouselThumbnails::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadCarouselThumbnails);
    if (event->button() != Qt::LeftButton || d->_pressedIndex < 0)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const int index = d->_pressedIndex;
    d->_pressedIndex = -1;
    update();
    if (d->thumbnailIndexAt(event->position()) == index && !d->_carousel.isNull())
    {
        d->_carousel->scrollTo(index);
    }
    event->accept();
}

void ShadCarouselThumbnails::leaveEvent(QEvent* event)
{
    Q_D(ShadCarouselThumbnails);
    d->setHoveredIndex(-1);
    QWidget::leaveEvent(event);
}

void ShadCarouselThumbnails::wheelEvent(QWheelEvent* event)
{
    Q_D(ShadCarouselThumbnails);
    if (d->maxScroll() <= 0.0)
    {
        event->ignore();
        return;
    }
    const QPoint delta = event->angleDelta();
    const int step = qAbs(delta.x()) > qAbs(delta.y()) ? delta.x() : delta.y();
    d->_scrollAnimation->stop();
    d->setScroll(d->_scroll - step / 2.0);
    event->accept();
}

void ShadCarouselThumbnails::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadCarouselThumbnails);
    using namespace ShadStyleHelper;
    const int count = int(d->_pixmaps.size());
    if (count == 0)
    {
        return;
    }
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    painter.setClipRect(visualRect(layoutDirection(), QRectF(rect()), d->stripRect()));
    const qreal radius = shadTheme->getRadius(ShadThemeType::ExtraLarge);
    const qreal ringOffset = ShadCarouselThumbnailsPrivate::kPadding;
    const int current = d->currentIndex();
    const qreal baseOpacity = isEnabled() ? 1.0 : kDisabledOpacity;
    for (int index = 0; index < count; ++index)
    {
        const QRectF thumbnailRect = d->thumbnailRect(index);
        const QPixmap& pixmap = d->_pixmaps.at(index);
        painter.save();
        if (index == d->_pressedIndex)
        {
            const QPointF center = thumbnailRect.center();
            painter.translate(center);
            painter.scale(ShadCarouselThumbnailsPrivate::kPressedScale, ShadCarouselThumbnailsPrivate::kPressedScale);
            painter.translate(-center);
        }
        painter.setOpacity(baseOpacity * (index == d->_hoveredIndex ? ShadCarouselThumbnailsPrivate::kHoverOpacity : 1.0));
        painter.setClipPath(roundedRectPath(thumbnailRect, radius), Qt::IntersectClip);
        if (pixmap.isNull())
        {
            painter.fillRect(thumbnailRect, shadTheme->getColor(ShadThemeType::Muted));
            ShadIcon::paint(&painter, thumbnailRect, ShadIconType::Image, shadTheme->getColor(ShadThemeType::MutedForeground), ShadCarouselThumbnailsPrivate::kPlaceholderIconSize);
        }
        else
        {
            const qreal side = qMin(pixmap.width(), pixmap.height());
            painter.drawPixmap(thumbnailRect, pixmap, QRectF((pixmap.width() - side) / 2.0, (pixmap.height() - side) / 2.0, side, side));
        }
        painter.restore();
        if (index != current)
        {
            continue;
        }
        const QRectF ringRect = thumbnailRect.adjusted(-ringOffset, -ringOffset, ringOffset, ringOffset);
        painter.setOpacity(baseOpacity);
        if (hasFocus() && d->_isFocusVisible)
        {
            strokeRoundedRect(&painter, ringRect, radius + ringOffset, focusRingColor(false), ShadCarouselThumbnailsPrivate::kPadding);
        }
        strokeRoundedRect(&painter, ringRect, radius + ringOffset, shadTheme->getColor(ShadThemeType::Primary), ShadCarouselThumbnailsPrivate::kRingWidth);
    }
}
