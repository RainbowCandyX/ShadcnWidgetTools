#include "ShadNavigationMenuViewport.h"

#include <QEasingCurve>
#include <QEvent>
#include <QFont>
#include <QGuiApplication>
#include <QHideEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QScreen>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>

#include "ShadPopupEffect.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kShadowMargin = 12;
    constexpr int kBorderWidth = 1;
    // p-2 pr-2.5
    constexpr int kPaddingLeft = 8;
    constexpr int kPaddingRight = 10;
    constexpr int kPaddingY = 8;
    // mt-1.5
    constexpr int kSideOffset = 6;
    constexpr int kScreenMargin = 8;
    constexpr int kAnimationDurationMs = 150;
    // zoom-in-90
    constexpr qreal kOpenScale = 0.9;
    // slide-in-from-right-52
    constexpr int kSlideDistance = 208;
} // namespace

ShadNavigationMenuViewport::ShadNavigationMenuViewport(QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    new ShadPopupEffect(this, [this]() { return cardRect(); });

    _body = new QWidget(this);
    _body->setMouseTracking(true);
    _body->installEventFilter(this);

    QEasingCurve easingCurve(QEasingCurve::BezierSpline);
    easingCurve.addCubicBezierSegment(QPointF(0.25, 0.1), QPointF(0.25, 1.0), QPointF(1.0, 1.0));
    _openAnimation = new QVariantAnimation(this);
    _openAnimation->setDuration(kAnimationDurationMs);
    _openAnimation->setStartValue(0.0);
    _openAnimation->setEndValue(1.0);
    _openAnimation->setEasingCurve(easingCurve);
    connect(_openAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        _openProgress = value.toReal();
        setWindowOpacity(_openProgress);
        update();
    });
    connect(_openAnimation, &QVariantAnimation::finished, this, [this]() { finishAnimations(); });

    _switchAnimation = new QVariantAnimation(this);
    _switchAnimation->setDuration(kAnimationDurationMs);
    _switchAnimation->setStartValue(0.0);
    _switchAnimation->setEndValue(1.0);
    _switchAnimation->setEasingCurve(easingCurve);
    connect(_switchAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        _switchProgress = value.toReal();
        if (!_content.isNull())
        {
            _content->setGeometry(contentRect(qRound(_switchSign * kSlideDistance * (1.0 - _switchProgress))));
        }
        _body->update();
    });
    connect(_switchAnimation, &QVariantAnimation::finished, this, [this]() { finishAnimations(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadNavigationMenuViewport::~ShadNavigationMenuViewport()
{
}

QWidget* ShadNavigationMenuViewport::body() const
{
    return _body;
}

QWidget* ShadNavigationMenuViewport::content() const
{
    return _content.data();
}

QRect ShadNavigationMenuViewport::cardRect() const
{
    return rect().adjusted(kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);
}

void ShadNavigationMenuViewport::showContent(QWidget* content, int step, const QRect& barRect, bool focusContent)
{
    if (content == nullptr)
    {
        return;
    }
    const bool wasVisible = isVisible();
    finishAnimations();
    QWidget* previous = _content.data();
    if (wasVisible && previous == content)
    {
        if (focusContent)
        {
            focusFirstChild();
        }
        return;
    }
    if (wasVisible && previous != nullptr)
    {
        _outgoingPixmap = ShadStyleHelper::grabContent(previous);
        _outgoingRect = previous->geometry();
    }
    if (previous != nullptr && previous != content)
    {
        previous->hide();
    }
    if (content->parentWidget() != _body)
    {
        content->setParent(_body);
    }
    _content = content;

    const QSize card = cardSize(content);
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    QPoint topLeft(isRightToLeft ? barRect.right() + 1 - card.width() : barRect.left(), barRect.bottom() + 1 + kSideOffset);
    if (const QScreen* screen = QGuiApplication::screenAt(barRect.center()))
    {
        const QRect available = screen->availableGeometry();
        topLeft.setX(qBound(available.left() + kScreenMargin, topLeft.x(), available.right() + 1 - kScreenMargin - card.width()));
    }
    setGeometry(QRect(topLeft, card).adjusted(-kShadowMargin, -kShadowMargin, kShadowMargin, kShadowMargin));
    _body->setGeometry(cardRect().adjusted(kBorderWidth, kBorderWidth, -kBorderWidth, -kBorderWidth));

    if (!wasVisible)
    {
        content->setGeometry(contentRect(0));
        content->show();
        _openProgress = 0.0;
        setWindowOpacity(0.0);
        setAttribute(Qt::WA_NoMouseReplay, false);
        show();
        raise();
        setFocus(Qt::PopupFocusReason);
        // 缩放期间绘制快照
        _openPixmap = ShadStyleHelper::grabContent(_body);
        _body->hide();
        _isFocusPending = focusContent;
        _openAnimation->start();
        return;
    }
    _switchSign = step >= 0 ? 1 : -1;
    if (isRightToLeft)
    {
        _switchSign = -_switchSign;
    }
    _switchProgress = 0.0;
    content->setGeometry(contentRect(_switchSign * kSlideDistance));
    content->show();
    if (focusContent)
    {
        focusFirstChild();
    }
    update();
    _switchAnimation->start();
}

bool ShadNavigationMenuViewport::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _body && event->type() == QEvent::Paint && !_outgoingPixmap.isNull())
    {
        const int offset = qRound(-_switchSign * kSlideDistance * _switchProgress);
        QPainter painter(_body);
        painter.setOpacity(1.0 - _switchProgress);
        painter.drawPixmap(_outgoingRect.topLeft() + QPoint(offset, 0), _outgoingPixmap);
    }
    return QWidget::eventFilter(watched, event);
}

void ShadNavigationMenuViewport::mousePressEvent(QMouseEvent* event)
{
    if (!cardRect().contains(event->position().toPoint()))
    {
        hide();
    }
    event->accept();
}

void ShadNavigationMenuViewport::hideEvent(QHideEvent* event)
{
    finishAnimations();
    Q_EMIT closed();
    QWidget::hideEvent(event);
}

void ShadNavigationMenuViewport::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF card = cardRect();
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    if (_openProgress < 1.0)
    {
        // origin-top-center
        const qreal scale = kOpenScale + (1.0 - kOpenScale) * _openProgress;
        const QPointF origin(card.center().x(), card.top());
        painter.translate(origin);
        painter.scale(scale, scale);
        painter.translate(-origin);
    }
    // rounded-md border bg-popover shadow
    drawShadowSm(&painter, card, radius);
    ShadPopupEffect::paintBackdrop(&painter, this, card, radius);
    fillRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Popover));
    strokeRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Border));
    if (!_openPixmap.isNull())
    {
        painter.drawPixmap(_body->geometry().topLeft(), _openPixmap);
    }
}

