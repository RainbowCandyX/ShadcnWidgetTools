#include "ShadProgress.h"

#include <QColor>
#include <QEasingCurve>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadProgressPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // 轨道高度，对应 h-2 rounded-full
    constexpr int kTrackHeight = 8;
    constexpr int kDefaultWidth = 200;
    constexpr int kRapidChangeIntervalMs = 100;
} // namespace

ShadProgress::ShadProgress(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadProgressPrivate())
{
    Q_D(ShadProgress);
    d->q_ptr = this;
    d->_valueTransition = new ShadTransition(this);
    d->_valueTransition->setScaledByDistance(false);
    d->_valueTransition->setEasingCurve(QEasingCurve(QEasingCurve::OutCubic));
    d->_valueTransition->setRapidChangeInterval(kRapidChangeIntervalMs);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadProgress::~ShadProgress()
{
}

void ShadProgress::setValue(int value)
{
    Q_D(ShadProgress);
    value = qBound(0, value, 100);
    if (d->_value == value)
    {
        return;
    }
    d->_value = value;
    d->_valueTransition->animateTo(value / 100.0);
    Q_EMIT valueChanged(value);
}

int ShadProgress::getValue() const
{
    Q_D(const ShadProgress);
    return d->_value;
}

QSize ShadProgress::sizeHint() const
{
    return QSize(kDefaultWidth, kTrackHeight);
}

QSize ShadProgress::minimumSizeHint() const
{
    return QSize(kTrackHeight, kTrackHeight);
}

void ShadProgress::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadProgress);
    using namespace ShadStyleHelper;
    const QRectF trackRect(0, (height() - kTrackHeight) / 2, width(), kTrackHeight);
    const qreal radius = kTrackHeight / 2.0;
    const QColor primary = shadTheme->getColor(ShadThemeType::Primary);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    // bg-primary/20 轨道，指示条 bg-primary，被圆角轨道裁剪（overflow-hidden）
    fillRoundedRect(&painter, trackRect, radius, withAlpha(primary, 0.2));
    const qreal ratio = qBound<qreal>(0.0, d->_valueTransition->value(), 1.0);
    if (ratio <= 0.0)
    {
        return;
    }
    QPainterPath trackPath;
    trackPath.addRoundedRect(trackRect, radius, radius);
    painter.setClipPath(trackPath);
    // RTL 时从右侧开始填充
    const QRectF indicatorRect(trackRect.left(), trackRect.top(), trackRect.width() * ratio, trackRect.height());
    painter.fillRect(visualRect(layoutDirection(), trackRect, indicatorRect), primary);
}
