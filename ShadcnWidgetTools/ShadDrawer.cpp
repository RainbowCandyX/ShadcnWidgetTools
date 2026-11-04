#include "ShadDrawer.h"

#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QEasingCurve>
#include <QFont>
#include <QKeyEvent>
#include <QList>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <cmath>

#include "ShadDrawerOverlay.h"
#include "ShadDrawerPrivate.h"
#include "ShadFrameAnimation.h"
#include "ShadLabel.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadDrawer::ShadDrawer(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadDrawerPrivate())
{
    Q_D(ShadDrawer);
    d->q_ptr = this;
    d->_anchor = parent;
    setFocusPolicy(Qt::ClickFocus);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    // 不随父组件显示
    hide();
    if (parent != nullptr)
    {
        connect(parent, &QObject::destroyed, this, [this]() { deleteLater(); });
    }

    d->_body = new QWidget(this);
    d->_bodyLayout = new QVBoxLayout(d->_body);
    d->_bodyLayout->setContentsMargins(0, 0, 0, 0);
    d->_bodyLayout->setSpacing(0);

    d->_header = new QWidget(d->_body);
    d->_headerLayout = new QVBoxLayout(d->_header);
    d->_headerLayout->setContentsMargins(ShadDrawerPrivate::kPadding, ShadDrawerPrivate::kPadding, ShadDrawerPrivate::kPadding, 0);
    d->_headerLayout->setSpacing(ShadDrawerPrivate::kHeaderGap);
    d->_titleLabel = new ShadLabel(d->_header);
    d->_titleLabel->setColorRole(ShadThemeType::Foreground);
    d->_titleLabel->setWordWrap(true);
    ShadStyleHelper::applyFont(d->_titleLabel, 14, QFont::Medium);
    d->_descriptionLabel = new ShadLabel(d->_header);
    d->_descriptionLabel->setColorRole(ShadThemeType::MutedForeground);
    d->_descriptionLabel->setWordWrap(true);
    ShadStyleHelper::applyFont(d->_descriptionLabel, 14, QFont::Normal);
    d->_headerLayout->addWidget(d->_titleLabel);
    d->_headerLayout->addWidget(d->_descriptionLabel);
    d->_bodyLayout->addWidget(d->_header);

    d->_contentLayout = new QVBoxLayout();
    d->_contentLayout->setContentsMargins(ShadDrawerPrivate::kPadding, ShadDrawerPrivate::kPadding, ShadDrawerPrivate::kPadding, ShadDrawerPrivate::kPadding);
    d->_contentLayout->setSpacing(ShadDrawerPrivate::kPadding);
    d->_bodyLayout->addLayout(d->_contentLayout, 1);
    d->_bodyLayout->addStretch(0);

    d->_footer = new QWidget(d->_body);
    d->_footerLayout = new QVBoxLayout(d->_footer);
    d->_footerLayout->setContentsMargins(ShadDrawerPrivate::kPadding, 0, ShadDrawerPrivate::kPadding, ShadDrawerPrivate::kPadding);
    d->_footerLayout->setSpacing(ShadDrawerPrivate::kFooterGap - 2 * ShadStyleHelper::kFocusRingWidth);
    d->_footer->setVisible(false);
    d->_bodyLayout->addWidget(d->_footer);

    QEasingCurve curve(QEasingCurve::BezierSpline);
    curve.addCubicBezierSegment(QPointF(0.22, 1.0), QPointF(0.36, 1.0), QPointF(1.0, 1.0));
    d->_animation = new ShadFrameAnimation(this);
    d->_animation->setEasingCurve(curve);
    connect(d->_animation, &ShadFrameAnimation::valueChanged, this, [d](qreal value)
    {
        d->_offset = value;
        if (d->_isOpening)
        {
            const qreal span = d->_animationStart - d->_animationEnd;
            d->_appear = span > 0.0 ? qBound(0.0, (d->_animationStart - value) / span, 1.0) : 1.0;
        }
        d->refresh();
    });
    connect(d->_animation, &ShadFrameAnimation::finished, this, [this, d]()
    {
        if (d->_isClosing)
        {
            d->finishClose();
            return;
        }
        if (d->_isOpening)
        {
            d->_isOpening = false;
            d->_appear = 1.0;
            d->refresh();
            Q_EMIT openChangeComplete(true);
        }
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });

    d->updateHeader();
}

ShadDrawer::ShadDrawer(const QString& title, const QString& description, QWidget* parent)
    : ShadDrawer(parent)
{
    setTitle(title);
    setDescription(description);
}

ShadDrawer::~ShadDrawer()
{
    Q_D(ShadDrawer);
    // 断开捕获 d 的连接
    QObject::disconnect(d->_animation, nullptr, this, nullptr);
    QWidget* host = d->_host.data();
    if (!isHidden())
    {
        hide();
    }
    d->detachHost();
    if (host != nullptr)
    {
        ShadDrawerPrivate::syncStack(host);
        ShadDrawerPrivate::syncOverlay(host);
    }
}

