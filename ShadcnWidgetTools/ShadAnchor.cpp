#include "ShadAnchor.h"

#include <QAbstractScrollArea>
#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QRectF>
#include <QScrollBar>
#include <QSize>
#include <QSizePolicy>
#include <QString>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadAnchorPrivate.h"
#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

ShadAnchor::ShadAnchor(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadAnchorPrivate())
{
    Q_D(ShadAnchor);
    d->q_ptr = this;
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, ShadAnchorPrivate::kFontPixelSize, QFont::Normal);

    d->_indicatorAnimation = new QVariantAnimation(this);
    d->_indicatorAnimation->setDuration(ShadAnchorPrivate::kIndicatorDurationMs);
    d->_indicatorAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_indicatorAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_indicatorPosition = value.toReal();
        update();
    });
    d->_scrollAnimation = new QVariantAnimation(this);
    d->_scrollAnimation->setEasingCurve(QEasingCurve::InOutCubic);
    connect(d->_scrollAnimation, &QVariantAnimation::valueChanged, this, [d](const QVariant& value)
    {
        if (d->_scrollArea.isNull())
        {
            return;
        }
        d->_isAnimatingScroll = true;
        d->_scrollArea->verticalScrollBar()->setValue(value.toInt());
        d->_isAnimatingScroll = false;
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadAnchor::~ShadAnchor()
{
    Q_D(ShadAnchor);
    d->_indicatorAnimation->stop();
    QObject::disconnect(d->_indicatorAnimation, nullptr, this, nullptr);
    QObject::disconnect(d->_scrollAnimation, nullptr, this, nullptr);
    d->disconnectScrollArea();
}

void ShadAnchor::setScrollArea(QAbstractScrollArea* scrollArea)
{
    Q_D(ShadAnchor);
    if (d->_scrollArea == scrollArea)
    {
        return;
    }
    d->disconnectScrollArea();
    d->_scrollArea = scrollArea;
    d->connectScrollArea();
    d->updateFromScroll();
}

QAbstractScrollArea* ShadAnchor::getScrollArea() const
{
    Q_D(const ShadAnchor);
    return d->_scrollArea.data();
}

int ShadAnchor::addLink(const QString& title, QWidget* target, int level)
{
    Q_D(ShadAnchor);
    ShadAnchorPrivate::Link link;
    link.title = title;
    link.target = target;
    link.level = qMax(0, level);
    d->_links.append(link);
    updateGeometry();
    update();
    d->updateFromScroll();
    return int(d->_links.size()) - 1;
}

void ShadAnchor::setLinkTitle(int index, const QString& title)
{
    Q_D(ShadAnchor);
    if (index < 0 || index >= d->_links.size() || d->_links.at(index).title == title)
    {
        return;
    }
    d->_links[index].title = title;
    updateGeometry();
    update();
}

QString ShadAnchor::getLinkTitle(int index) const
{
    Q_D(const ShadAnchor);
    if (index < 0 || index >= d->_links.size())
    {
        return QString();
    }
    return d->_links.at(index).title;
}

QWidget* ShadAnchor::getLinkTarget(int index) const
{
    Q_D(const ShadAnchor);
    if (index < 0 || index >= d->_links.size())
    {
        return nullptr;
    }
    return d->_links.at(index).target.data();
}

int ShadAnchor::count() const
{
    Q_D(const ShadAnchor);
    return int(d->_links.size());
}

void ShadAnchor::clear()
{
    Q_D(ShadAnchor);
    d->_scrollAnimation->stop();
    d->_links.clear();
    d->_hoverIndex = -1;
    d->_pressedIndex = -1;
    d->_focusIndex = 0;
    unsetCursor();
    d->setCurrent(-1);
    updateGeometry();
    update();
}

void ShadAnchor::setCurrentIndex(int index)
{
    Q_D(ShadAnchor);
    if (index < 0 || index >= d->_links.size())
    {
        d->_scrollAnimation->stop();
        d->setCurrent(-1);
        return;
    }
    d->_focusIndex = index;
    d->setCurrent(index);
    d->scrollToLink(index);
}

int ShadAnchor::getCurrentIndex() const
{
    Q_D(const ShadAnchor);
    return d->_currentIndex;
}

void ShadAnchor::setOffset(int offset)
{
    Q_D(ShadAnchor);
    if (d->_offset == offset)
    {
        return;
    }
    d->_offset = offset;
    d->updateFromScroll();
}

int ShadAnchor::getOffset() const
{
    Q_D(const ShadAnchor);
    return d->_offset;
}

void ShadAnchor::setDuration(int durationMs)
{
    Q_D(ShadAnchor);
    d->_durationMs = qMax(0, durationMs);
}

int ShadAnchor::getDuration() const
{
    Q_D(const ShadAnchor);
    return d->_durationMs;
}

QSize ShadAnchor::sizeHint() const
{
    Q_D(const ShadAnchor);
    const QFont font = d->textFont();
    int textWidth = 0;
    for (const ShadAnchorPrivate::Link& link : d->_links)
    {
        textWidth = qMax(textWidth, link.level * ShadAnchorPrivate::kLevelIndent + ShadStyleHelper::textWidth(font, link.title));
    }
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const int width = 2 * margin + ShadAnchorPrivate::kRailWidth + ShadAnchorPrivate::kTextPadding + textWidth + ShadAnchorPrivate::kEndPadding;
    return QSize(width, 2 * margin + int(d->_links.size()) * ShadAnchorPrivate::kRowHeight);
}

QSize ShadAnchor::minimumSizeHint() const
{
    Q_D(const ShadAnchor);
    const int margin = ShadStyleHelper::kFocusRingWidth;
    const int width = 2 * margin + ShadAnchorPrivate::kRailWidth + ShadAnchorPrivate::kTextPadding + ShadAnchorPrivate::kMinTextWidth + ShadAnchorPrivate::kEndPadding;
    return QSize(width, 2 * margin + int(d->_links.size()) * ShadAnchorPrivate::kRowHeight);
}

bool ShadAnchor::event(QEvent* event)
{
    Q_D(ShadAnchor);
    if (event->type() == QEvent::Leave)
    {
        d->_hoverIndex = -1;
        unsetCursor();
        update();
    }
    else if (event->type() == QEvent::Show)
    {
        d->updateFromScroll();
    }
    return QWidget::event(event);
}

bool ShadAnchor::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadAnchor);
    if (!d->_scrollArea.isNull() && watched == d->_scrollArea->viewport() && event->type() == QEvent::Resize)
    {
        d->updateFromScroll();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadAnchor::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
        update();
    }
    else if (event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadAnchor::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadAnchor);
    const int index = d->indexAt(event->position().toPoint());
    if (index != d->_hoverIndex)
    {
        d->_hoverIndex = index;
        if (index >= 0)
        {
            setCursor(Qt::PointingHandCursor);
        }
        else
        {
            unsetCursor();
        }
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ShadAnchor::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadAnchor);
    if (event->button() == Qt::LeftButton)
    {
        d->_pressedIndex = d->indexAt(event->position().toPoint());
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadAnchor::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadAnchor);
    if (event->button() == Qt::LeftButton)
    {
        const int index = d->indexAt(event->position().toPoint());
        const bool isClick = index >= 0 && index == d->_pressedIndex;
        d->_pressedIndex = -1;
        if (isClick)
        {
            d->activate(index);
        }
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadAnchor::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadAnchor);
    const int lastIndex = int(d->_links.size()) - 1;
    if (lastIndex < 0)
    {
        QWidget::keyPressEvent(event);
        return;
    }
    int focusIndex = qBound(0, d->_focusIndex, lastIndex);
    switch (event->key())
    {
        case Qt::Key_Up:
            focusIndex = qMax(0, focusIndex - 1);
            break;
        case Qt::Key_Down:
            focusIndex = qMin(lastIndex, focusIndex + 1);
            break;
        case Qt::Key_Home:
            focusIndex = 0;
            break;
        case Qt::Key_End:
            focusIndex = lastIndex;
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            d->_isFocusVisible = true;
            d->activate(focusIndex);
            event->accept();
            return;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    d->_focusIndex = focusIndex;
    d->_isFocusVisible = true;
    update();
    event->accept();
}

void ShadAnchor::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadAnchor);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    if (event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason)
    {
        d->_focusIndex = qMax(0, d->_currentIndex);
    }
    update();
    QWidget::focusInEvent(event);
}

