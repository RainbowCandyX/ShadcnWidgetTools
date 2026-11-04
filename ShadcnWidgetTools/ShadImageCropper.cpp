#include "ShadImageCropper.h"

#include <QBrush>
#include <QColor>
#include <QCursor>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QSizePolicy>
#include <QTransform>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadImageCropperPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kEmptyIconSize = 32;
    constexpr int kEmptyGap = 8;
    constexpr int kEmptyFontSize = 14;
    constexpr int kEdgeHandleLength = 20;
    constexpr int kEdgeHandleMinimum = 64;

    Qt::CursorShape cursorFor(ShadImageCropperPrivate::Handle handle)
    {
        switch (handle)
        {
            case ShadImageCropperPrivate::Move:
                return Qt::SizeAllCursor;
            case ShadImageCropperPrivate::TopLeft:
            case ShadImageCropperPrivate::BottomRight:
                return Qt::SizeFDiagCursor;
            case ShadImageCropperPrivate::TopRight:
            case ShadImageCropperPrivate::BottomLeft:
                return Qt::SizeBDiagCursor;
            case ShadImageCropperPrivate::Top:
            case ShadImageCropperPrivate::Bottom:
                return Qt::SizeVerCursor;
            case ShadImageCropperPrivate::Left:
            case ShadImageCropperPrivate::Right:
                return Qt::SizeHorCursor;
            default:
                return Qt::ArrowCursor;
        }
    }

    void updateCursor(QWidget* widget, ShadImageCropperPrivate::Handle handle)
    {
        if (handle == ShadImageCropperPrivate::None)
        {
            widget->unsetCursor();
            return;
        }
        widget->setCursor(cursorFor(handle));
    }

    void drawHandles(QPainter* painter, const QRectF& crop)
    {
        const qreal thickness = ShadImageCropperPrivate::kHandleThickness;
        const qreal half = thickness / 2.0;
        const qreal length = qMin<qreal>(ShadImageCropperPrivate::kHandleLength, qMin(crop.width(), crop.height()) / 3.0);
        QPen pen(Qt::white, thickness);
        pen.setCapStyle(Qt::FlatCap);
        pen.setJoinStyle(Qt::MiterJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        const qreal left = crop.left() + half;
        const qreal right = crop.right() - half;
        const qreal top = crop.top() + half;
        const qreal bottom = crop.bottom() - half;
        const QPointF topLeft[] = {QPointF(left, top + length), QPointF(left, top), QPointF(left + length, top)};
        const QPointF topRight[] = {QPointF(right - length, top), QPointF(right, top), QPointF(right, top + length)};
        const QPointF bottomRight[] = {QPointF(right, bottom - length), QPointF(right, bottom), QPointF(right - length, bottom)};
        const QPointF bottomLeft[] = {QPointF(left + length, bottom), QPointF(left, bottom), QPointF(left, bottom - length)};
        painter->drawPolyline(topLeft, 3);
        painter->drawPolyline(topRight, 3);
        painter->drawPolyline(bottomRight, 3);
        painter->drawPolyline(bottomLeft, 3);
        const QPointF center = crop.center();
        const qreal edge = kEdgeHandleLength / 2.0;
        if (crop.width() >= kEdgeHandleMinimum)
        {
            painter->drawLine(QPointF(center.x() - edge, top), QPointF(center.x() + edge, top));
            painter->drawLine(QPointF(center.x() - edge, bottom), QPointF(center.x() + edge, bottom));
        }
        if (crop.height() >= kEdgeHandleMinimum)
        {
            painter->drawLine(QPointF(left, center.y() - edge), QPointF(left, center.y() + edge));
            painter->drawLine(QPointF(right, center.y() - edge), QPointF(right, center.y() + edge));
        }
    }
} // namespace

