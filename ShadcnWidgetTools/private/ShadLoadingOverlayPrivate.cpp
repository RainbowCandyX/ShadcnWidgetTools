#include "ShadLoadingOverlayPrivate.h"

#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>

#include "ShadLoadingOverlay.h"

ShadLoadingOverlayPrivate::ShadLoadingOverlayPrivate()
{
}

ShadLoadingOverlayPrivate::~ShadLoadingOverlayPrivate()
{
}

void ShadLoadingOverlayPrivate::attach(QWidget* target)
{
    Q_Q(ShadLoadingOverlay);
    _target = target;
    q->setParent(target);
    target->installEventFilter(q);
    syncGeometry();
    if (_isShown)
    {
        q->show();
        q->raise();
    }
}

void ShadLoadingOverlayPrivate::detach()
{
    Q_Q(ShadLoadingOverlay);
    if (!_target.isNull())
    {
        _target->removeEventFilter(q);
    }
    _target = nullptr;
    _previousFocus = nullptr;
}

void ShadLoadingOverlayPrivate::syncGeometry()
{
    Q_Q(ShadLoadingOverlay);
    if (!_target.isNull())
    {
        q->setGeometry(_target->rect());
    }
}

void ShadLoadingOverlayPrivate::setShown(bool shown)
{
    Q_Q(ShadLoadingOverlay);
    if (_isShown == shown)
    {
        return;
    }
    _isShown = shown;
    _fadeAnimation->stop();
    if (shown && !_target.isNull())
    {
        syncGeometry();
        q->show();
        q->raise();
        QWidget* focusWidget = QApplication::focusWidget();
        if (focusWidget != nullptr && (focusWidget == _target || _target->isAncestorOf(focusWidget)))
        {
            _previousFocus = focusWidget;
            q->setFocus(Qt::OtherFocusReason);
        }
    }
    if (!shown && q->hasFocus() && !_previousFocus.isNull())
    {
        _previousFocus->setFocus(Qt::OtherFocusReason);
    }
    const qreal endOpacity = shown ? 1.0 : 0.0;
    const QWidget* parent = q->parentWidget();
    if (parent == nullptr || !parent->isVisible())
    {
        _opacityEffect->setOpacity(endOpacity);
        if (!shown)
        {
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

void ShadLoadingOverlayPrivate::handleFocusChanged(QWidget* now)
{
    Q_Q(ShadLoadingOverlay);
    if (!_isShown || now == nullptr || now == q || _target.isNull())
    {
        return;
    }
    if (now == _target || _target->isAncestorOf(now))
    {
        q->setFocus(Qt::OtherFocusReason);
    }
}
