#include "ShadNotificationPrivate.h"

#include <QAbstractAnimation>
#include <QApplication>
#include <QFontMetrics>
#include <QHash>
#include <QVariant>
#include <QVariantAnimation>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadNotification.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    QHash<QWidget*, QList<ShadNotification*>>& registry()
    {
        static QHash<QWidget*, QList<ShadNotification*>> instance;
        return instance;
    }

    QWidget* resolveHost(QWidget* parent)
    {
        return parent != nullptr ? parent : QApplication::activeWindow();
    }
}

ShadNotificationPrivate::ShadNotificationPrivate()
{
}

ShadNotificationPrivate::~ShadNotificationPrivate()
{
}

void ShadNotificationPrivate::notify(ShadNotificationType::Type type, const QString& title, const QString& text, int duration, ShadNotificationType::Position position, QWidget* parent)
{
    QWidget* host = resolveHost(parent);
    if (host == nullptr)
    {
        qWarning("ShadNotification 没有可显示的窗口");
        return;
    }
    new ShadNotification(type, position, title, text, duration, host);
}

void ShadNotificationPrivate::closeAll(QWidget* parent)
{
    QWidget* host = resolveHost(parent);
    if (host == nullptr)
    {
        return;
    }
    const QList<ShadNotification*> notifications = registry().value(host);
    for (ShadNotification* notification : notifications)
    {
        notification->d_func()->close();
    }
}

void ShadNotificationPrivate::repositionStack(QWidget* host, ShadNotificationType::Position position)
{
    const QList<ShadNotification*> notifications = registry().value(host);
    for (ShadNotification* notification : notifications)
    {
        ShadNotificationPrivate* d = notification->d_func();
        if (d->_position == position && d->_isPlaced)
        {
            d->moveTo(d->widgetPos(d->logicalCardRect()), kMoveDuration);
        }
    }
}

void ShadNotificationPrivate::repositionHost(QWidget* host)
{
    const QList<ShadNotification*> notifications = registry().value(host);
    for (ShadNotification* notification : notifications)
    {
        notification->d_func()->updateSize();
    }
    for (ShadNotification* notification : notifications)
    {
        ShadNotificationPrivate* d = notification->d_func();
        if (d->_isPlaced)
        {
            d->moveTo(d->widgetPos(d->logicalCardRect()), 0);
        }
    }
}

bool ShadNotificationPrivate::isFirstOnHost(const ShadNotification* notification)
{
    const QList<ShadNotification*> notifications = registry().value(notification->d_func()->_host);
    return !notifications.isEmpty() && notifications.first() == notification;
}

int ShadNotificationPrivate::cardRadius()
{
    return shadTheme->getBaseRadius() + 2;
}

void ShadNotificationPrivate::start()
{
    Q_Q(ShadNotification);
    registry()[_host].append(q);
    updateSize();
    QList<ShadNotification*> notifications = stack();
    while (notifications.size() > 1 && isOverflowing())
    {
        notifications.first()->d_func()->close();
        notifications = stack();
    }
    q->move(enterStartPos());
    _isPlaced = true;
    const bool isFade = isFadePosition();
    if (isFade)
    {
        _opacity = 0.0;
        _opacityAnimation->setDuration(kFadeEnterDuration);
        _opacityAnimation->setStartValue(0.0);
        _opacityAnimation->setEndValue(1.0);
        _opacityAnimation->start();
    }
    updateCloseButton();
    q->show();
    q->raise();
    moveTo(widgetPos(logicalCardRect()), isFade ? kFadeEnterDuration : kEnterDuration);
}

void ShadNotificationPrivate::close()
{
    if (_isClosing)
    {
        return;
    }
    _isClosing = true;
    _countdownAnimation->stop();
    _moveAnimation->stop();
    unregister();
    updateCloseButton();
    repositionStack(_host, _position);
    _opacityAnimation->stop();
    _opacityAnimation->setDuration(kExitDuration);
    _opacityAnimation->setStartValue(_opacity);
    _opacityAnimation->setEndValue(0.0);
    _opacityAnimation->start();
}

void ShadNotificationPrivate::unregister()
{
    Q_Q(ShadNotification);
    QHash<QWidget*, QList<ShadNotification*>>& notifications = registry();
    const QHash<QWidget*, QList<ShadNotification*>>::iterator it = notifications.find(_host);
    if (it == notifications.end())
    {
        return;
    }
    it->removeOne(q);
    if (it->isEmpty())
    {
        notifications.erase(it);
    }
}

void ShadNotificationPrivate::updateSize()
{
    Q_Q(ShadNotification);
    _cardSize = computeCardSize();
    q->resize(_cardSize.width() + 2 * kShadowMargin, _cardSize.height() + 2 * kShadowMargin);
    layoutCloseButton();
}

void ShadNotificationPrivate::layoutCloseButton()
{
    const ContentLayout layout = computeGeometry();
    const int margin = ShadStyleHelper::kFocusRingWidth;
    _closeButton->setGeometry(layout.closeRect.adjusted(-margin, -margin, margin, margin).toRect());
}

