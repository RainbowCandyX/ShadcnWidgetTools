#include "ShadWatermark.h"

#include <QEvent>
#include <QMetaObject>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadTheme.h"
#include "ShadWatermarkPrivate.h"

ShadWatermark::ShadWatermark(QWidget* target)
    : QWidget(target), d_ptr(new ShadWatermarkPrivate())
{
    Q_D(ShadWatermark);
    d->q_ptr = this;
    d->_target = target;
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->invalidate(); });
    if (target != nullptr)
    {
        setGeometry(target->rect());
        target->installEventFilter(this);
    }
    show();
    raise();
}

ShadWatermark::~ShadWatermark()
{
}

QWidget* ShadWatermark::getTarget() const
{
    Q_D(const ShadWatermark);
    return d->_target;
}

void ShadWatermark::setText(const QString& text)
{
    Q_D(ShadWatermark);
    if (d->_text == text)
    {
        return;
    }
    d->_text = text;
    d->invalidate();
}

QString ShadWatermark::getText() const
{
    Q_D(const ShadWatermark);
    return d->_text;
}

void ShadWatermark::setImage(const QImage& image)
{
    Q_D(ShadWatermark);
    d->_image = image;
    d->invalidate();
}

QImage ShadWatermark::getImage() const
{
    Q_D(const ShadWatermark);
    return d->_image;
}

void ShadWatermark::setImageSize(const QSize& size)
{
    Q_D(ShadWatermark);
    if (d->_imageSize == size)
    {
        return;
    }
    d->_imageSize = size;
    d->invalidate();
}

QSize ShadWatermark::getImageSize() const
{
    Q_D(const ShadWatermark);
    return d->imageDisplaySize();
}

void ShadWatermark::setRotation(qreal degrees)
{
    Q_D(ShadWatermark);
    if (qFuzzyCompare(d->_rotation, degrees))
    {
        return;
    }
    d->_rotation = degrees;
    d->invalidate();
}

qreal ShadWatermark::getRotation() const
{
    Q_D(const ShadWatermark);
    return d->_rotation;
}

void ShadWatermark::setGap(const QSize& gap)
{
    Q_D(ShadWatermark);
    if (d->_gap == gap)
    {
        return;
    }
    d->_gap = gap;
    d->invalidate();
}

QSize ShadWatermark::getGap() const
{
    Q_D(const ShadWatermark);
    return d->_gap;
}

void ShadWatermark::setOffset(const QPoint& offset)
{
    Q_D(ShadWatermark);
    d->_offset = offset;
    d->_hasOffset = true;
    d->invalidate();
}

QPoint ShadWatermark::getOffset() const
{
    Q_D(const ShadWatermark);
    return d->effectiveOffset();
}

void ShadWatermark::setFontPixelSize(int pixelSize)
{
    Q_D(ShadWatermark);
    pixelSize = qMax(1, pixelSize);
    if (d->_fontPixelSize == pixelSize)
    {
        return;
    }
    d->_fontPixelSize = pixelSize;
    d->invalidate();
}

int ShadWatermark::getFontPixelSize() const
{
    Q_D(const ShadWatermark);
    return d->_fontPixelSize;
}

void ShadWatermark::setColor(const QColor& color)
{
    Q_D(ShadWatermark);
    d->_color = color;
    d->_hasColor = color.isValid();
    d->invalidate();
}

QColor ShadWatermark::getColor() const
{
    Q_D(const ShadWatermark);
    return d->effectiveColor();
}

void ShadWatermark::setOpacity(qreal opacity)
{
    Q_D(ShadWatermark);
    opacity = qBound<qreal>(0.0, opacity, 1.0);
    if (qFuzzyCompare(d->_opacity, opacity))
    {
        return;
    }
    d->_opacity = opacity;
    d->invalidate();
}

qreal ShadWatermark::getOpacity() const
{
    Q_D(const ShadWatermark);
    return d->_opacity;
}

bool ShadWatermark::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadWatermark);
    if (watched == d->_target)
    {
        if (event->type() == QEvent::Resize)
        {
            setGeometry(d->_target->rect());
        }
        else if (event->type() == QEvent::ChildAdded)
        {
            QMetaObject::invokeMethod(this, [this]() { raise(); }, Qt::QueuedConnection);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadWatermark::changeEvent(QEvent* event)
{
    Q_D(ShadWatermark);
    if (event->type() == QEvent::FontChange)
    {
        d->invalidate();
    }
    QWidget::changeEvent(event);
}

void ShadWatermark::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadWatermark);
    if (d->_text.isEmpty() && d->_image.isNull())
    {
        return;
    }
    const qreal ratio = devicePixelRatioF();
    d->ensureCache(ratio);
    if (d->_cache.isNull())
    {
        return;
    }
    const QSize unit = d->unitSize();
    const QPoint offset = d->effectiveOffset();
    const int originX = ((offset.x() % unit.width()) + unit.width()) % unit.width();
    const int originY = ((offset.y() % unit.height()) + unit.height()) % unit.height();
    QPainter painter(this);
    int row = 0;
    for (int y = originY - unit.height(); y < height(); y += unit.height())
    {
        const int shift = (row % 2 == 1) ? unit.width() / 2 : 0;
        for (int x = originX - 2 * unit.width() + shift; x < width(); x += unit.width())
        {
            painter.drawPixmap(x, y, d->_cache);
        }
        ++row;
    }
}
