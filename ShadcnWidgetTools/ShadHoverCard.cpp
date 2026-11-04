#include "ShadHoverCard.h"

#include <QEasingCurve>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QHideEvent>
#include <QKeyEvent>
#include <QMetaObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QRectF>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadHoverCardPrivate.h"
#include "ShadPlacement.h"
#include "ShadPopupEffect.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kOpenDurationMs = 150;
    // slide-in-from-*-2
    constexpr qreal kSlideDistance = 8.0;
    constexpr int kContentGap = 16;
} // namespace

ShadHoverCardPrivate::ShadHoverCardPrivate()
{
}

ShadHoverCardPrivate::~ShadHoverCardPrivate()
{
}

QSize ShadHoverCardPrivate::cardSize() const
{
    const int width = _contentWidth;
    const int height = _card->hasHeightForWidth() ? _card->heightForWidth(width) : _card->sizeHint().height();
    return QSize(width, qMax(height, _card->minimumSizeHint().height()));
}

QRect ShadHoverCardPrivate::cardRect() const
{
    Q_Q(const ShadHoverCard);
    QRect card = q->rect().adjusted(kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);
    const int slide = qRound((1.0 - _openProgress) * kSlideDistance);
    switch (_actualSide)
    {
        case ShadPlacementType::Top:
            card.translate(0, slide);
            break;
        case ShadPlacementType::Bottom:
            card.translate(0, -slide);
            break;
        case ShadPlacementType::Left:
            card.translate(slide, 0);
            break;
        case ShadPlacementType::Right:
            card.translate(-slide, 0);
            break;
    }
    return card;
}

void ShadHoverCardPrivate::reposition()
{
    Q_Q(ShadHoverCard);
    if (_trigger.isNull())
    {
        return;
    }
    const int focusMargin = ShadStyleHelper::kFocusRingWidth;
    const QRect localRect = _trigger->rect().adjusted(focusMargin, focusMargin, -focusMargin, -focusMargin);
    const QRect anchorRect(_trigger->mapToGlobal(localRect.topLeft()), localRect.size());
    const ShadPlacement::Result placement = ShadPlacement::place(anchorRect, cardSize(), _side, _align, kSideOffset, _trigger->layoutDirection());
    _actualSide = placement.side;
    q->setGeometry(placement.rect.adjusted(-kShadowMargin, -kShadowMargin, kShadowMargin, kShadowMargin));
    layoutCard();
}

void ShadHoverCardPrivate::layoutCard()
{
    _card->setGeometry(cardRect());
}

void ShadHoverCardPrivate::scheduleOpen()
{
    Q_Q(ShadHoverCard);
    _closeTimer->stop();
    if (q->isVisible() || _openTimer->isActive())
    {
        return;
    }
    if (_openDelay <= 0)
    {
        q->popup();
        return;
    }
    _openTimer->start(_openDelay);
}

void ShadHoverCardPrivate::scheduleClose()
{
    Q_Q(ShadHoverCard);
    _openTimer->stop();
    if (!q->isVisible() || _closeTimer->isActive())
    {
        return;
    }
    if (_closeDelay <= 0)
    {
        q->hide();
        return;
    }
    _closeTimer->start(_closeDelay);
}