void ShadNotificationPrivate::updateCloseButton()
{
    Q_Q(ShadNotification);
    const bool isVisible = !_isClosing && _opacity >= 1.0;
    if (!isVisible && _closeButton->isVisible())
    {
        ShadStyleHelper::clearFocusWithin(q);
    }
    _closeButton->setVisible(isVisible);
}

void ShadNotificationPrivate::moveTo(const QPoint& target, int duration)
{
    Q_Q(ShadNotification);
    _moveAnimation->stop();
    if (duration <= 0 || q->pos() == target)
    {
        q->move(target);
        if (!_isCountdownStarted && !_isClosing)
        {
            startCountdown();
        }
        return;
    }
    _moveAnimation->setDuration(duration);
    _moveAnimation->setStartValue(q->pos());
    _moveAnimation->setEndValue(target);
    _moveAnimation->start();
}

void ShadNotificationPrivate::startCountdown()
{
    _isCountdownStarted = true;
    if (_duration <= 0)
    {
        return;
    }
    _countdownAnimation->setDuration(_duration);
    _countdownAnimation->setStartValue(1.0);
    _countdownAnimation->setEndValue(0.0);
    _countdownAnimation->start();
}

bool ShadNotificationPrivate::isFadePosition() const
{
    return _position == ShadNotificationType::Top || _position == ShadNotificationType::Bottom;
}

bool ShadNotificationPrivate::isBottomPosition() const
{
    return _position == ShadNotificationType::Bottom || _position == ShadNotificationType::BottomLeft || _position == ShadNotificationType::BottomRight;
}

bool ShadNotificationPrivate::isMiddlePosition() const
{
    return _position == ShadNotificationType::Left || _position == ShadNotificationType::Right;
}

bool ShadNotificationPrivate::isStartSide() const
{
    return _position == ShadNotificationType::Left || _position == ShadNotificationType::TopLeft || _position == ShadNotificationType::BottomLeft;
}

bool ShadNotificationPrivate::isEndSide() const
{
    return _position == ShadNotificationType::Right || _position == ShadNotificationType::TopRight || _position == ShadNotificationType::BottomRight;
}

int ShadNotificationPrivate::titleBarInset() const
{
    const QVariant value = _host->property("titleBarHeight");
    return value.isValid() ? qMax(0, value.toInt()) : 0;
}

QRect ShadNotificationPrivate::availableRect() const
{
    const int top = qMin(titleBarInset(), _host->height());
    return QRect(0, top, _host->width(), _host->height() - top);
}

int ShadNotificationPrivate::titleWidth() const
{
    if (_title.isEmpty())
    {
        return 0;
    }
    return qMin(ShadStyleHelper::textWidth(_titleFont, _title) + 1, kMaxTitleWidth);
}

int ShadNotificationPrivate::textHeight(int width) const
{
    if (_text.isEmpty())
    {
        return 0;
    }
    const QRect bounds = QFontMetrics(_textFont).boundingRect(QRect(0, 0, qMax(1, width), 100000), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, _text);
    return bounds.height();
}

QSize ShadNotificationPrivate::computeCardSize() const
{
    const int title = titleWidth();
    const int titleGap = (title > 0 && !_text.isEmpty()) ? kTextGap : 0;
    const int fixedWidth = 2 * kPaddingX + kIconSize + kContentGap + title + titleGap + kContentGap + kButtonSize;
    const int naturalTextWidth = _text.isEmpty() ? 0 : ShadStyleHelper::textWidth(_textFont, _text) + 1;
    const int maxWidth = qMax(fixedWidth, qMin(kMaxCardWidth, availableRect().width() - 2 * kViewportOffset));
    const int width = qMin(fixedWidth + naturalTextWidth, maxWidth);
    const int blockHeight = qMax(kLineHeight, textHeight(width - fixedWidth));
    return QSize(width, qMax(kButtonSize, blockHeight) + 2 * kPaddingY);
}

QList<ShadNotification*> ShadNotificationPrivate::stack() const
{
    QList<ShadNotification*> result;
    const QList<ShadNotification*> notifications = registry().value(_host);
    for (ShadNotification* notification : notifications)
    {
        if (notification->d_func()->_position == _position)
        {
            result.append(notification);
        }
    }
    return result;
}

QRect ShadNotificationPrivate::logicalCardRect() const
{
    Q_Q(const ShadNotification);
    const QList<ShadNotification*> notifications = stack();
    const QRect area = availableRect();
    const int width = _cardSize.width();
    const int height = _cardSize.height();
    const int firstHeight = notifications.isEmpty() ? height : notifications.first()->d_func()->_cardSize.height();
    int offset = 0;
    for (const ShadNotification* notification : notifications)
    {
        if (notification == q)
        {
            break;
        }
        offset += notification->d_func()->_cardSize.height() + kStackGap;
    }
    int x = area.left() + (area.width() - width) / 2;
    if (isStartSide())
    {
        x = area.left() + kViewportOffset;
    }
    else if (isEndSide())
    {
        x = area.right() + 1 - kViewportOffset - width;
    }
    int y = area.top() + kViewportOffset + offset;
    if (isBottomPosition())
    {
        y = area.bottom() + 1 - kViewportOffset - offset - height;
    }
    else if (isMiddlePosition())
    {
        y = area.top() + area.height() / 2 - firstHeight / 2 + offset;
    }
    return QRect(x, y, width, height);
}

