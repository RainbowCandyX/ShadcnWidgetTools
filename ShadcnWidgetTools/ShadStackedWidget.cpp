#include "ShadStackedWidget.h"

#include <QWidget>

#include "ShadPageTransition.h"
#include "ShadStackedWidgetPrivate.h"

ShadStackedWidget::ShadStackedWidget(QWidget* parent)
    : QStackedWidget(parent), d_ptr(new ShadStackedWidgetPrivate())
{
    Q_D(ShadStackedWidget);
    d->q_ptr = this;
    d->_transition = new ShadPageTransition(this);
    connect(this, &QStackedWidget::currentChanged, this, [this, d]()
    {
        QWidget* previous = d->_shownWidget.data();
        QWidget* next = currentWidget();
        d->_shownWidget = next;
        // 移除的页面不播放
        if (previous == nullptr || next == nullptr || previous == next || indexOf(previous) < 0)
        {
            return;
        }
        // 切换后才收到信号，此时截取已隐藏的旧页面
        d->_transition->capture(previous);
        d->_transition->play(next, indexOf(next) >= indexOf(previous) ? 1 : -1);
    });
    connect(this, &QStackedWidget::widgetRemoved, this, [d]() { d->_transition->finish(); });
}

ShadStackedWidget::~ShadStackedWidget()
{
}
