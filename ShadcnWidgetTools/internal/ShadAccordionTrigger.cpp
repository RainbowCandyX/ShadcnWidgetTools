#include "ShadAccordionTrigger.h"

#include <QAbstractButton>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <climits>

#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kPaddingY = 16;
    constexpr int kIconSize = 16;
    constexpr int kMinTextWidth = 40;
} // namespace

ShadAccordionTrigger::ShadAccordionTrigger(QWidget* parent)
    : QAbstractButton(parent)
{
    setFocusPolicy(Qt::StrongFocus);
#ifdef Q_OS_MACOS
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadAccordionTrigger::~ShadAccordionTrigger()
{
}

void ShadAccordionTrigger::setExpanded(bool expanded)
{
    if (_isExpanded == expanded)
    {
        return;
    }
    _isExpanded = expanded;
    update();
}

bool ShadAccordionTrigger::isExpanded() const
{
    return _isExpanded;
}

QSize ShadAccordionTrigger::sizeHint() const
{
    const int width = QFontMetrics(font()).horizontalAdvance(text()) + kIconSize + 2 * ShadStyleHelper::kFocusRingWidth;
    return QSize(width, heightForWidth(width));
}

QSize ShadAccordionTrigger::minimumSizeHint() const
{
    const int width = kMinTextWidth + kIconSize + 2 * ShadStyleHelper::kFocusRingWidth;
    return QSize(width, 2 * kPaddingY + kIconSize);
}

bool ShadAccordionTrigger::hasHeightForWidth() const
{
    return true;
}

int ShadAccordionTrigger::heightForWidth(int width) const
{
    return 2 * kPaddingY + qMax(textHeight(textWidth(width)), kIconSize);
}

void ShadAccordionTrigger::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Home:
        case Qt::Key_End:
            event->ignore();
            return;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (!event->isAutoRepeat())
            {
                click();
            }
            event->accept();
            return;
        default:
            break;
    }
    QAbstractButton::keyPressEvent(event);
}

void ShadAccordionTrigger::enterEvent(QEnterEvent* event)
{
    _isHovered = true;
    update();
    QAbstractButton::enterEvent(event);
}

void ShadAccordionTrigger::leaveEvent(QEvent* event)
{
    _isHovered = false;
    update();
    QAbstractButton::leaveEvent(event);
}

void ShadAccordionTrigger::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QAbstractButton::focusInEvent(event);
    update();
}

void ShadAccordionTrigger::focusOutEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QAbstractButton::focusOutEvent(event);
    update();
}

void ShadAccordionTrigger::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QAbstractButton::changeEvent(event);
}

void ShadAccordionTrigger::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const Qt::LayoutDirection direction = layoutDirection();
    const QRectF bounds(rect());
    const int ring = kFocusRingWidth;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    if (hasFocus() && _isFocusVisible)
    {
        const QRectF box = bounds.adjusted(ring, ring, -ring, -ring);
        const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
        strokeRoundedRect(&painter, box, radius, shadTheme->getColor(ShadThemeType::Ring));
        drawFocusRing(&painter, box, radius, focusRingColor(false));
    }

    const int contentWidth = textWidth(width());
    QFont titleFont = font();
    titleFont.setUnderline(_isHovered && isEnabled());
    painter.setFont(titleFont);
    painter.setPen(shadTheme->getColor(ShadThemeType::Foreground));
    const QRectF textRect = visualRect(direction, bounds, QRectF(ring, kPaddingY, contentWidth, height() - 2 * kPaddingY));
    const int textFlags = static_cast<int>(visualAlignment(direction, Qt::AlignLeft | Qt::AlignTop)) | Qt::TextWordWrap;
    painter.drawText(textRect, textFlags, text());

    const QRectF iconRect = visualRect(direction, bounds, QRectF(width() - ring - kIconSize, kPaddingY, kIconSize, kIconSize));
    ShadIcon::paint(&painter, iconRect, _isExpanded ? ShadIconType::ChevronUp : ShadIconType::ChevronDown, shadTheme->getColor(ShadThemeType::MutedForeground), kIconSize);
}

int ShadAccordionTrigger::textWidth(int width) const
{
    return qMax(1, width - 2 * ShadStyleHelper::kFocusRingWidth - kIconSize);
}

int ShadAccordionTrigger::textHeight(int width) const
{
    if (text().isEmpty())
    {
        return 0;
    }
    return QFontMetrics(font()).boundingRect(QRect(0, 0, width, INT_MAX), Qt::TextWordWrap, text()).height();
}
