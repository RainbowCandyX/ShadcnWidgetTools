#include "ShadSkeleton.h"

#include <QEvent>
#include <QHideEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QShowEvent>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadSkeletonPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kDefaultWidth = 200;
    constexpr int kDefaultHeight = 16;
} // namespace

ShadSkeleton::ShadSkeleton(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadSkeletonPrivate())
{
    Q_D(ShadSkeleton);
    d->q_ptr = this;
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadSkeleton::ShadSkeleton(const QSize& size, QWidget* parent)
    : ShadSkeleton(parent)
{
    setFixedSize(size);
}

ShadSkeleton::~ShadSkeleton()
{
    Q_D(ShadSkeleton);
    // 从共用动画中移除，避免动画继续刷新已销毁的组件
    d->_isAnimated = false;
    d->updateAnimation();
}

void ShadSkeleton::setRadius(int radius)
{
    Q_D(ShadSkeleton);
    const int value = qMax(-1, radius);
    if (d->_radius == value)
    {
        return;
    }
    d->_radius = value;
    update();
}

int ShadSkeleton::getRadius() const
{
    Q_D(const ShadSkeleton);
    return d->_radius;
}

void ShadSkeleton::setCircular(bool circular)
{
    Q_D(ShadSkeleton);
    if (d->_isCircular == circular)
    {
        return;
    }
    d->_isCircular = circular;
    update();
}

bool ShadSkeleton::isCircular() const
{
    Q_D(const ShadSkeleton);
    return d->_isCircular;
}

void ShadSkeleton::setAnimated(bool animated)
{
    Q_D(ShadSkeleton);
    if (d->_isAnimated == animated)
    {
        return;
    }
    d->_isAnimated = animated;
    d->updateAnimation();
}

bool ShadSkeleton::isAnimated() const
{
    Q_D(const ShadSkeleton);
    return d->_isAnimated;
}

QSize ShadSkeleton::sizeHint() const
{
    return QSize(kDefaultWidth, kDefaultHeight);
}

QSize ShadSkeleton::minimumSizeHint() const
{
    return QSize(kDefaultHeight, kDefaultHeight);
}

void ShadSkeleton::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadSkeleton::showEvent(QShowEvent* event)
{
    Q_D(ShadSkeleton);
    QWidget::showEvent(event);
    d->updateAnimation();
}

void ShadSkeleton::hideEvent(QHideEvent* event)
{
    Q_D(ShadSkeleton);
    QWidget::hideEvent(event);
    d->updateAnimation();
}

void ShadSkeleton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadSkeleton);
    using namespace ShadStyleHelper;
    const QRectF bodyRect(rect());
    qreal radius = d->_radius >= 0 ? d->_radius : shadTheme->getRadius(ShadThemeType::Medium);
    if (d->_isCircular)
    {
        radius = bodyRect.height() / 2.0;
    }
    radius = qMin(radius, qMin(bodyRect.width(), bodyRect.height()) / 2.0);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity((isEnabled() ? 1.0 : kDisabledOpacity) * d->pulseOpacity());
    fillRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(ShadThemeType::Accent));
}
