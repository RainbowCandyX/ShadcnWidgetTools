#include "ShadLoadingOverlay.h"

#include <QApplication>
#include <QEasingCurve>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadLabel.h"
#include "ShadLoadingOverlayPrivate.h"
#include "ShadSpinner.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadLoadingOverlay::ShadLoadingOverlay(QWidget* target)
    : QWidget(target), d_ptr(new ShadLoadingOverlayPrivate())
{
    Q_D(ShadLoadingOverlay);
    d->q_ptr = this;
    setFocusPolicy(Qt::StrongFocus);

    d->_spinner = new ShadSpinner(this);
    d->_spinner->setSpinnerSize(ShadLoadingOverlayPrivate::kDefaultSpinnerSize);
    d->_label = new ShadLabel(this);
    d->_label->setColorRole(ShadThemeType::MutedForeground);
    d->_label->setAlignment(Qt::AlignCenter);
    d->_label->hide();
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(ShadLoadingOverlayPrivate::kSpacing);
    layout->addStretch(1);
    layout->addWidget(d->_spinner, 0, Qt::AlignHCenter);
    layout->addWidget(d->_label, 0, Qt::AlignHCenter);
    layout->addStretch(1);

    d->_opacityEffect = new QGraphicsOpacityEffect(this);
    d->_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(d->_opacityEffect);
    d->_fadeAnimation = new QVariantAnimation(this);
    d->_fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_fadeAnimation, &QVariantAnimation::valueChanged, this, [d](const QVariant& value) { d->_opacityEffect->setOpacity(value.toReal()); });
    connect(d->_fadeAnimation, &QVariantAnimation::finished, this, [this, d]()
    {
        if (!d->_isShown)
        {
            hide();
        }
    });
    d->_delayTimer = new QTimer(this);
    d->_delayTimer->setSingleShot(true);
    connect(d->_delayTimer, &QTimer::timeout, this, [d]() { d->setShown(true); });
    connect(qApp, &QApplication::focusChanged, this, [d](QWidget* old, QWidget* now)
    {
        Q_UNUSED(old);
        d->handleFocusChanged(now);
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    hide();
    if (target != nullptr)
    {
        d->attach(target);
    }
}

ShadLoadingOverlay::~ShadLoadingOverlay()
{
    Q_D(ShadLoadingOverlay);
    d->_fadeAnimation->stop();
    d->_delayTimer->stop();
    QObject::disconnect(d->_fadeAnimation, nullptr, this, nullptr);
    QObject::disconnect(d->_delayTimer, nullptr, this, nullptr);
    if (qApp != nullptr)
    {
        QObject::disconnect(qApp, nullptr, this, nullptr);
    }
}

void ShadLoadingOverlay::setTarget(QWidget* target)
{
    Q_D(ShadLoadingOverlay);
    if (d->_target == target)
    {
        return;
    }
    d->detach();
    if (target == nullptr)
    {
        hide();
        setParent(nullptr);
        return;
    }
    d->attach(target);
}

QWidget* ShadLoadingOverlay::getTarget() const
{
    Q_D(const ShadLoadingOverlay);
    return d->_target.data();
}

bool ShadLoadingOverlay::isLoading() const
{
    Q_D(const ShadLoadingOverlay);
    return d->_isLoading;
}

void ShadLoadingOverlay::setText(const QString& text)
{
    Q_D(ShadLoadingOverlay);
    d->_label->setText(text);
    d->_label->setVisible(!text.isEmpty());
}

QString ShadLoadingOverlay::getText() const
{
    Q_D(const ShadLoadingOverlay);
    return d->_label->text();
}

void ShadLoadingOverlay::setDelay(int delayMs)
{
    Q_D(ShadLoadingOverlay);
    d->_delayMs = qMax(0, delayMs);
}

int ShadLoadingOverlay::getDelay() const
{
    Q_D(const ShadLoadingOverlay);
    return d->_delayMs;
}

void ShadLoadingOverlay::setRadius(int radius)
{
    Q_D(ShadLoadingOverlay);
    d->_radius = qMax(0, radius);
    update();
}

int ShadLoadingOverlay::getRadius() const
{
    Q_D(const ShadLoadingOverlay);
    return d->_radius;
}

void ShadLoadingOverlay::setSpinnerSize(int size)
{
    Q_D(ShadLoadingOverlay);
    d->_spinner->setSpinnerSize(size);
}

int ShadLoadingOverlay::getSpinnerSize() const
{
    Q_D(const ShadLoadingOverlay);
    return d->_spinner->getSpinnerSize();
}

void ShadLoadingOverlay::setLoading(bool loading)
{
    Q_D(ShadLoadingOverlay);
    if (d->_isLoading == loading)
    {
        return;
    }
    d->_isLoading = loading;
    d->_delayTimer->stop();
    if (loading && d->_delayMs > 0)
    {
        d->_delayTimer->start(d->_delayMs);
    }
    else
    {
        d->setShown(loading);
    }
    Q_EMIT loadingChanged(loading);
}

void ShadLoadingOverlay::start()
{
    setLoading(true);
}

void ShadLoadingOverlay::stop()
{
    setLoading(false);
}

bool ShadLoadingOverlay::event(QEvent* event)
{
    switch (event->type())
    {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::Wheel:
        case QEvent::ContextMenu:
        case QEvent::KeyRelease:
            event->accept();
            return true;
        case QEvent::KeyPress:
        {
            const int key = static_cast<QKeyEvent*>(event)->key();
            if (key == Qt::Key_Tab || key == Qt::Key_Backtab)
            {
                break;
            }
            event->accept();
            return true;
        }
        default:
            break;
    }
    return QWidget::event(event);
}

bool ShadLoadingOverlay::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadLoadingOverlay);
    if (watched == d->_target)
    {
        if (event->type() == QEvent::Resize)
        {
            d->syncGeometry();
        }
        else if (event->type() == QEvent::ChildAdded && d->_isShown)
        {
            raise();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadLoadingOverlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadLoadingOverlay);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    ShadStyleHelper::fillRoundedRect(&painter, QRectF(rect()), d->_radius, ShadStyleHelper::withAlpha(shadTheme->getColor(ShadThemeType::Background), ShadLoadingOverlayPrivate::kMaskOpacity));
}