QRect ShadNotificationPrivate::visualCardRect(const QRect& logicalRect) const
{
    return ShadStyleHelper::visualRect(_host->layoutDirection(), _host->rect(), logicalRect);
}

QPoint ShadNotificationPrivate::widgetPos(const QRect& logicalRect) const
{
    return visualCardRect(logicalRect).topLeft() - QPoint(kShadowMargin, kShadowMargin);
}

QPoint ShadNotificationPrivate::enterStartPos() const
{
    QRect rect = logicalCardRect();
    if (isFadePosition())
    {
        rect.translate(0, isBottomPosition() ? kSlideDistance : -kSlideDistance);
    }
    else if (isStartSide())
    {
        rect.moveLeft(-rect.width() - kShadowMargin);
    }
    else
    {
        rect.moveLeft(_host->width() + kShadowMargin);
    }
    return widgetPos(rect);
}

bool ShadNotificationPrivate::isOverflowing() const
{
    const QRect rect = logicalCardRect();
    const QRect area = availableRect();
    if (isBottomPosition())
    {
        return rect.top() < area.top() + kViewportOffset;
    }
    return rect.bottom() > area.bottom() - kViewportOffset;
}

ShadNotificationPrivate::ContentLayout ShadNotificationPrivate::computeGeometry() const
{
    Q_Q(const ShadNotification);
    ContentLayout layout;
    layout.cardRect = QRectF(kShadowMargin, kShadowMargin, _cardSize.width(), _cardSize.height());
    const QRectF card = layout.cardRect;
    const int title = titleWidth();
    const int titleGap = (title > 0 && !_text.isEmpty()) ? kTextGap : 0;
    const qreal left = card.left() + kPaddingX;
    const qreal centerY = card.center().y();
    layout.closeRect = QRectF(card.right() - kPaddingX - kButtonSize, centerY - kButtonSize / 2.0, kButtonSize, kButtonSize);
    const qreal textLeft = left + kIconSize + kContentGap + title + titleGap;
    const qreal textWidth = qMax<qreal>(0.0, layout.closeRect.left() - kContentGap - textLeft);
    const int lineHeight = QFontMetrics(_titleFont).height();
    const int blockHeight = qMax(kLineHeight, textHeight(qRound(textWidth)));
    const qreal top = centerY - blockHeight / 2.0;
    layout.iconRect = QRectF(left, top + (lineHeight - kIconSize) / 2.0, kIconSize, kIconSize);
    layout.titleRect = QRectF(left + kIconSize + kContentGap, top, title, lineHeight);
    layout.textRect = QRectF(textLeft, top, textWidth, blockHeight);
    if (q->layoutDirection() == Qt::RightToLeft)
    {
        const QRectF bounds(0, 0, q->width(), q->height());
        layout.iconRect = ShadStyleHelper::visualRect(Qt::RightToLeft, bounds, layout.iconRect);
        layout.titleRect = ShadStyleHelper::visualRect(Qt::RightToLeft, bounds, layout.titleRect);
        layout.textRect = ShadStyleHelper::visualRect(Qt::RightToLeft, bounds, layout.textRect);
        layout.closeRect = ShadStyleHelper::visualRect(Qt::RightToLeft, bounds, layout.closeRect);
    }
    return layout;
}

QColor ShadNotificationPrivate::statusColor() const
{
    const bool isDark = shadTheme->isDark();
    switch (_type)
    {
        case ShadNotificationType::Success:
            return isDark ? QColor(0x4a, 0xde, 0x80) : QColor(0x16, 0xa3, 0x4a);
        case ShadNotificationType::Info:
            return isDark ? QColor(0x60, 0xa5, 0xfa) : QColor(0x25, 0x63, 0xeb);
        case ShadNotificationType::Warning:
            return isDark ? QColor(0xfb, 0xbf, 0x24) : QColor(0xd9, 0x77, 0x06);
        case ShadNotificationType::Error:
            return shadTheme->getColor(ShadThemeType::Destructive);
    }
    return shadTheme->getColor(ShadThemeType::Primary);
}

ShadIconType::IconName ShadNotificationPrivate::statusIcon() const
{
    switch (_type)
    {
        case ShadNotificationType::Success:
            return ShadIconType::CircleCheck;
        case ShadNotificationType::Info:
            return ShadIconType::Info;
        case ShadNotificationType::Warning:
            return ShadIconType::TriangleAlert;
        case ShadNotificationType::Error:
            return ShadIconType::OctagonX;
    }
    return ShadIconType::Info;
}