void ShadDrawer::setTitle(const QString& title)
{
    Q_D(ShadDrawer);
    d->_titleLabel->setText(title);
    d->updateHeader();
}

QString ShadDrawer::getTitle() const
{
    Q_D(const ShadDrawer);
    return d->_titleLabel->text();
}

void ShadDrawer::setDescription(const QString& description)
{
    Q_D(ShadDrawer);
    d->_descriptionLabel->setText(description);
    d->updateHeader();
}

QString ShadDrawer::getDescription() const
{
    Q_D(const ShadDrawer);
    return d->_descriptionLabel->text();
}

void ShadDrawer::setSwipeDirection(ShadDrawerType::SwipeDirection direction)
{
    Q_D(ShadDrawer);
    if (d->_direction == direction)
    {
        return;
    }
    d->_direction = direction;
    d->updateHeaderStyle();
    if (isHidden())
    {
        return;
    }
    d->updateExtent();
    if (d->_isOpen && !d->_isSwiping)
    {
        d->_animation->stop();
        d->_isOpening = false;
        d->_offset = d->restingOffset();
    }
    d->refresh();
}

ShadDrawerType::SwipeDirection ShadDrawer::getSwipeDirection() const
{
    Q_D(const ShadDrawer);
    return d->_direction;
}

void ShadDrawer::setSwipeHandleVisible(bool visible)
{
    Q_D(ShadDrawer);
    if (d->_isHandleVisible == visible)
    {
        return;
    }
    d->_isHandleVisible = visible;
    if (!isHidden())
    {
        d->updateExtent();
        d->refresh();
    }
}

bool ShadDrawer::isSwipeHandleVisible() const
{
    Q_D(const ShadDrawer);
    return d->_isHandleVisible;
}

void ShadDrawer::setModal(bool modal)
{
    Q_D(ShadDrawer);
    d->_isModal = modal;
}

bool ShadDrawer::isModal() const
{
    Q_D(const ShadDrawer);
    return d->_isModal;
}

void ShadDrawer::setPointerDismissalEnabled(bool enabled)
{
    Q_D(ShadDrawer);
    d->_isPointerDismissal = enabled;
}

bool ShadDrawer::isPointerDismissalEnabled() const
{
    Q_D(const ShadDrawer);
    return d->_isPointerDismissal;
}

void ShadDrawer::setPreferredSize(int size)
{
    Q_D(ShadDrawer);
    const int preferredSize = qMax(0, size);
    if (d->_preferredSize == preferredSize)
    {
        return;
    }
    d->_preferredSize = preferredSize;
    if (!isHidden())
    {
        d->updateExtent();
        d->refresh();
    }
}

int ShadDrawer::getPreferredSize() const
{
    Q_D(const ShadDrawer);
    return d->_preferredSize;
}

void ShadDrawer::setSnapPoints(const QList<qreal>& snapPoints)
{
    Q_D(ShadDrawer);
    QList<qreal> points;
    for (qreal point : snapPoints)
    {
        if (point > 0.0 && !points.contains(point))
        {
            points.append(point);
        }
    }
    d->_snapPoints = points;
    if (points.isEmpty())
    {
        d->_snapPoint = 0.0;
    }
    else if (!points.contains(d->_snapPoint))
    {
        d->setCurrentSnapPoint(points.first());
    }
    if (isHidden())
    {
        return;
    }
    d->updateExtent();
    if (d->_isOpen && !d->_isSwiping)
    {
        d->animateTo(d->restingOffset(), ShadDrawerPrivate::kOpenDurationMs);
    }
    d->refresh();
}

QList<qreal> ShadDrawer::getSnapPoints() const
{
    Q_D(const ShadDrawer);
    return d->_snapPoints;
}

void ShadDrawer::setSnapPoint(qreal snapPoint)
{
    Q_D(ShadDrawer);
    if (!d->_snapPoints.contains(snapPoint))
    {
        return;
    }
    d->setCurrentSnapPoint(snapPoint);
    if (!isHidden() && d->_isOpen && !d->_isSwiping)
    {
        d->animateTo(d->restingOffset(), ShadDrawerPrivate::kOpenDurationMs);
    }
}

qreal ShadDrawer::getSnapPoint() const
{
    Q_D(const ShadDrawer);
    return d->_snapPoint;
}

void ShadDrawer::setSequentialSnapEnabled(bool enabled)
{
    Q_D(ShadDrawer);
    d->_isSequentialSnap = enabled;
}

bool ShadDrawer::isSequentialSnapEnabled() const
{
    Q_D(const ShadDrawer);
    return d->_isSequentialSnap;
}

