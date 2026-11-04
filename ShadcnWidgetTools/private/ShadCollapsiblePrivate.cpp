#include "ShadCollapsiblePrivate.h"

#include <QAbstractButton>
#include <QApplication>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadCollapsePanel.h"
#include "ShadCollapsible.h"
#include "ShadFrameAnimation.h"

ShadCollapsiblePrivate::ShadCollapsiblePrivate()
{
}

ShadCollapsiblePrivate::~ShadCollapsiblePrivate()
{
}

void ShadCollapsiblePrivate::applyOpen(bool isAnimated)
{
    Q_Q(ShadCollapsible);
    // 收起前移走内容区里的焦点
    if (!_isOpen)
    {
        QWidget* focusWidget = QApplication::focusWidget();
        if (focusWidget != nullptr && _contentWidget->isAncestorOf(focusWidget))
        {
            if (!_trigger.isNull() && _trigger->isVisible())
            {
                _trigger->setFocus(Qt::OtherFocusReason);
            }
            else
            {
                focusWidget->clearFocus();
            }
        }
    }
    const qreal target = _isOpen ? 1.0 : 0.0;
    _animation->stop();
    if (!isAnimated || _duration <= 0 || !q->isVisible())
    {
        _panel->setProgress(target);
        return;
    }
    _animation->setStartValue(_panel->progress());
    _animation->setEndValue(target);
    _animation->setDuration(_duration);
    _animation->start();
}