QSize ShadNavigationMenuViewport::contentSize(QWidget* content) const
{
    QSize size = content->sizeHint().expandedTo(content->minimumSizeHint());
    if (content->hasHeightForWidth())
    {
        size.setHeight(qMax(content->minimumSizeHint().height(), content->heightForWidth(size.width())));
    }
    return size.expandedTo(content->minimumSize()).boundedTo(content->maximumSize());
}

QSize ShadNavigationMenuViewport::cardSize(QWidget* content) const
{
    return contentSize(content) + QSize(kPaddingLeft + kPaddingRight + 2 * kBorderWidth, 2 * kPaddingY + 2 * kBorderWidth);
}

QRect ShadNavigationMenuViewport::contentRect(int offset) const
{
    if (_content.isNull())
    {
        return QRect();
    }
    return QRect(QPoint(kPaddingLeft + offset, kPaddingY), contentSize(_content.data()));
}

void ShadNavigationMenuViewport::finishAnimations()
{
    _openAnimation->stop();
    _switchAnimation->stop();
    const bool wasOpening = _openProgress < 1.0;
    _openProgress = 1.0;
    _switchProgress = 1.0;
    _openPixmap = QPixmap();
    _outgoingPixmap = QPixmap();
    if (wasOpening && isVisible())
    {
        setWindowOpacity(1.0);
    }
    _body->show();
    if (!_content.isNull())
    {
        _content->setGeometry(contentRect(0));
    }
    if (_isFocusPending && isVisible())
    {
        focusFirstChild();
    }
    _isFocusPending = false;
    update();
}

void ShadNavigationMenuViewport::focusFirstChild()
{
    QWidget* candidate = _body->nextInFocusChain();
    while (candidate != nullptr && candidate != _body)
    {
        const bool isTabFocus = (candidate->focusPolicy() & Qt::TabFocus) == Qt::TabFocus;
        if (_body->isAncestorOf(candidate) && candidate->isVisibleTo(this) && candidate->isEnabled() && isTabFocus)
        {
            candidate->setFocus(Qt::PopupFocusReason);
            return;
        }
        candidate = candidate->nextInFocusChain();
    }
    setFocus(Qt::PopupFocusReason);
}