ShadImageCropper::ShadImageCropper(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadImageCropperPrivate())
{
    Q_D(ShadImageCropper);
    d->q_ptr = this;
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadImageCropper::~ShadImageCropper()
{
}

void ShadImageCropper::setImage(const QImage& image)
{
    Q_D(ShadImageCropper);
    d->_image = image;
    QImage transformed = image;
    if (!transformed.isNull())
    {
        transformed.setDevicePixelRatio(1.0);
    }
    d->applyTransform(transformed, QRectF());
    d->setCropRectF(d->defaultCropRect());
    d->_pressHandle = ShadImageCropperPrivate::None;
    Q_EMIT imageChanged();
    d->emitCropRectIfChanged();
    update();
}

QImage ShadImageCropper::getImage() const
{
    Q_D(const ShadImageCropper);
    return d->_image;
}

void ShadImageCropper::setAspectRatio(qreal ratio)
{
    Q_D(ShadImageCropper);
    ratio = qMax<qreal>(0.0, ratio);
    if (qFuzzyCompare(d->_aspectRatio + 1.0, ratio + 1.0))
    {
        return;
    }
    d->_aspectRatio = ratio;
    if (!d->_transformed.isNull())
    {
        d->setCropRectF(d->fitAspect(d->_cropRect));
        d->emitCropRectIfChanged();
    }
    update();
}

qreal ShadImageCropper::getAspectRatio() const
{
    Q_D(const ShadImageCropper);
    return d->_aspectRatio;
}

void ShadImageCropper::setShape(ShadImageCropperType::Shape shape)
{
    Q_D(ShadImageCropper);
    if (d->_shape == shape)
    {
        return;
    }
    d->_shape = shape;
    if (!d->_transformed.isNull())
    {
        d->setCropRectF(d->fitAspect(d->_cropRect));
        d->emitCropRectIfChanged();
    }
    update();
}

ShadImageCropperType::Shape ShadImageCropper::getShape() const
{
    Q_D(const ShadImageCropper);
    return d->_shape;
}

void ShadImageCropper::rotateLeft()
{
    Q_D(ShadImageCropper);
    if (d->_transformed.isNull())
    {
        return;
    }
    const qreal width = d->_transformed.width();
    const QRectF rect = d->_cropRect;
    const QRectF mapped(rect.y(), width - rect.x() - rect.width(), rect.height(), rect.width());
    d->applyTransform(d->_transformed.transformed(QTransform().rotate(-90)), mapped);
    d->setCropRectF(d->fitAspect(d->clampToImage(d->_cropRect)));
    d->emitCropRectIfChanged();
    update();
}

void ShadImageCropper::rotateRight()
{
    Q_D(ShadImageCropper);
    if (d->_transformed.isNull())
    {
        return;
    }
    const qreal height = d->_transformed.height();
    const QRectF rect = d->_cropRect;
    const QRectF mapped(height - rect.y() - rect.height(), rect.x(), rect.height(), rect.width());
    d->applyTransform(d->_transformed.transformed(QTransform().rotate(90)), mapped);
    d->setCropRectF(d->fitAspect(d->clampToImage(d->_cropRect)));
    d->emitCropRectIfChanged();
    update();
}

void ShadImageCropper::flipHorizontal()
{
    Q_D(ShadImageCropper);
    if (d->_transformed.isNull())
    {
        return;
    }
    const qreal width = d->_transformed.width();
    const QRectF rect = d->_cropRect;
    const QRectF mapped(width - rect.x() - rect.width(), rect.y(), rect.width(), rect.height());
    d->applyTransform(d->_transformed.mirrored(true, false), mapped);
    d->setCropRectF(d->clampToImage(d->_cropRect));
    d->emitCropRectIfChanged();
    update();
}

void ShadImageCropper::flipVertical()
{
    Q_D(ShadImageCropper);
    if (d->_transformed.isNull())
    {
        return;
    }
    const qreal height = d->_transformed.height();
    const QRectF rect = d->_cropRect;
    const QRectF mapped(rect.x(), height - rect.y() - rect.height(), rect.width(), rect.height());
    d->applyTransform(d->_transformed.mirrored(false, true), mapped);
    d->setCropRectF(d->clampToImage(d->_cropRect));
    d->emitCropRectIfChanged();
    update();
}

void ShadImageCropper::reset()
{
    Q_D(ShadImageCropper);
    if (d->_image.isNull())
    {
        return;
    }
    QImage transformed = d->_image;
    transformed.setDevicePixelRatio(1.0);
    d->applyTransform(transformed, QRectF());
    d->setCropRectF(d->defaultCropRect());
    d->emitCropRectIfChanged();
    update();
}

QRect ShadImageCropper::getCropRect() const
{
    Q_D(const ShadImageCropper);
    return d->roundedCropRect();
}

void ShadImageCropper::setCropRect(const QRect& rect)
{
    Q_D(ShadImageCropper);
    if (d->_transformed.isNull())
    {
        return;
    }
    const QRect bounded = rect.intersected(QRect(QPoint(0, 0), d->_transformed.size()));
    if (bounded.isEmpty())
    {
        return;
    }
    d->setCropRectF(d->fitAspect(QRectF(bounded)));
    d->emitCropRectIfChanged();
    update();
}

QImage ShadImageCropper::croppedImage() const
{
    Q_D(const ShadImageCropper);
    const QRect rect = d->roundedCropRect();
    if (d->_transformed.isNull() || rect.isEmpty())
    {
        return QImage();
    }
    const QImage cropped = d->_transformed.copy(rect);
    if (d->_shape != ShadImageCropperType::Circle)
    {
        return cropped;
    }
    QImage result(cropped.size(), QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(cropped.convertToFormat(QImage::Format_ARGB32_Premultiplied)));
    painter.drawEllipse(QRectF(QPointF(0.0, 0.0), QSizeF(cropped.size())));
    return result;
}

QSize ShadImageCropper::sizeHint() const
{
    return QSize(480, 320);
}

QSize ShadImageCropper::minimumSizeHint() const
{
    return QSize(160, 120);
}

void ShadImageCropper::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadImageCropper);
    using namespace ShadStyleHelper;
    const QRectF box = d->contentRect();
    const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    fillRoundedRect(&painter, box, radius, shadTheme->getColor(ShadThemeType::Muted));
    if (d->_transformed.isNull())
    {
        const QColor muted = shadTheme->getColor(ShadThemeType::MutedForeground);
        QFont font = painter.font();
        font.setPixelSize(kEmptyFontSize);
        painter.setFont(font);
        const int textHeight = painter.fontMetrics().height();
        const qreal top = box.center().y() - (kEmptyIconSize + kEmptyGap + textHeight) / 2.0;
        ShadIcon::paint(&painter, QRectF(box.center().x() - kEmptyIconSize / 2.0, top, kEmptyIconSize, kEmptyIconSize), ShadIconType::Image, muted, kEmptyIconSize);
        painter.setPen(muted);
        painter.drawText(QRectF(box.left(), top + kEmptyIconSize + kEmptyGap, box.width(), textHeight), Qt::AlignHCenter | Qt::AlignTop, tr("未选择图片"));
    }
    else
    {
        d->ensureCache();
        const QRectF display = d->displayRect();
        const QRectF crop = d->cropDisplayRect();
        if (!d->_cache.isNull())
        {
            painter.drawPixmap(display.topLeft(), d->_cache);
        }
        const bool isCircle = d->_shape == ShadImageCropperType::Circle;
        QPainterPath mask;
        mask.setFillRule(Qt::OddEvenFill);
        mask.addRect(display);
        if (isCircle)
        {
            mask.addEllipse(crop);
        }
        else
        {
            mask.addRect(crop);
        }
        painter.fillPath(mask, QColor(0, 0, 0, shadTheme->isDark() ? 153 : 128));
        if (d->_pressHandle != ShadImageCropperPrivate::None)
        {
            painter.save();
            QPainterPath clip;
            if (isCircle)
            {
                clip.addEllipse(crop);
            }
            else
            {
                clip.addRect(crop);
            }
            painter.setClipPath(clip);
            painter.setPen(QPen(withAlpha(Qt::white, 0.4), 1.0));
            for (int index = 1; index < 3; ++index)
            {
                const qreal x = crop.left() + crop.width() * index / 3.0;
                const qreal y = crop.top() + crop.height() * index / 3.0;
                painter.drawLine(QPointF(x, crop.top()), QPointF(x, crop.bottom()));
                painter.drawLine(QPointF(crop.left(), y), QPointF(crop.right(), y));
            }
            painter.restore();
        }
        painter.setPen(QPen(Qt::white, 1.0));
        painter.setBrush(Qt::NoBrush);
        const QRectF borderRect = crop.adjusted(0.5, 0.5, -0.5, -0.5);
        if (isCircle)
        {
            painter.drawEllipse(borderRect);
        }
        else
        {
            painter.drawRect(borderRect);
        }
        drawHandles(&painter, crop);
    }
    strokeRoundedRect(&painter, box, radius, shadTheme->getColor(ShadThemeType::Border));
    if (d->_isFocusVisible && hasFocus())
    {
        drawFocusRing(&painter, box, radius, focusRingColor(false));
    }
}

