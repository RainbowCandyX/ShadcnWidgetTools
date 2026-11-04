#include "ShadNotification.h"

#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QFont>
#include <QMetaObject>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QResizeEvent>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadNotificationPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadNotification::ShadNotification(ShadNotificationType::Type type, ShadNotificationType::Position position, const QString& title, const QString& text, int duration, QWidget* parent)
    : QWidget(parent), d_ptr(new ShadNotificationPrivate())
{
    Q_D(ShadNotification);
    d->q_ptr = this;
    d->_host = parent;
    d->_type = type;
    d->_position = position;
    d->_title = title;
    d->_text = text;
    d->_duration = qMax(0, duration);
    hide();
    d->_titleFont = font();
    d->_titleFont.setPixelSize(14);
    d->_titleFont.setWeight(QFont::Medium);
    d->_textFont = font();
    d->_textFont.setPixelSize(14);
    d->_textFont.setWeight(QFont::Normal);

    d->_closeButton = new ShadButton(QString(), ShadButtonType::Ghost, this);
    d->_closeButton->setButtonSize(ShadButtonType::IconSmall);
    d->_closeButton->setShadIcon(ShadIconType::X);
    d->_closeButton->setToolTip(tr("关闭"));
    connect(d->_closeButton, &ShadButton::clicked, this, [d]() { d->close(); });

    d->_moveAnimation = new QVariantAnimation(this);
    d->_moveAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_moveAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) { move(value.toPoint()); });
    connect(d->_moveAnimation, &QVariantAnimation::finished, this, [d]()
    {
        if (!d->_isCountdownStarted && !d->_isClosing)
        {
            d->startCountdown();
        }
    });

    d->_opacityAnimation = new QVariantAnimation(this);
    d->_opacityAnimation->setEasingCurve(QEasingCurve::InOutSine);
    connect(d->_opacityAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_opacity = value.toReal();
        d->updateCloseButton();
        update();
    });
    connect(d->_opacityAnimation, &QVariantAnimation::finished, this, [this, d]()
    {
        if (d->_isClosing)
        {
            hide();
            deleteLater();
            return;
        }
        d->updateCloseButton();
    });

    d->_countdownAnimation = new QVariantAnimation(this);
    d->_countdownAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(d->_countdownAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_countdownProgress = value.toReal();
        update();
    });
    connect(d->_countdownAnimation, &QVariantAnimation::finished, this, [d]() { d->close(); });

    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    parent->installEventFilter(this);
    d->start();
}

ShadNotification::~ShadNotification()
{
    Q_D(ShadNotification);
    d->unregister();
}

void ShadNotification::info(const QString& title, const QString& text, int duration, ShadNotificationType::Position position, QWidget* parent)
{
    ShadNotificationPrivate::notify(ShadNotificationType::Info, title, text, duration, position, parent);
}

void ShadNotification::success(const QString& title, const QString& text, int duration, ShadNotificationType::Position position, QWidget* parent)
{
    ShadNotificationPrivate::notify(ShadNotificationType::Success, title, text, duration, position, parent);
}

void ShadNotification::warning(const QString& title, const QString& text, int duration, ShadNotificationType::Position position, QWidget* parent)
{
    ShadNotificationPrivate::notify(ShadNotificationType::Warning, title, text, duration, position, parent);
}

void ShadNotification::error(const QString& title, const QString& text, int duration, ShadNotificationType::Position position, QWidget* parent)
{
    ShadNotificationPrivate::notify(ShadNotificationType::Error, title, text, duration, position, parent);
}

void ShadNotification::closeAll(QWidget* parent)
{
    ShadNotificationPrivate::closeAll(parent);
}

bool ShadNotification::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadNotification);
    if (watched == d->_host && !d->_isClosing)
    {
        if ((event->type() == QEvent::Resize || event->type() == QEvent::LayoutDirectionChange) && ShadNotificationPrivate::isFirstOnHost(this))
        {
            ShadNotificationPrivate::repositionHost(d->_host);
        }
        else if (event->type() == QEvent::ChildAdded)
        {
            QMetaObject::invokeMethod(this, [this]() { raise(); }, Qt::QueuedConnection);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadNotification::changeEvent(QEvent* event)
{
    Q_D(ShadNotification);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->layoutCloseButton();
        update();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        d->_closeButton->setToolTip(tr("关闭"));
    }
    QWidget::changeEvent(event);
}

void ShadNotification::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadNotification);
    QWidget::resizeEvent(event);
    d->layoutCloseButton();
}

void ShadNotification::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadNotification);
    if (d->_opacity <= 0.0)
    {
        return;
    }
    const ShadNotificationPrivate::ContentLayout layout = d->computeGeometry();
    const qreal radius = qMin<qreal>(ShadNotificationPrivate::cardRadius(), layout.cardRect.height() / 2.0);
    const QColor status = d->statusColor();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(d->_opacity);
    ShadStyleHelper::drawShadowLg(&painter, layout.cardRect, radius);
    ShadStyleHelper::fillRoundedRect(&painter, layout.cardRect, radius, shadTheme->getColor(ShadThemeType::Popover));
    ShadStyleHelper::strokeRoundedRect(&painter, layout.cardRect, radius, shadTheme->getColor(ShadThemeType::Border));
    ShadIcon::paint(&painter, layout.iconRect, d->statusIcon(), status, ShadNotificationPrivate::kIconSize);
    if (!d->_title.isEmpty())
    {
        painter.setFont(d->_titleFont);
        painter.setPen(shadTheme->getColor(ShadThemeType::PopoverForeground));
        painter.drawText(layout.titleRect, ShadStyleHelper::visualAlignment(layoutDirection(), Qt::AlignLeft | Qt::AlignTop), ShadStyleHelper::elideText(d->_titleFont, d->_title, layout.titleRect.width()));
    }
    if (!d->_text.isEmpty())
    {
        painter.setFont(d->_textFont);
        painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
        painter.drawText(layout.textRect, ShadStyleHelper::visualAlignment(layoutDirection(), Qt::AlignLeft | Qt::AlignTop) | Qt::TextWordWrap, d->_text);
    }
    if (d->_duration > 0)
    {
        const QRectF innerRect = layout.cardRect.adjusted(1.0, 1.0, -1.0, -1.0);
        const QRectF logicalBar(innerRect.left(), innerRect.bottom() - ShadNotificationPrivate::kCountdownHeight, innerRect.width() * d->_countdownProgress, ShadNotificationPrivate::kCountdownHeight);
        painter.save();
        painter.setClipPath(ShadStyleHelper::roundedRectPath(innerRect, qMax<qreal>(0.0, radius - 1.0)));
        painter.fillRect(ShadStyleHelper::visualRect(layoutDirection(), layout.cardRect, logicalBar), status);
        painter.restore();
    }
}