QVBoxLayout* ShadDrawer::contentLayout() const
{
    Q_D(const ShadDrawer);
    return d->_contentLayout;
}

QVBoxLayout* ShadDrawer::footerLayout()
{
    Q_D(ShadDrawer);
    d->_footer->setVisible(true);
    return d->_footerLayout;
}

bool ShadDrawer::isOpen() const
{
    Q_D(const ShadDrawer);
    return d->_isOpen;
}

void ShadDrawer::open()
{
    Q_D(ShadDrawer);
    if (d->_isOpen)
    {
        return;
    }
    if (d->_outsideDismissClock.isValid() && d->_outsideDismissClock.elapsed() < QApplication::doubleClickInterval())
    {
        d->_outsideDismissClock.invalidate();
        return;
    }
    QWidget* host = d->resolveHost();
    if (host == nullptr)
    {
        return;
    }
    const bool isFresh = isHidden() || d->_host != host;
    d->attachHost(host);
    d->_isOpen = true;
    d->_isClosing = false;
    if (isFresh)
    {
        d->_openSerial = ++ShadDrawerPrivate::s_openSerial;
        d->_stackLevel = 0.0;
        d->_bodySnapshot = QPixmap();
        d->_body->show();
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        QWidget* focus = QApplication::focusWidget();
        d->_focusBeforeOpen = focus != nullptr && focus != this && !isAncestorOf(focus) ? focus : nullptr;
        if (d->_isModal && ShadDrawerPrivate::findOverlay(host) == nullptr)
        {
            ShadDrawerOverlay* overlay = new ShadDrawerOverlay(host);
            overlay->refreshBackdrop();
            overlay->show();
        }
        d->updateHeaderStyle();
        d->updateExtent();
        d->_offset = d->closedOffset();
        d->_appear = 0.0;
        d->place();
        show();
    }
    ShadDrawerOverlay* overlay = ShadDrawerPrivate::findOverlay(host);
    if (overlay != nullptr && !d->_overlayConnection)
    {
        d->_overlayConnection = connect(overlay, &ShadDrawerOverlay::pressed, this, [d]()
        {
            if (d->isTopmost() && d->_isPointerDismissal)
            {
                d->requestClose();
            }
        });
    }
    ShadDrawerPrivate::restack(host);
    d->_isOpening = true;
    d->animateTo(d->restingOffset(), ShadDrawerPrivate::kOpenDurationMs);
    d->focusFirstChild();
    Q_EMIT openChanged(true);
}

void ShadDrawer::closeEvent(QCloseEvent* event)
{
    Q_D(ShadDrawer);
    event->ignore();
    d->requestClose();
}

bool ShadDrawer::focusNextPrevChild(bool next)
{
    Q_D(ShadDrawer);
    if (!d->_isOpen || !d->_isModal)
    {
        return QWidget::focusNextPrevChild(next);
    }
    const QList<QWidget*> widgets = d->focusableChildren();
    if (widgets.isEmpty())
    {
        return true;
    }
    const int count = static_cast<int>(widgets.size());
    int index = static_cast<int>(widgets.indexOf(QApplication::focusWidget()));
    if (index < 0)
    {
        index = next ? 0 : count - 1;
    }
    else
    {
        index = (index + (next ? 1 : -1) + count) % count;
    }
    widgets.at(index)->setFocus(next ? Qt::TabFocusReason : Qt::BacktabFocusReason);
    return true;
}

void ShadDrawer::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadDrawer);
    if (event->key() == Qt::Key_Escape && d->_isOpen)
    {
        d->requestClose();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadDrawer::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadDrawer);
    event->accept();
    if (event->button() != Qt::LeftButton || !d->_isOpen || !d->isTopmost())
    {
        return;
    }
    d->_isPressed = true;
    d->_isSwiping = false;
    d->_pressPos = event->globalPosition();
    d->_pressOffset = d->_offset;
    d->_lastOffset = d->_offset;
    d->_velocity = 0.0;
    d->_clock.start();
    d->_lastTime = 0;
    event->accept();
}