ShadHoverCard::ShadHoverCard(QWidget* parent)
    : QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint), d_ptr(new ShadHoverCardPrivate())
{
    Q_D(ShadHoverCard);
    d->q_ptr = this;
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    new ShadPopupEffect(this, [this]()
    {
        Q_D(ShadHoverCard);
        return d->cardRect();
    });

    d->_card = new QWidget(this);
    d->_contentLayout = new QVBoxLayout(d->_card);
    const int inset = ShadHoverCardPrivate::kBorderWidth + d->_padding;
    d->_contentLayout->setContentsMargins(inset, inset, inset, inset);
    d->_contentLayout->setSpacing(kContentGap);
    d->_card->installEventFilter(this);

    d->_openTimer = new QTimer(this);
    d->_openTimer->setSingleShot(true);
    connect(d->_openTimer, &QTimer::timeout, this, [this]() { popup(); });
    d->_closeTimer = new QTimer(this);
    d->_closeTimer->setSingleShot(true);
    connect(d->_closeTimer, &QTimer::timeout, this, [this]() { hide(); });

    d->_openAnimation = new QVariantAnimation(this);
    d->_openAnimation->setDuration(kOpenDurationMs);
    d->_openAnimation->setStartValue(0.0);
    d->_openAnimation->setEndValue(1.0);
    d->_openAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_openAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_openProgress = value.toReal();
        setWindowOpacity(d->_openProgress);
        d->layoutCard();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadHoverCard::~ShadHoverCard()
{
    Q_D(ShadHoverCard);
    if (!d->_trigger.isNull())
    {
        d->_trigger->removeEventFilter(this);
    }
}

void ShadHoverCard::setTrigger(QWidget* trigger)
{
    Q_D(ShadHoverCard);
    if (d->_trigger == trigger)
    {
        return;
    }
    if (!d->_trigger.isNull())
    {
        d->_trigger->removeEventFilter(this);
    }
    d->_openTimer->stop();
    d->_closeTimer->stop();
    hide();
    d->_trigger = trigger;
    if (trigger != nullptr)
    {
        trigger->installEventFilter(this);
    }
}

QWidget* ShadHoverCard::getTrigger() const
{
    Q_D(const ShadHoverCard);
    return d->_trigger.data();
}

QVBoxLayout* ShadHoverCard::contentLayout() const
{
    Q_D(const ShadHoverCard);
    return d->_contentLayout;
}

void ShadHoverCard::setSide(ShadPlacementType::Side side)
{
    Q_D(ShadHoverCard);
    d->_side = side;
    if (isVisible())
    {
        d->reposition();
    }
}

ShadPlacementType::Side ShadHoverCard::getSide() const
{
    Q_D(const ShadHoverCard);
    return d->_side;
}

void ShadHoverCard::setAlign(ShadPlacementType::Align align)
{
    Q_D(ShadHoverCard);
    d->_align = align;
    if (isVisible())
    {
        d->reposition();
    }
}

ShadPlacementType::Align ShadHoverCard::getAlign() const
{
    Q_D(const ShadHoverCard);
    return d->_align;
}

void ShadHoverCard::setOpenDelay(int msecs)
{
    Q_D(ShadHoverCard);
    d->_openDelay = qMax(0, msecs);
}

int ShadHoverCard::getOpenDelay() const
{
    Q_D(const ShadHoverCard);
    return d->_openDelay;
}

void ShadHoverCard::setCloseDelay(int msecs)
{
    Q_D(ShadHoverCard);
    d->_closeDelay = qMax(0, msecs);
}

int ShadHoverCard::getCloseDelay() const
{
    Q_D(const ShadHoverCard);
    return d->_closeDelay;
}

void ShadHoverCard::setContentWidth(int width)
{
    Q_D(ShadHoverCard);
    d->_contentWidth = qMax(2 * (ShadHoverCardPrivate::kBorderWidth + d->_padding), width);
    if (isVisible())
    {
        d->reposition();
    }
}

int ShadHoverCard::getContentWidth() const
{
    Q_D(const ShadHoverCard);
    return d->_contentWidth;
}

void ShadHoverCard::setContentPadding(int padding)
{
    Q_D(ShadHoverCard);
    const int contentPadding = qMax(0, padding);
    if (d->_padding == contentPadding)
    {
        return;
    }
    d->_padding = contentPadding;
    const int inset = ShadHoverCardPrivate::kBorderWidth + contentPadding;
    d->_contentLayout->setContentsMargins(inset, inset, inset, inset);
    d->_contentWidth = qMax(2 * inset, d->_contentWidth);
    if (isVisible())
    {
        d->reposition();
    }
}

int ShadHoverCard::getContentPadding() const
{
    Q_D(const ShadHoverCard);
    return d->_padding;
}

void ShadHoverCard::popup()
{
    Q_D(ShadHoverCard);
    d->_openTimer->stop();
    d->_closeTimer->stop();
    if (d->_trigger.isNull() || !d->_trigger->isVisible())
    {
        return;
    }
    if (isVisible())
    {
        d->reposition();
        return;
    }
    setLayoutDirection(d->_trigger->layoutDirection());
    d->_openAnimation->stop();
    d->_openProgress = 0.0;
    setWindowOpacity(0.0);
    d->reposition();
    Q_EMIT aboutToShow();
    show();
    raise();
    d->_openAnimation->start();
}

QSize ShadHoverCard::sizeHint() const
{
    Q_D(const ShadHoverCard);
    return d->cardSize() + QSize(2 * ShadHoverCardPrivate::kShadowMargin, 2 * ShadHoverCardPrivate::kShadowMargin);
}

bool ShadHoverCard::event(QEvent* event)
{
    Q_D(ShadHoverCard);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        update();
    }
    else if (event->type() == QEvent::FontChange && isVisible())
    {
        d->reposition();
    }
    return QWidget::event(event);
}

