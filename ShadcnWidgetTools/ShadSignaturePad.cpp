#include "ShadSignaturePad.h"

#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QImage>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineF>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QTabletEvent>
#include <QtCore/qnamespace.h>
#include <QtMath>

#include "ShadSignaturePadPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadSignaturePad::ShadSignaturePad(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadSignaturePadPrivate())
{
    Q_D(ShadSignaturePad);
    d->q_ptr = this;
    d->_placeholderText = tr("在此签名");
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    d->refreshCursor();
    connect(shadTheme, &ShadTheme::themeChanged, this, [this, d]()
    {
        d->invalidateCache();
        update();
    });
}

ShadSignaturePad::~ShadSignaturePad()
{
}

void ShadSignaturePad::setPenColor(const QColor& color)
{
    Q_D(ShadSignaturePad);
    d->_hasPenColor = color.isValid();
    d->_penColor = color;
    d->invalidateCache();
    update();
}

QColor ShadSignaturePad::getPenColor() const
{
    Q_D(const ShadSignaturePad);
    return d->effectivePenColor();
}

void ShadSignaturePad::setMinPenWidth(qreal width)
{
    Q_D(ShadSignaturePad);
    d->_minPenWidth = qMax<qreal>(0.1, width);
    d->_maxPenWidth = qMax(d->_maxPenWidth, d->_minPenWidth);
}

qreal ShadSignaturePad::getMinPenWidth() const
{
    Q_D(const ShadSignaturePad);
    return d->_minPenWidth;
}

void ShadSignaturePad::setMaxPenWidth(qreal width)
{
    Q_D(ShadSignaturePad);
    d->_maxPenWidth = qMax<qreal>(0.1, width);
    d->_minPenWidth = qMin(d->_minPenWidth, d->_maxPenWidth);
}

qreal ShadSignaturePad::getMaxPenWidth() const
{
    Q_D(const ShadSignaturePad);
    return d->_maxPenWidth;
}

void ShadSignaturePad::setPlaceholderText(const QString& text)
{
    Q_D(ShadSignaturePad);
    d->_hasCustomPlaceholder = true;
    d->_placeholderText = text;
    update();
}

QString ShadSignaturePad::getPlaceholderText() const
{
    Q_D(const ShadSignaturePad);
    return d->_placeholderText;
}

void ShadSignaturePad::setReadOnly(bool readOnly)
{
    Q_D(ShadSignaturePad);
    if (d->_isReadOnly == readOnly)
    {
        return;
    }
    d->_isReadOnly = readOnly;
    d->endStroke();
    d->refreshCursor();
    update();
}

bool ShadSignaturePad::isReadOnly() const
{
    Q_D(const ShadSignaturePad);
    return d->_isReadOnly;
}

bool ShadSignaturePad::isEmpty() const
{
    Q_D(const ShadSignaturePad);
    return d->_strokes.isEmpty();
}

int ShadSignaturePad::strokeCount() const
{
    Q_D(const ShadSignaturePad);
    return d->_strokes.size();
}

QImage ShadSignaturePad::toImage(bool trimmed, const QColor& background) const
{
    Q_D(const ShadSignaturePad);
    if (d->_strokes.isEmpty())
    {
        return QImage();
    }
    const qreal ratio = devicePixelRatioF();
    QRectF source = QRectF(rect());
    if (trimmed)
    {
        source = d->strokesBounds().adjusted(-ShadSignaturePadPrivate::kTrimMargin, -ShadSignaturePadPrivate::kTrimMargin, ShadSignaturePadPrivate::kTrimMargin, ShadSignaturePadPrivate::kTrimMargin);
    }
    QImage image(QSize(qCeil(source.width() * ratio), qCeil(source.height() * ratio)), QImage::Format_ARGB32_Premultiplied);
    if (image.isNull())
    {
        return QImage();
    }
    image.setDevicePixelRatio(ratio);
    image.fill(background);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(-source.topLeft());
    for (const ShadSignaturePadPrivate::Stroke& stroke : d->_strokes)
    {
        d->paintStroke(&painter, stroke, true);
    }
    return image;
}

QSize ShadSignaturePad::sizeHint() const
{
    return QSize(400, 200);
}

QSize ShadSignaturePad::minimumSizeHint() const
{
    return QSize(160, 80);
}

void ShadSignaturePad::undo()
{
    Q_D(ShadSignaturePad);
    if (d->_strokes.isEmpty())
    {
        return;
    }
    d->_isDrawing = false;
    d->_isTabletStroke = false;
    d->_strokes.removeLast();
    d->invalidateCache();
    update();
    Q_EMIT changed();
}

void ShadSignaturePad::clear()
{
    Q_D(ShadSignaturePad);
    if (d->_strokes.isEmpty())
    {
        return;
    }
    d->_isDrawing = false;
    d->_isTabletStroke = false;
    d->_strokes.clear();
    d->invalidateCache();
    update();
    Q_EMIT changed();
}

