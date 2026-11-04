#include "ShadSpinner.h"

#include <QEasingCurve>
#include <QEvent>
#include <QHideEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QShowEvent>
#include <QSizePolicy>
#include <QString>
#include <QVariant>
#include <QVariantAnimation>
#include <QtGlobal>

#include "ShadIcon.h"
#include "ShadSpinnerPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadSpinner::ShadSpinner(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadSpinnerPrivate())
{
    Q_D(ShadSpinner);
    d->q_ptr = this;
    // 对应 role="status" aria-label="Loading"
    ShadStyleHelper::applyDefaultText(this, "accessibleName", accessibleName(), tr("加载中"), [this](const QString& text) { setAccessibleName(text); });
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFixedSize(d->_size, d->_size);

    d->_animation = new QVariantAnimation(this);
    d->_animation->setStartValue(0.0);
    d->_animation->setEndValue(360.0);
    d->_animation->setDuration(ShadStyleHelper::kSpinDurationMs);
    d->_animation->setLoopCount(-1);
    d->_animation->setEasingCurve(QEasingCurve::Linear);
    connect(d->_animation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_angle = value.toReal();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadSpinner::~ShadSpinner()
{
    Q_D(ShadSpinner);
    // 私有数据先于子对象释放，断开捕获 d 的连接
    QObject::disconnect(d->_animation, nullptr, this, nullptr);
}

void ShadSpinner::setShadIcon(ShadIconType::IconName icon)
{
    Q_D(ShadSpinner);
    if (d->_icon == icon)
    {
        return;
    }
    d->_icon = icon;
    d->updateAnimation();
    update();
}

ShadIconType::IconName ShadSpinner::getShadIcon() const
{
    Q_D(const ShadSpinner);
    return d->_icon;
}

void ShadSpinner::setSpinnerSize(int size)
{
    Q_D(ShadSpinner);
    const int spinnerSize = qMax(1, size);
    if (d->_size == spinnerSize)
    {
        return;
    }
    d->_size = spinnerSize;
    setFixedSize(spinnerSize, spinnerSize);
    update();
}

int ShadSpinner::getSpinnerSize() const
{
    Q_D(const ShadSpinner);
    return d->_size;
}

void ShadSpinner::setColorRole(ShadThemeType::ThemeColor colorRole)
{
    Q_D(ShadSpinner);
    d->_colorRole = colorRole;
    update();
}

ShadThemeType::ThemeColor ShadSpinner::getColorRole() const
{
    Q_D(const ShadSpinner);
    return d->_colorRole;
}

void ShadSpinner::setColor(const QColor& color)
{
    Q_D(ShadSpinner);
    d->_color = color;
    update();
}

QColor ShadSpinner::getColor() const
{
    Q_D(const ShadSpinner);
    return d->_color;
}

void ShadSpinner::setRunning(bool running)
{
    Q_D(ShadSpinner);
    if (d->_isRunning == running)
    {
        return;
    }
    d->_isRunning = running;
    d->updateAnimation();
}

bool ShadSpinner::isRunning() const
{
    Q_D(const ShadSpinner);
    return d->_isRunning;
}

QSize ShadSpinner::sizeHint() const
{
    Q_D(const ShadSpinner);
    return QSize(d->_size, d->_size);
}

void ShadSpinner::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange)
    {
        update();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        ShadStyleHelper::applyDefaultText(this, "accessibleName", accessibleName(), tr("加载中"), [this](const QString& text) { setAccessibleName(text); });
    }
    QWidget::changeEvent(event);
}

void ShadSpinner::showEvent(QShowEvent* event)
{
    Q_D(ShadSpinner);
    QWidget::showEvent(event);
    d->updateAnimation();
}

void ShadSpinner::hideEvent(QHideEvent* event)
{
    Q_D(ShadSpinner);
    QWidget::hideEvent(event);
    d->updateAnimation();
}

void ShadSpinner::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadSpinner);
    if (d->_icon == ShadIconType::None)
    {
        return;
    }
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
    const QRectF iconRect(rect());
    const QPointF center = iconRect.center();
    painter.translate(center);
    painter.rotate(d->_angle);
    painter.translate(-center);
    ShadIcon::paint(&painter, iconRect, d->_icon, d->iconColor(), d->_size);
}