void ShadImageCropper::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadImageCropper);
    if (event->button() != Qt::LeftButton || d->_transformed.isNull())
    {
        QWidget::mousePressEvent(event);
        return;
    }
    const ShadImageCropperPrivate::Handle handle = d->hitTest(event->position());
    if (handle == ShadImageCropperPrivate::None)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    d->_pressHandle = handle;
    d->_pressPosition = event->position();
    d->_pressRect = d->_cropRect;
    event->accept();
    update();
}

void ShadImageCropper::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadImageCropper);
    if (d->_pressHandle == ShadImageCropperPrivate::None)
    {
        const ShadImageCropperPrivate::Handle handle = d->hitTest(event->position());
        if (handle != d->_hoverHandle)
        {
            d->_hoverHandle = handle;
            updateCursor(this, handle);
        }
        QWidget::mouseMoveEvent(event);
        return;
    }
    const qreal scale = d->displayScale();
    const qreal dx = (event->position().x() - d->_pressPosition.x()) / scale;
    const qreal dy = (event->position().y() - d->_pressPosition.y()) / scale;
    if (d->_pressHandle == ShadImageCropperPrivate::Move)
    {
        d->setCropRectF(d->clampToImage(d->_pressRect.translated(dx, dy)));
    }
    else
    {
        d->setCropRectF(d->resized(d->_pressHandle, dx, dy));
    }
    d->emitCropRectIfChanged();
    event->accept();
    update();
}

