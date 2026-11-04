#include "ShadExpanderPrivate.h"

#include <QApplication>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadCollapsePanel.h"
#include "ShadExpander.h"
#include "ShadExpanderHeader.h"
#include "ShadFrameAnimation.h"

ShadExpanderPrivate::ShadExpanderPrivate()
{
}

ShadExpanderPrivate::~ShadExpanderPrivate()
{
}

void ShadExpanderPrivate::applyOpen(bool isAnimated)
{
    Q_Q(ShadExpander);
    // 收起前移走内容行里的焦点
    if (!_isOpen)
    {
        QWidget* focusWidget = QApplication::focusWidget();
        if (focusWidget != nullptr && _itemsWidget->isAncestorOf(focusWidget))
        {
            _header->setFocus(Qt::OtherFocusReason);
        }
    }
    const qreal target = _isOpen ? 1.0 : 0.0;
    _animation->stop();
    if (!isAnimated || _duration <= 0 || !q->isVisible())
    {
        setProgress(target);
        return;
    }
    _animation->setStartValue(_panel->progress());
    _animation->setEndValue(target);
    _animation->setDuration(_duration);
    _animation->start();
}

void ShadExpanderPrivate::setProgress(qreal progress)
{
    _panel->setProgress(progress);
    _header->setProgress(progress);
}