void ShadSignaturePad::changeEvent(QEvent* event)
{
    Q_D(ShadSignaturePad);
    if (event->type() == QEvent::EnabledChange)
    {
        d->endStroke();
        d->refreshCursor();
        update();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        if (!d->_hasCustomPlaceholder)
        {
            d->_placeholderText = tr("在此签名");
        }
        update();
    }
    QWidget::changeEvent(event);
}

void ShadSignaturePad::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadSignaturePad);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusInEvent(event);
    update();
}

void ShadSignaturePad::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadSignaturePad);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusOutEvent(event);
    update();
}

void ShadSignaturePad::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadSignaturePad);
    if (d->canDraw() && !d->_isDrawing)
    {
        if (event->matches(QKeySequence::Undo))
        {
            undo();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
        {
            clear();
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void ShadSignaturePad::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadSignaturePad);
    if (event->button() != Qt::LeftButton || !d->canDraw() || d->_isDrawing)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    d->beginStroke(event->position(), static_cast<qint64>(event->timestamp()), 0.0);
    event->accept();
}

void ShadSignaturePad::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadSignaturePad);
    if (!d->_isDrawing || d->_isTabletStroke)
    {
        QWidget::mouseMoveEvent(event);
        return;
    }
    d->appendPoint(event->position(), static_cast<qint64>(event->timestamp()), 0.0);
    event->accept();
}

void ShadSignaturePad::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadSignaturePad);
    if (event->button() != Qt::LeftButton || !d->_isDrawing || d->_isTabletStroke)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    d->appendPoint(event->position(), static_cast<qint64>(event->timestamp()), 0.0);
    d->endStroke();
    event->accept();
}

void ShadSignaturePad::tabletEvent(QTabletEvent* event)
{
    Q_D(ShadSignaturePad);
    if (!d->canDraw())
    {
        event->ignore();
        return;
    }
    const qint64 timestamp = static_cast<qint64>(event->timestamp());
    if (event->type() == QEvent::TabletPress)
    {
        if (!d->_isDrawing)
        {
            d->beginStroke(event->position(), timestamp, event->pressure());
            d->_isTabletStroke = true;
        }
    }
    else if (event->type() == QEvent::TabletMove)
    {
        if (d->_isDrawing && d->_isTabletStroke)
        {
            d->appendPoint(event->position(), timestamp, event->pressure());
        }
    }
    else if (event->type() == QEvent::TabletRelease)
    {
        if (d->_isDrawing && d->_isTabletStroke)
        {
            d->appendPoint(event->position(), timestamp, 0.0);
            d->endStroke();
        }
    }
    event->accept();
}

void ShadSignaturePad::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadSignaturePad);
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    if (shadTheme->isDark())
    {
        fillRoundedRect(&painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3));
    }
    const bool showFocus = hasFocus() && d->_isFocusVisible;
    strokeRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(showFocus ? ShadThemeType::Ring : ShadThemeType::Input));
    if (showFocus)
    {
        drawFocusRing(&painter, bodyRect, radius, focusRingColor(false));
    }

    const qreal baselineY = bodyRect.bottom() - bodyRect.height() / 4.0;
    const qreal leftX = bodyRect.left() + ShadSignaturePadPrivate::kBaselineInset;
    const qreal rightX = bodyRect.right() - ShadSignaturePadPrivate::kBaselineInset;
    if (rightX > leftX)
    {
        QPen dashPen(shadTheme->getColor(ShadThemeType::Border), 1.0, Qt::DashLine);
        painter.setPen(dashPen);
        painter.drawLine(QLineF(leftX, baselineY, rightX, baselineY));
        const qreal half = ShadSignaturePadPrivate::kMarkerHalfSize;
        const QPointF center(leftX + half, baselineY - half - 6.0);
        painter.setPen(QPen(shadTheme->getColor(ShadThemeType::MutedForeground), 1.5, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QLineF(center.x() - half, center.y() - half, center.x() + half, center.y() + half));
        painter.drawLine(QLineF(center.x() - half, center.y() + half, center.x() + half, center.y() - half));
    }

    if (d->_strokes.isEmpty() && !d->_placeholderText.isEmpty())
    {
        QFont placeholderFont = font();
        placeholderFont.setPixelSize(14);
        painter.setFont(placeholderFont);
        painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
        painter.drawText(bodyRect, Qt::AlignCenter, d->_placeholderText);
    }

    painter.save();
    painter.setClipPath(roundedRectPath(bodyRect, radius));
    if (d->ensureCache())
    {
        painter.drawImage(QPointF(0.0, 0.0), d->_cache);
    }
    if (d->_isDrawing && !d->_strokes.isEmpty())
    {
        d->drawTail(&painter, d->_strokes.last());
    }
    painter.restore();
}