void ShadDrawer::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadDrawer);
    if (!d->_isPressed)
    {
        QWidget::mouseMoveEvent(event);
        return;
    }
    const QPointF delta = event->globalPosition() - d->_pressPos;
    qreal distance = 0.0;
    switch (d->_direction)
    {
        case ShadDrawerType::Up:
            distance = -delta.y();
            break;
        case ShadDrawerType::Right:
            distance = delta.x();
            break;
        case ShadDrawerType::Down:
            distance = delta.y();
            break;
        case ShadDrawerType::Left:
            distance = -delta.x();
            break;
    }
    if (!d->_isSwiping)
    {
        if (qAbs(distance) < QApplication::startDragDistance())
        {
            return;
        }
        d->_isSwiping = true;
        d->_animation->stop();
        if (d->_isOpening)
        {
            d->_isOpening = false;
            d->_appear = 1.0;
            Q_EMIT openChangeComplete(true);
        }
        d->_pressPos = event->globalPosition();
        d->_pressOffset = d->_offset;
        d->_lastOffset = d->_offset;
        d->_lastTime = d->_clock.elapsed();
        return;
    }
    qreal offset = d->_pressOffset + distance;
    const qreal minimum = d->minimumOffset();
    if (offset < minimum)
    {
        // 越界阻力
        const qreal over = minimum - offset;
        offset = minimum - ShadDrawerPrivate::kBleed * (1.0 - std::exp(-over / ShadDrawerPrivate::kBleed));
    }
    offset = qMin(offset, d->closedOffset());
    const qint64 now = d->_clock.elapsed();
    const qint64 elapsed = now - d->_lastTime;
    if (elapsed > 0)
    {
        const qreal instant = (offset - d->_lastOffset) / static_cast<qreal>(elapsed);
        d->_velocity = 0.7 * instant + 0.3 * d->_velocity;
        d->_lastOffset = offset;
        d->_lastTime = now;
    }
    d->_offset = offset;
    d->refresh();
}

void ShadDrawer::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadDrawer);
    if (!d->_isPressed || event->button() != Qt::LeftButton)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    d->_isPressed = false;
    if (!d->_isSwiping)
    {
        return;
    }
    d->_isSwiping = false;
    d->settle();
}

void ShadDrawer::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadDrawer);
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    const QRectF panel(d->_panelRect);
    const qreal scale = d->_panelScale;
    const qreal radius = shadTheme->getRadius(ShadThemeType::ExtraLarge) * scale;
    const qreal amount = qMin<qreal>(d->_stackLevel, 1.0);
    const QColor background = shadTheme->getColor(ShadThemeType::Popover);

    if (d->_stackLevel <= 0.0)
    {
        QRectF bleed;
        switch (d->_direction)
        {
            case ShadDrawerType::Up:
                bleed = QRectF(0.0, 0.0, width(), panel.top());
                break;
            case ShadDrawerType::Right:
                bleed = QRectF(panel.right(), 0.0, width() - panel.right(), height());
                break;
            case ShadDrawerType::Down:
                bleed = QRectF(0.0, panel.bottom(), width(), height() - panel.bottom());
                break;
            case ShadDrawerType::Left:
                bleed = QRectF(0.0, 0.0, panel.left(), height());
                break;
        }
        painter.fillRect(bleed, background);
    }

    const QPainterPath path = d->panelPath(panel, radius);
    painter.fillPath(path, background);

    if (!d->_bodySnapshot.isNull() && amount < 1.0)
    {
        painter.save();
        painter.setClipPath(path);
        painter.setOpacity(1.0 - amount);
        const QRectF& source = d->_snapshotRect;
        const QRectF target(panel.left() + source.left() * scale, panel.top() + source.top() * scale, source.width() * scale, source.height() * scale);
        painter.drawPixmap(target, d->_bodySnapshot, QRectF(QPointF(0.0, 0.0), QSizeF(d->_bodySnapshot.size())));
        painter.restore();
    }

    painter.strokePath(d->innerEdgePath(panel, radius), QPen(shadTheme->getColor(ShadThemeType::Border), ShadDrawerPrivate::kBorderWidth));

    if (d->_isHandleVisible && amount < 1.0)
    {
        const qreal inset = (ShadDrawerPrivate::kBorderWidth + ShadDrawerPrivate::kHandleArea - ShadDrawerPrivate::kHandleThickness) * scale;
        const qreal length = ShadDrawerPrivate::kHandleLength * scale;
        const qreal thickness = ShadDrawerPrivate::kHandleThickness * scale;
        QRectF handle;
        switch (d->_direction)
        {
            case ShadDrawerType::Up:
                handle = QRectF(panel.center().x() - length / 2.0, panel.bottom() - inset - thickness, length, thickness);
                break;
            case ShadDrawerType::Right:
                handle = QRectF(panel.left() + inset, panel.center().y() - length / 2.0, thickness, length);
                break;
            case ShadDrawerType::Down:
                handle = QRectF(panel.center().x() - length / 2.0, panel.top() + inset, length, thickness);
                break;
            case ShadDrawerType::Left:
                handle = QRectF(panel.right() - inset - thickness, panel.center().y() - length / 2.0, thickness, length);
                break;
        }
        painter.setOpacity(1.0 - amount);
        fillRoundedRect(&painter, handle, thickness / 2.0, shadTheme->getColor(ShadThemeType::Muted));
        painter.setOpacity(1.0);
    }

    if (amount > 0.0)
    {
        painter.fillPath(path, QColor(0, 0, 0, qRound(255 * ShadDrawerPrivate::kStackDim * amount)));
    }
}