void ShadImageCropper::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadImageCropper);
    if (event->button() != Qt::LeftButton || d->_pressHandle == ShadImageCropperPrivate::None)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    d->_pressHandle = ShadImageCropperPrivate::None;
    d->_hoverHandle = d->hitTest(event->position());
    updateCursor(this, d->_hoverHandle);
    event->accept();
    update();
}

void ShadImageCropper::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadImageCropper);
    if (d->_transformed.isNull())
    {
        QWidget::keyPressEvent(event);
        return;
    }
    qreal step = 1.0 / d->displayScale();
    if (event->modifiers() & Qt::ShiftModifier)
    {
        step *= 10.0;
    }
    qreal dx = 0.0;
    qreal dy = 0.0;
    switch (event->key())
    {
        case Qt::Key_Left:
            dx = -step;
            break;
        case Qt::Key_Right:
            dx = step;
            break;
        case Qt::Key_Up:
            dy = -step;
            break;
        case Qt::Key_Down:
            dy = step;
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    d->setCropRectF(d->clampToImage(d->_cropRect.translated(dx, dy)));
    d->emitCropRectIfChanged();
    event->accept();
    update();
}

void ShadImageCropper::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadImageCropper);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusInEvent(event);
    update();
}

void ShadImageCropper::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadImageCropper);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusOutEvent(event);
    update();
}

void ShadImageCropper::leaveEvent(QEvent* event)
{
    Q_D(ShadImageCropper);
    if (d->_pressHandle == ShadImageCropperPrivate::None)
    {
        d->_hoverHandle = ShadImageCropperPrivate::None;
        unsetCursor();
    }
    QWidget::leaveEvent(event);
}

void ShadImageCropper::changeEvent(QEvent* event)
{
    Q_D(ShadImageCropper);
    if (event->type() == QEvent::EnabledChange)
    {
        d->_pressHandle = ShadImageCropperPrivate::None;
        update();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}
