#include "ShadBackTopPrivate.h"

#include <QAbstractAnimation>
#include <QAbstractScrollArea>
#include <QGraphicsOpacityEffect>
#include <QObject>
#include <QRect>
#include <QScrollBar>
#include <QSize>
#include <QString>
#include <QVariantAnimation>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadBackTop.h"
#include "ShadStyleHelper.h"

ShadBackTopPrivate::ShadBackTopPrivate()
{
}

ShadBackTopPrivate::~ShadBackTopPrivate()
{
}

void ShadBackTopPrivate::attach(QAbstractScrollArea* target)
{
    _target = target;
    q->setParent(target);
    _opacityEffect->setOpacity(0.0);
    _isShown = false;
    _viewport = target->viewport();
    target->installEventFilter(q);
    _viewport->installEventFilter(q);
    QScrollBar* scrollBar = target->verticalScrollBar();
    _targetConnections.append(QObject::connect(scrollBar, &QScrollBar::valueChanged, q, [this]()
    {
        if (!_isAnimatingScroll && _scrollAnimation->state() == QAbstractAnimation::Running)
        {
            _scrollAnimation->stop();
        }
        updateVisibility();
    }));
    _targetConnections.append(QObject::connect(scrollBar, &QScrollBar::rangeChanged, q, [this]() { updateVisibility(); }));
    updatePosition();
    updateVisibility();
}

void ShadBackTopPrivate::detach()
{
    for (const QMetaObject::Connection& connection : std::as_const(_targetConnections))
    {
        QObject::disconnect(connection);
    }
    _targetConnections.clear();
    _scrollAnimation->stop();
    if (!_target.isNull())
    {
        _target->removeEventFilter(q);
    }
    if (!_viewport.isNull())
    {
        _viewport->removeEventFilter(q);
    }
    _target = nullptr;
    _viewport = nullptr;
}

void ShadBackTopPrivate::updatePosition()
{
    if (_target.isNull())
    {
        return;
    }
    const QSize size = q->sizeHint();
    q->resize(size);
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const QRect viewportRect = _target->viewport()->geometry();
    const int x = q->layoutDirection() == Qt::RightToLeft ? viewportRect.left() + _offset - margin : viewportRect.right() + 1 - _offset - (size.width() - margin);
    const int y = viewportRect.bottom() + 1 - _offset - (size.height() - margin);
    q->move(x, y);
    q->raise();
}

void ShadBackTopPrivate::updateVisibility()
{
    if (_target.isNull())
    {
        setShown(false);
        return;
    }
    const QScrollBar* scrollBar = _target->verticalScrollBar();
    setShown(scrollBar->maximum() > scrollBar->minimum() && scrollBar->value() - scrollBar->minimum() >= _visibilityHeight);
}

void ShadBackTopPrivate::setShown(bool shown)
{
    if (_isShown == shown)
    {
        return;
    }
    _isShown = shown;
    _fadeAnimation->stop();
    const qreal endOpacity = shown ? 1.0 : 0.0;
    if (shown)
    {
        q->show();
        q->raise();
    }
    const QWidget* parent = q->parentWidget();
    if (parent == nullptr || !parent->isVisible())
    {
        _opacityEffect->setOpacity(endOpacity);
        if (!shown)
        {
            ShadStyleHelper::clearFocusWithin(q);
            q->hide();
        }
        return;
    }
    const qreal startOpacity = _opacityEffect->opacity();
    _fadeAnimation->setStartValue(startOpacity);
    _fadeAnimation->setEndValue(endOpacity);
    _fadeAnimation->setDuration(qMax(1, qRound(kFadeDurationMs * qAbs(endOpacity - startOpacity))));
    _fadeAnimation->start();
}

void ShadBackTopPrivate::retranslate()
{
    ShadStyleHelper::applyDefaultText(q, "toolTip", q->toolTip(), ShadBackTop::tr("回到顶部"), [this](const QString& text) { q->setToolTip(text); });
    ShadStyleHelper::applyDefaultText(q, "accessibleName", q->accessibleName(), ShadBackTop::tr("回到顶部"), [this](const QString& text) { q->setAccessibleName(text); });
}
