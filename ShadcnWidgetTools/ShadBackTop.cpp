#include "ShadBackTop.h"

#include <QAbstractButton>
#include <QAbstractScrollArea>
#include <QEasingCurve>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QObject>
#include <QScrollBar>
#include <QVariant>
#include <QVariantAnimation>
#include <QtGlobal>

#include "ShadBackTopPrivate.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"

ShadBackTop::ShadBackTop(QAbstractScrollArea* target)
    : ShadButton(target), _private(new ShadBackTopPrivate())
{
    ShadBackTopPrivate* d = _private.data();
    d->q = this;
    setVariant(ShadButtonType::Outline);
    setButtonSize(ShadButtonType::Icon);
    setRoundedFull(true);
    setShadIcon(ShadIconType::ArrowUp);

    d->_opacityEffect = new QGraphicsOpacityEffect(this);
    d->_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(d->_opacityEffect);
    d->_fadeAnimation = new QVariantAnimation(this);
    d->_fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_fadeAnimation, &QVariantAnimation::valueChanged, this, [d](const QVariant& value) { d->_opacityEffect->setOpacity(value.toReal()); });
    connect(d->_fadeAnimation, &QVariantAnimation::finished, this, [this, d]()
    {
        if (!d->_isShown)
        {
            ShadStyleHelper::clearFocusWithin(this);
            hide();
        }
    });
    d->_scrollAnimation = new QVariantAnimation(this);
    d->_scrollAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    connect(d->_scrollAnimation, &QVariantAnimation::valueChanged, this, [d](const QVariant& value)
    {
        if (d->_target.isNull())
        {
            return;
        }
        d->_isAnimatingScroll = true;
        d->_target->verticalScrollBar()->setValue(value.toInt());
        d->_isAnimatingScroll = false;
    });
    connect(this, &QAbstractButton::clicked, this, &ShadBackTop::scrollToTop);
    hide();
    d->retranslate();
    if (target != nullptr)
    {
        d->attach(target);
    }
}

ShadBackTop::~ShadBackTop()
{
    ShadBackTopPrivate* d = _private.data();
    d->_fadeAnimation->stop();
    QObject::disconnect(d->_fadeAnimation, nullptr, this, nullptr);
    QObject::disconnect(d->_scrollAnimation, nullptr, this, nullptr);
    d->detach();
}

void ShadBackTop::setTarget(QAbstractScrollArea* target)
{
    ShadBackTopPrivate* d = _private.data();
    if (d->_target == target)
    {
        return;
    }
    d->detach();
    if (target == nullptr)
    {
        d->setShown(false);
        return;
    }
    d->attach(target);
}

QAbstractScrollArea* ShadBackTop::getTarget() const
{
    return _private->_target.data();
}

void ShadBackTop::setVisibilityHeight(int height)
{
    ShadBackTopPrivate* d = _private.data();
    height = qMax(0, height);
    if (d->_visibilityHeight == height)
    {
        return;
    }
    d->_visibilityHeight = height;
    d->updateVisibility();
}

int ShadBackTop::getVisibilityHeight() const
{
    return _private->_visibilityHeight;
}

void ShadBackTop::setOffset(int offset)
{
    ShadBackTopPrivate* d = _private.data();
    offset = qMax(0, offset);
    if (d->_offset == offset)
    {
        return;
    }
    d->_offset = offset;
    d->updatePosition();
}

int ShadBackTop::getOffset() const
{
    return _private->_offset;
}

void ShadBackTop::setDuration(int durationMs)
{
    _private->_durationMs = qMax(0, durationMs);
}

int ShadBackTop::getDuration() const
{
    return _private->_durationMs;
}

void ShadBackTop::scrollToTop()
{
    ShadBackTopPrivate* d = _private.data();
    if (d->_target.isNull())
    {
        return;
    }
    QScrollBar* scrollBar = d->_target->verticalScrollBar();
    d->_scrollAnimation->stop();
    if (scrollBar->value() == scrollBar->minimum())
    {
        return;
    }
    if (d->_durationMs <= 0 || !d->_target->isVisible())
    {
        scrollBar->setValue(scrollBar->minimum());
        return;
    }
    d->_scrollAnimation->setStartValue(scrollBar->value());
    d->_scrollAnimation->setEndValue(scrollBar->minimum());
    d->_scrollAnimation->setDuration(d->_durationMs);
    d->_scrollAnimation->start();
}

bool ShadBackTop::eventFilter(QObject* watched, QEvent* event)
{
    ShadBackTopPrivate* d = _private.data();
    const bool isTarget = !d->_target.isNull() && (watched == d->_target || watched == d->_target->viewport());
    if (isTarget && (event->type() == QEvent::Resize || event->type() == QEvent::Move))
    {
        d->updatePosition();
    }
    return ShadButton::eventFilter(watched, event);
}

void ShadBackTop::changeEvent(QEvent* event)
{
    ShadBackTopPrivate* d = _private.data();
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->updatePosition();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        d->retranslate();
    }
    ShadButton::changeEvent(event);
}