void ShadAnchor::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadAnchor);
    if (event->reason() != Qt::PopupFocusReason && event->reason() != Qt::ActiveWindowFocusReason)
    {
        d->_isFocusVisible = false;
    }
    update();
    QWidget::focusOutEvent(event);
}

void ShadAnchor::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadAnchor);
    using namespace ShadStyleHelper;
    const int linkCount = int(d->_links.size());
    if (linkCount == 0)
    {
        return;
    }
    const Qt::LayoutDirection direction = layoutDirection();
    const QRectF bounds(rect());
    const int margin = kFocusRingWidth;
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    const QRectF railRect(margin, margin, ShadAnchorPrivate::kRailWidth, linkCount * ShadAnchorPrivate::kRowHeight);
    painter.fillRect(visualRect(direction, bounds, railRect), shadTheme->getColor(ShadThemeType::Border));
    if (d->_currentIndex >= 0)
    {
        const QRectF indicatorRect(margin, margin + d->_indicatorPosition * ShadAnchorPrivate::kRowHeight + ShadAnchorPrivate::kIndicatorInset, ShadAnchorPrivate::kRailWidth, ShadAnchorPrivate::kRowHeight - 2 * ShadAnchorPrivate::kIndicatorInset);
        fillRoundedRect(&painter, visualRect(direction, bounds, indicatorRect), ShadAnchorPrivate::kRailWidth / 2.0, shadTheme->getColor(ShadThemeType::Primary));
    }

    const QFont font = d->textFont();
    painter.setFont(font);
    const QColor mutedColor = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor activeColor = shadTheme->getColor(ShadThemeType::Foreground);
    const Qt::Alignment alignment = visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter);
    for (int index = 0; index < linkCount; ++index)
    {
        const QRectF textRect = d->textRect(index);
        const bool isActive = index == d->_currentIndex || (index == d->_hoverIndex && isEnabled());
        painter.setPen(isActive ? activeColor : mutedColor);
        painter.drawText(visualRect(direction, bounds, textRect), alignment, elideText(font, d->_links.at(index).title, textRect.width()));
    }
    if (hasFocus() && d->_isFocusVisible && d->_focusIndex >= 0 && d->_focusIndex < linkCount)
    {
        const QRectF focusRect = d->rowRect(d->_focusIndex).adjusted(ShadAnchorPrivate::kRailWidth + ShadAnchorPrivate::kTextPadding / 2.0, 0, 0, 0);
        drawFocusRing(&painter, visualRect(direction, bounds, focusRect), shadTheme->getRadius(ShadThemeType::Small), focusRingColor(false));
    }
}
