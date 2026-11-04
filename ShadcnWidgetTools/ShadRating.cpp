#include "ShadRating.h"

#include <QBrush>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>

#include "ShadRatingPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadRating::ShadRating(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadRatingPrivate())
{
    Q_D(ShadRating);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadRating::~ShadRating()
{
}

void ShadRating::setRating(int rating)
{
    Q_D(ShadRating);
    rating = qBound(0, rating, d->_maximum);
    if (d->_rating == rating)
    {
        return;
    }
    d->_rating = rating;
    update();
    Q_EMIT ratingChanged(rating);
}

int ShadRating::getRating() const
{
    Q_D(const ShadRating);
    return d->_rating;
}

void ShadRating::setMaximum(int maximum)
{
    Q_D(ShadRating);
    maximum = qMax(1, maximum);
    if (d->_maximum == maximum)
    {
        return;
    }
    d->_maximum = maximum;
    d->_hoveredStar = -1;
    updateGeometry();
    update();
    if (d->_rating > maximum)
    {
        setRating(maximum);
    }
}

int ShadRating::getMaximum() const
{
    Q_D(const ShadRating);
    return d->_maximum;
}

void ShadRating::setStarSize(int size)
{
    Q_D(ShadRating);
    size = qMax(1, size);
    if (d->_starSize == size)
    {
        return;
    }
    d->_starSize = size;
    updateGeometry();
    update();
}

int ShadRating::getStarSize() const
{
    Q_D(const ShadRating);
    return d->_starSize;
}

void ShadRating::setSpacing(int spacing)
{
    Q_D(ShadRating);
    spacing = qMax(0, spacing);
    if (d->_spacing == spacing)
    {
        return;
    }
    d->_spacing = spacing;
    updateGeometry();
    update();
}

int ShadRating::getSpacing() const
{
    Q_D(const ShadRating);
    return d->_spacing;
}

void ShadRating::setReadOnly(bool readOnly)
{
    Q_D(ShadRating);
    if (d->_isReadOnly == readOnly)
    {
        return;
    }
    d->_isReadOnly = readOnly;
    d->_hoveredStar = -1;
    // 只读时不参与 Tab 顺序；先清除焦点，避免焦点按 Tab 顺序跳到其他组件
    if (readOnly)
    {
        ShadStyleHelper::clearFocusWithin(this);
    }
    setFocusPolicy(readOnly ? Qt::NoFocus : Qt::StrongFocus);
    if (readOnly)
    {
        unsetCursor();
    }
    else
    {
        setCursor(Qt::PointingHandCursor);
    }
    update();
}

bool ShadRating::isReadOnly() const
{
    Q_D(const ShadRating);
    return d->_isReadOnly;
}

void ShadRating::setActiveColor(const QColor& color)
{
    Q_D(ShadRating);
    d->_activeColor = color;
    update();
}

QColor ShadRating::getActiveColor() const
{
    Q_D(const ShadRating);
    return d->_activeColor.isValid() ? d->_activeColor : shadTheme->getColor(ShadThemeType::Primary);
}

QSize ShadRating::sizeHint() const
{
    Q_D(const ShadRating);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    return QSize(d->_maximum * d->_starSize + (d->_maximum - 1) * d->_spacing + margin, d->_starSize + margin);
}

QSize ShadRating::minimumSizeHint() const
{
    return sizeHint();
}

void ShadRating::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadRating);
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    // 悬停时预览到光标所在的星星
    const int effectiveRating = d->isInteractive() && d->_hoveredStar >= 0 ? d->_hoveredStar + 1 : d->_rating;
    const QColor activeColor = getActiveColor();
    const QColor inactiveColor = shadTheme->getColor(ShadThemeType::MutedForeground);
    // stroke-width 2（24 视口），圆角连接与 Lucide 一致
    const qreal strokeWidth = d->_starSize / 12.0;
    for (int index = 0; index < d->_maximum; ++index)
    {
        const bool isActive = index < effectiveRating;
        const QColor color = isActive ? activeColor : inactiveColor;
        painter.setPen(QPen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(isActive ? QBrush(color) : QBrush(Qt::NoBrush));
        painter.drawPath(ShadRatingPrivate::starPath(d->starRect(index)));
    }
    // 键盘聚焦时在整排星星外侧显示焦点环
    if (hasFocus() && d->_isFocusVisible)
    {
        drawFocusRing(&painter, insetRect(rect(), kFocusRingWidth), shadTheme->getRadius(ShadThemeType::Small), focusRingColor(false));
    }
}

void ShadRating::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadRating);
    if (d->isInteractive())
    {
        d->setHoveredStar(d->starAt(event->position().toPoint()));
    }
    QWidget::mouseMoveEvent(event);
}

void ShadRating::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadRating);
    if (event->button() == Qt::LeftButton && d->isInteractive())
    {
        const int index = d->starAt(event->position().toPoint());
        if (index >= 0)
        {
            setRating(index + 1);
        }
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadRating::leaveEvent(QEvent* event)
{
    Q_D(ShadRating);
    d->setHoveredStar(-1);
    QWidget::leaveEvent(event);
}

void ShadRating::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadRating);
    if (!d->isInteractive())
    {
        QWidget::keyPressEvent(event);
        return;
    }
    // 右方向键朝结束侧增加，RTL 下结束侧在左
    const int forward = layoutDirection() == Qt::RightToLeft ? -1 : 1;
    switch (event->key())
    {
        case Qt::Key_Right:
            setRating(d->_rating + forward);
            break;
        case Qt::Key_Left:
            setRating(d->_rating - forward);
            break;
        case Qt::Key_Up:
            setRating(d->_rating + 1);
            break;
        case Qt::Key_Down:
            setRating(d->_rating - 1);
            break;
        case Qt::Key_Home:
            setRating(0);
            break;
        case Qt::Key_End:
            setRating(d->_maximum);
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    event->accept();
}

void ShadRating::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadRating);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    QWidget::focusInEvent(event);
    update();
}

void ShadRating::focusOutEvent(QFocusEvent* event)
{
    QWidget::focusOutEvent(event);
    update();
}

void ShadRating::changeEvent(QEvent* event)
{
    Q_D(ShadRating);
    if (event->type() == QEvent::EnabledChange)
    {
        d->_hoveredStar = -1;
        update();
    }
    else if (event->type() == QEvent::LayoutDirectionChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}
