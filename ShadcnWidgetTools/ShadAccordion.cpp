#include "ShadAccordion.h"

#include <QEasingCurve>
#include <QKeyEvent>
#include <QLayout>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadAccordionPanel.h"
#include "ShadAccordionPrivate.h"
#include "ShadAccordionTrigger.h"
#include "ShadFrameAnimation.h"
#include "ShadLabel.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadAccordion::ShadAccordion(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadAccordionPrivate())
{
    Q_D(ShadAccordion);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    d->_layout = new QVBoxLayout(this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->_layout->setSpacing(0);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadAccordion::~ShadAccordion()
{
}

bool ShadAccordion::addItem(const QString& value, const QString& title, QWidget* content)
{
    Q_D(ShadAccordion);
    if (value.isEmpty() || content == nullptr || d->findItem(value) != nullptr)
    {
        return false;
    }
    ShadAccordionPrivate::Item item;
    item.value = value;
    item.container = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(item.container);
    layout->setSpacing(0);
    item.trigger = new ShadAccordionTrigger(item.container);
    item.trigger->setText(title);
    item.panel = new ShadAccordionPanel(item.container);
    item.panel->setContent(content);
    layout->addWidget(item.trigger);
    layout->addWidget(item.panel);

    // 收起时面板不可见，动画挂在容器上
    QEasingCurve curve(QEasingCurve::BezierSpline);
    curve.addCubicBezierSegment(QPointF(0.0, 0.0), QPointF(0.58, 1.0), QPointF(1.0, 1.0));
    item.animation = new ShadFrameAnimation(item.container);
    item.animation->setEasingCurve(curve);
    ShadAccordionPanel* panel = item.panel;
    connect(item.animation, &ShadFrameAnimation::valueChanged, panel, [panel](qreal progress) { panel->setProgress(progress); });
    connect(item.trigger, &ShadAccordionTrigger::clicked, this, [d, value]() { d->toggle(value); });

    d->_items.append(item);
    d->_layout->addWidget(item.container);
    d->updateItemMargins();
    update();
    return true;
}

ShadLabel* ShadAccordion::addItem(const QString& value, const QString& title, const QString& text)
{
    ShadLabel* label = new ShadLabel(text);
    label->setWordWrap(true);
    if (!addItem(value, title, static_cast<QWidget*>(label)))
    {
        delete label;
        return nullptr;
    }
    return label;
}

void ShadAccordion::removeItem(const QString& value)
{
    Q_D(ShadAccordion);
    for (int index = 0; index < d->_items.size(); ++index)
    {
        if (d->_items.at(index).value != value)
        {
            continue;
        }
        const bool wasExpanded = d->_items.at(index).isExpanded;
        delete d->_items.at(index).container;
        d->_items.removeAt(index);
        d->updateItemMargins();
        update();
        if (wasExpanded)
        {
            Q_EMIT expandedItemsChanged(getExpandedItems());
        }
        return;
    }
}

QStringList ShadAccordion::getItems() const
{
    Q_D(const ShadAccordion);
    QStringList values;
    for (const ShadAccordionPrivate::Item& item : d->_items)
    {
        values.append(item.value);
    }
    return values;
}

void ShadAccordion::setItemTitle(const QString& value, const QString& title)
{
    Q_D(ShadAccordion);
    if (ShadAccordionPrivate::Item* item = d->findItem(value))
    {
        item->trigger->setText(title);
        item->trigger->updateGeometry();
    }
}

QString ShadAccordion::getItemTitle(const QString& value) const
{
    Q_D(const ShadAccordion);
    const ShadAccordionPrivate::Item* item = d->findItem(value);
    return item != nullptr ? item->trigger->text() : QString();
}

void ShadAccordion::setItemDisabled(const QString& value, bool disabled)
{
    Q_D(ShadAccordion);
    if (ShadAccordionPrivate::Item* item = d->findItem(value))
    {
        item->trigger->setEnabled(!disabled);
    }
}

bool ShadAccordion::isItemDisabled(const QString& value) const
{
    Q_D(const ShadAccordion);
    const ShadAccordionPrivate::Item* item = d->findItem(value);
    return item != nullptr && !item->trigger->isEnabled();
}

void ShadAccordion::setMultiple(bool multiple)
{
    Q_D(ShadAccordion);
    if (d->_isMultiple == multiple)
    {
        return;
    }
    d->_isMultiple = multiple;
    if (!multiple)
    {
        d->applyExpanded(getExpandedItems());
    }
}

bool ShadAccordion::isMultiple() const
{
    Q_D(const ShadAccordion);
    return d->_isMultiple;
}

void ShadAccordion::setVariant(ShadAccordionType::Variant variant)
{
    Q_D(ShadAccordion);
    if (d->_variant == variant)
    {
        return;
    }
    d->_variant = variant;
    d->updateItemMargins();
    update();
}

ShadAccordionType::Variant ShadAccordion::getVariant() const
{
    Q_D(const ShadAccordion);
    return d->_variant;
}

void ShadAccordion::setAnimationDuration(int msecs)
{
    Q_D(ShadAccordion);
    d->_duration = qBound(0, msecs, ShadAccordionPrivate::kMaxDurationMs);
}

int ShadAccordion::getAnimationDuration() const
{
    Q_D(const ShadAccordion);
    return d->_duration;
}

void ShadAccordion::setItemExpanded(const QString& value, bool expanded)
{
    Q_D(ShadAccordion);
    const ShadAccordionPrivate::Item* item = d->findItem(value);
    if (item == nullptr || item->isExpanded == expanded)
    {
        return;
    }
    QStringList values = getExpandedItems();
    if (!expanded)
    {
        values.removeAll(value);
    }
    else if (d->_isMultiple)
    {
        values.append(value);
    }
    else
    {
        values = {value};
    }
    d->applyExpanded(values);
}

bool ShadAccordion::isItemExpanded(const QString& value) const
{
    Q_D(const ShadAccordion);
    const ShadAccordionPrivate::Item* item = d->findItem(value);
    return item != nullptr && item->isExpanded;
}

void ShadAccordion::setExpandedItems(const QStringList& values)
{
    Q_D(ShadAccordion);
    d->applyExpanded(values);
}

QStringList ShadAccordion::getExpandedItems() const
{
    Q_D(const ShadAccordion);
    QStringList values;
    for (const ShadAccordionPrivate::Item& item : d->_items)
    {
        if (item.isExpanded)
        {
            values.append(item.value);
        }
    }
    return values;
}

void ShadAccordion::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadAccordion);
    switch (event->key())
    {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Home:
        case Qt::Key_End:
            if (event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::KeypadModifier)
            {
                d->moveFocus(event->key());
                event->accept();
                return;
            }
            break;
        default:
            break;
    }
    QWidget::keyPressEvent(event);
}

void ShadAccordion::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadAccordion);
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QColor border = shadTheme->getColor(ShadThemeType::Border);
    const bool isBordered = d->_variant == ShadAccordionType::Bordered;
    if (isBordered)
    {
        strokeRoundedRect(&painter, QRectF(rect()), shadTheme->getRadius(ShadThemeType::Large), border);
    }
    const int inset = isBordered ? ShadAccordionPrivate::kSeparatorWidth : 0;
    for (int index = 0; index + 1 < d->_items.size(); ++index)
    {
        const QRect geometry = d->_items.at(index).container->geometry();
        painter.fillRect(QRect(inset, geometry.bottom(), width() - 2 * inset, ShadAccordionPrivate::kSeparatorWidth), border);
    }
}
