#include "ShadMenubarTrigger.h"

#include <QAbstractButton>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // px-2 py-1 text-sm
    constexpr int kPaddingX = 8;
    constexpr int kHeight = 28;
} // namespace

ShadMenubarTrigger::ShadMenubarTrigger(const QString& text, QWidget* parent)
    : QAbstractButton(parent)
{
    setText(text);
    setFocusPolicy(Qt::StrongFocus);
#ifdef Q_OS_MACOS
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadMenubarTrigger::~ShadMenubarTrigger()
{
}

void ShadMenubarTrigger::setOpen(bool open)
{
    if (_isOpen == open)
    {
        return;
    }
    _isOpen = open;
    update();
}

bool ShadMenubarTrigger::isOpen() const
{
    return _isOpen;
}

QSize ShadMenubarTrigger::sizeHint() const
{
    return QSize(ShadStyleHelper::textWidth(font(), text()) + 2 * kPaddingX, kHeight);
}

QSize ShadMenubarTrigger::minimumSizeHint() const
{
    return sizeHint();
}

void ShadMenubarTrigger::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        Q_EMIT triggerPressed();
    }
    event->accept();
}

void ShadMenubarTrigger::mouseReleaseEvent(QMouseEvent* event)
{
    event->accept();
}

void ShadMenubarTrigger::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            event->ignore();
            return;
        default:
            break;
    }
    QAbstractButton::keyPressEvent(event);
}

void ShadMenubarTrigger::enterEvent(QEnterEvent* event)
{
    _isHovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void ShadMenubarTrigger::leaveEvent(QEvent* event)
{
    _isHovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void ShadMenubarTrigger::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QAbstractButton::focusInEvent(event);
    update();
}

void ShadMenubarTrigger::focusOutEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QAbstractButton::focusOutEvent(event);
    update();
}

void ShadMenubarTrigger::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QAbstractButton::changeEvent(event);
}

void ShadMenubarTrigger::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF bounds(rect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::Small);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    // hover:bg-muted aria-expanded:bg-muted
    if ((_isHovered && isEnabled()) || _isOpen)
    {
        fillRoundedRect(&painter, bounds, radius, shadTheme->getColor(ShadThemeType::Muted));
    }
    painter.setFont(font());
    painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
    painter.drawText(bounds, Qt::AlignCenter, text());
    if (hasFocus() && _isFocusVisible)
    {
        const QRectF box = bounds.adjusted(kFocusRingWidth, kFocusRingWidth, -kFocusRingWidth, -kFocusRingWidth);
        strokeRoundedRect(&painter, box, radius, shadTheme->getColor(ShadThemeType::Ring));
        drawFocusRing(&painter, box, radius, focusRingColor(false));
    }
}
