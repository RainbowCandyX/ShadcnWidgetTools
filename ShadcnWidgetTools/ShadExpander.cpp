#include "ShadExpander.h"

#include <QEasingCurve>
#include <QHBoxLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include "ShadCollapsePanel.h"
#include "ShadDef.h"
#include "ShadExpanderHeader.h"
#include "ShadExpanderItem.h"
#include "ShadExpanderPrivate.h"
#include "ShadFrameAnimation.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadExpander::ShadExpander(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadExpanderPrivate())
{
    Q_D(ShadExpander);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    const int border = ShadExpanderPrivate::kBorderWidth;
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(border, border, border, border);
    mainLayout->setSpacing(0);

    d->_header = new ShadExpanderHeader(this);
    connect(d->_header, &ShadExpanderHeader::clicked, this, &ShadExpander::toggle);
    mainLayout->addWidget(d->_header);

    d->_panel = new ShadCollapsePanel(this);
    d->_itemsWidget = new QWidget();
    d->_itemsLayout = new QVBoxLayout(d->_itemsWidget);
    d->_itemsLayout->setContentsMargins(0, 0, 0, 0);
    d->_itemsLayout->setSpacing(0);
    d->_panel->setContent(d->_itemsWidget);
    mainLayout->addWidget(d->_panel);

    d->_animation = new ShadFrameAnimation(this);
    QEasingCurve curve(QEasingCurve::BezierSpline);
    curve.addCubicBezierSegment(QPointF(0.0, 0.0), QPointF(0.58, 1.0), QPointF(1.0, 1.0));
    d->_animation->setEasingCurve(curve);
    connect(d->_animation, &ShadFrameAnimation::valueChanged, this, [d](qreal value) { d->setProgress(value); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadExpander::~ShadExpander()
{
}

QHBoxLayout* ShadExpander::headerLayout() const
{
    Q_D(const ShadExpander);
    return d->_header->contentLayout();
}

void ShadExpander::addItem(QWidget* widget)
{
    Q_D(ShadExpander);
    if (widget == nullptr || getItems().contains(widget))
    {
        return;
    }
    ShadExpanderItem* item = new ShadExpanderItem(widget, d->_itemsWidget);
    d->_itemsLayout->addWidget(item);
    d->_items.append(item);
}

void ShadExpander::removeItem(QWidget* widget)
{
    Q_D(ShadExpander);
    for (int index = 0; index < int(d->_items.size()); ++index)
    {
        ShadExpanderItem* item = d->_items.at(index);
        if (item->content() == widget)
        {
            d->_items.removeAt(index);
            d->_itemsLayout->removeWidget(item);
            item->takeContent();
            delete item;
            return;
        }
    }
}

QList<QWidget*> ShadExpander::getItems() const
{
    Q_D(const ShadExpander);
    QList<QWidget*> widgets;
    widgets.reserve(d->_items.size());
    for (const ShadExpanderItem* item : d->_items)
    {
        widgets.append(item->content());
    }
    return widgets;
}

void ShadExpander::setOpen(bool open)
{
    Q_D(ShadExpander);
    if (d->_isOpen == open)
    {
        return;
    }
    d->_isOpen = open;
    d->applyOpen(true);
    Q_EMIT openChanged(open);
}

bool ShadExpander::isOpen() const
{
    Q_D(const ShadExpander);
    return d->_isOpen;
}

void ShadExpander::setAnimationDuration(int msecs)
{
    Q_D(ShadExpander);
    d->_duration = qBound(0, msecs, 1000);
}

int ShadExpander::getAnimationDuration() const
{
    Q_D(const ShadExpander);
    return d->_duration;
}

void ShadExpander::toggle()
{
    setOpen(!isOpen());
}

void ShadExpander::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    // rounded-lg border bg-card
    const QRectF body(rect());
    const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    fillRoundedRect(&painter, body, radius, shadTheme->getColor(ShadThemeType::Card));
    strokeRoundedRect(&painter, body, radius, shadTheme->getColor(ShadThemeType::Border));
}
