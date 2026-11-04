#include "ShadCollapsible.h"

#include <QAbstractButton>
#include <QEasingCurve>
#include <QPointF>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include "ShadCollapsePanel.h"
#include "ShadCollapsiblePrivate.h"
#include "ShadFrameAnimation.h"

ShadCollapsible::ShadCollapsible(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadCollapsiblePrivate())
{
    Q_D(ShadCollapsible);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    d->_headerWidget = new QWidget(this);
    d->_headerLayout = new QVBoxLayout(d->_headerWidget);
    d->_headerLayout->setContentsMargins(0, 0, 0, 0);
    d->_headerLayout->setSpacing(ShadCollapsiblePrivate::kGap);
    mainLayout->addWidget(d->_headerWidget);

    // 间距随内容一起收起
    d->_panel = new ShadCollapsePanel(this);
    d->_contentWidget = new QWidget();
    d->_contentLayout = new QVBoxLayout(d->_contentWidget);
    d->_contentLayout->setContentsMargins(0, ShadCollapsiblePrivate::kGap, 0, 0);
    d->_contentLayout->setSpacing(ShadCollapsiblePrivate::kGap);
    d->_panel->setContent(d->_contentWidget);
    mainLayout->addWidget(d->_panel);

    d->_animation = new ShadFrameAnimation(this);
    QEasingCurve curve(QEasingCurve::BezierSpline);
    curve.addCubicBezierSegment(QPointF(0.0, 0.0), QPointF(0.58, 1.0), QPointF(1.0, 1.0));
    d->_animation->setEasingCurve(curve);
    connect(d->_animation, &ShadFrameAnimation::valueChanged, this, [d](qreal value) { d->_panel->setProgress(value); });
}

ShadCollapsible::~ShadCollapsible()
{
    Q_D(ShadCollapsible);
    QObject::disconnect(d->_triggerConnection);
}

QVBoxLayout* ShadCollapsible::headerLayout() const
{
    Q_D(const ShadCollapsible);
    return d->_headerLayout;
}

QVBoxLayout* ShadCollapsible::contentLayout() const
{
    Q_D(const ShadCollapsible);
    return d->_contentLayout;
}

void ShadCollapsible::setTrigger(QAbstractButton* button)
{
    Q_D(ShadCollapsible);
    if (d->_trigger == button)
    {
        return;
    }
    QObject::disconnect(d->_triggerConnection);
    d->_trigger = button;
    if (button != nullptr)
    {
        d->_triggerConnection = connect(button, &QAbstractButton::clicked, this, &ShadCollapsible::toggle);
    }
}

QAbstractButton* ShadCollapsible::getTrigger() const
{
    Q_D(const ShadCollapsible);
    return d->_trigger;
}

void ShadCollapsible::setOpen(bool open)
{
    Q_D(ShadCollapsible);
    if (d->_isOpen == open)
    {
        return;
    }
    d->_isOpen = open;
    d->applyOpen(true);
    Q_EMIT openChanged(open);
}

bool ShadCollapsible::isOpen() const
{
    Q_D(const ShadCollapsible);
    return d->_isOpen;
}

void ShadCollapsible::setAnimationDuration(int msecs)
{
    Q_D(ShadCollapsible);
    d->_duration = qBound(0, msecs, 1000);
}

int ShadCollapsible::getAnimationDuration() const
{
    Q_D(const ShadCollapsible);
    return d->_duration;
}

void ShadCollapsible::toggle()
{
    setOpen(!isOpen());
}