bool ShadHoverCard::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadHoverCard);
    if (watched == d->_card)
    {
        if (event->type() == QEvent::LayoutRequest && isVisible())
        {
            QMetaObject::invokeMethod(this, [d]() { d->reposition(); }, Qt::QueuedConnection);
        }
        return QWidget::eventFilter(watched, event);
    }
    if (watched != d->_trigger.data())
    {
        return QWidget::eventFilter(watched, event);
    }
    switch (event->type())
    {
        case QEvent::Enter:
            {
                d->scheduleOpen();
                break;
            }
        case QEvent::Leave:
            {
                d->scheduleClose();
                break;
            }
        case QEvent::FocusIn:
            {
                const Qt::FocusReason reason = static_cast<QFocusEvent*>(event)->reason();
                if (reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason)
                {
                    d->scheduleOpen();
                }
                break;
            }
        case QEvent::FocusOut:
            {
                if (!d->_trigger->underMouse())
                {
                    d->scheduleClose();
                }
                break;
            }
        case QEvent::KeyPress:
            {
                if (isVisible() && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape)
                {
                    hide();
                    return true;
                }
                break;
            }
        case QEvent::Hide:
        case QEvent::EnabledChange:
        case QEvent::ActivationChange:
            {
                if (!d->_trigger->isVisible() || !d->_trigger->isEnabled() || !d->_trigger->isActiveWindow())
                {
                    d->_openTimer->stop();
                    hide();
                }
                break;
            }
        default:
            {
                break;
            }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadHoverCard::enterEvent(QEnterEvent* event)
{
    Q_D(ShadHoverCard);
    d->_closeTimer->stop();
    QWidget::enterEvent(event);
}

void ShadHoverCard::leaveEvent(QEvent* event)
{
    Q_D(ShadHoverCard);
    if (d->_trigger.isNull() || !d->_trigger->underMouse())
    {
        d->scheduleClose();
    }
    QWidget::leaveEvent(event);
}

void ShadHoverCard::hideEvent(QHideEvent* event)
{
    Q_D(ShadHoverCard);
    d->_openAnimation->stop();
    d->_closeTimer->stop();
    Q_EMIT aboutToHide();
    QWidget::hideEvent(event);
}

void ShadHoverCard::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadHoverCard);
    using namespace ShadStyleHelper;
    const QRectF card = d->cardRect();
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    // rounded-md border bg-popover shadow-md
    drawShadowMd(&painter, card, radius);
    ShadPopupEffect::paintBackdrop(&painter, this, card, radius);
    fillRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Popover));
    strokeRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Border));
}
