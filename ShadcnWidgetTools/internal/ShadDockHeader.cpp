#include "ShadDockHeader.h"

#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QHelpEvent>
#include <QIcon>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QSizePolicy>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTooltip.h"
#include "ShadTransition.h"

namespace
{
    constexpr int kHeaderHeight = 36;
    constexpr int kPaddingX = 10;
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 8;
    constexpr int kCloseSize = 20;
    constexpr int kCloseIconSize = 14;
    constexpr int kCloseInset = 8;
    constexpr int kCloseGap = 4;
    constexpr int kFontPixelSize = 13;
    constexpr qreal kCloseHoverAlpha = 0.1;
    constexpr qreal kClosePressedAlpha = 0.15;
} // namespace

ShadDockHeader::ShadDockHeader(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setMouseTracking(true);
    setProperty(ShadStyleHelper::kCustomToolTipProperty, true);
    _activeTransition = new ShadTransition(this);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadDockHeader::~ShadDockHeader()
{
}

void ShadDockHeader::setTitle(ShadIconType::IconName icon, const QString& title)
{
    _icon = icon;
    _title = title;
    update();
}

void ShadDockHeader::setCloseVisible(bool visible)
{
    if (_isCloseVisible == visible)
    {
        return;
    }
    _isCloseVisible = visible;
    setCloseHover(false);
    update();
}

void ShadDockHeader::setDragEnabled(bool enabled)
{
    _isDragEnabled = enabled;
}

void ShadDockHeader::setActive(bool active)
{
    if (_isActive == active)
    {
        return;
    }
    _isActive = active;
    _activeTransition->animateTo(active ? 1.0 : 0.0);
}

QSize ShadDockHeader::sizeHint() const
{
    return QSize(2 * kPaddingX + kIconSize + kIconGap + kCloseSize, kHeaderHeight);
}

QSize ShadDockHeader::minimumSizeHint() const
{
    return QSize(kCloseInset + kCloseSize, kHeaderHeight);
}

bool ShadDockHeader::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip)
    {
        const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        if (_isCloseVisible && closeRect().contains(helpEvent->pos()))
        {
            shadTooltip->showText(this, closeRect().toAlignedRect(), tr("关闭"));
            return true;
        }
        event->ignore();
        return false;
    }
    return QWidget::event(event);
}

void ShadDockHeader::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    const QPoint pos = event->position().toPoint();
    if (_isCloseVisible && closeRect().contains(pos))
    {
        _isClosePressed = true;
        update();
    }
    else
    {
        _isTitlePressed = true;
        _pressPos = pos;
    }
    event->accept();
}

void ShadDockHeader::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint pos = event->position().toPoint();
    if (_isTitlePressed && event->buttons().testFlag(Qt::LeftButton))
    {
        if (_isDragEnabled && (pos - _pressPos).manhattanLength() >= QApplication::startDragDistance())
        {
            _isTitlePressed = false;
            event->accept();
            Q_EMIT dragRequested(_pressPos);
        }
        return;
    }
    setCloseHover(_isCloseVisible && closeRect().contains(pos));
    QWidget::mouseMoveEvent(event);
}

void ShadDockHeader::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const bool isCloseClick = _isClosePressed && _isCloseVisible && closeRect().contains(event->position().toPoint());
    _isClosePressed = false;
    _isTitlePressed = false;
    update();
    event->accept();
    if (isCloseClick)
    {
        Q_EMIT closeClicked();
    }
}

void ShadDockHeader::leaveEvent(QEvent* event)
{
    setCloseHover(false);
    QWidget::leaveEvent(event);
}

void ShadDockHeader::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadDockHeader::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const qreal active = _activeTransition->value();
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor mutedForeground = shadTheme->getColor(ShadThemeType::MutedForeground);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    if (!isEnabled())
    {
        painter.setOpacity(kDisabledOpacity);
    }
    if (active > 0.0)
    {
        painter.fillRect(rect(), withAlpha(shadTheme->getColor(ShadThemeType::Accent), active));
    }
    QFont font = this->font();
    font.setPixelSize(kFontPixelSize);
    font.setWeight(QFont::Medium);
    const QColor textColor = mix(mutedForeground, foreground, active);
    QRectF content(kPaddingX, 0.0, width() - 2 * kPaddingX, height());
    if (_isCloseVisible)
    {
        content.setRight(width() - kCloseInset - kCloseSize - kCloseGap);
    }
    if (content.width() >= kIconSize)
    {
        drawIconText(&painter, visualRect(layoutDirection(), QRectF(rect()), content), _icon, QIcon(), _title, font, textColor, kIconGap, Qt::AlignLeft | Qt::AlignVCenter, kIconSize, 0.0, layoutDirection());
    }
    if (_isCloseVisible)
    {
        const QRectF area = closeRect();
        if (_isCloseHover)
        {
            fillRoundedRect(&painter, area, shadTheme->getRadius(ShadThemeType::Small), withAlpha(foreground, _isClosePressed ? kClosePressedAlpha : kCloseHoverAlpha));
        }
        drawIconText(&painter, area, ShadIconType::X, QIcon(), QString(), font, _isCloseHover ? foreground : textColor, 0.0, Qt::AlignCenter, kCloseIconSize, 0.0, layoutDirection());
    }
}

QRectF ShadDockHeader::closeRect() const
{
    const QRectF logicalRect(width() - kCloseInset - kCloseSize, (height() - kCloseSize) / 2.0, kCloseSize, kCloseSize);
    return ShadStyleHelper::visualRect(layoutDirection(), QRectF(rect()), logicalRect);
}

void ShadDockHeader::setCloseHover(bool hover)
{
    if (_isCloseHover == hover)
    {
        return;
    }
    _isCloseHover = hover;
    update();
}
