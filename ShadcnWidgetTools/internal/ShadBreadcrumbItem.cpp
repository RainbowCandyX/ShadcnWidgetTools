#include "ShadBreadcrumbItem.h"

#include <QColor>
#include <QEnterEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRectF>
#include <QResizeEvent>
#include <QtGlobal>

#include "ShadDropdownMenu.h"
#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // text-sm 行高 20px
    constexpr int kLineHeight = 20;
    // 下拉项文字与 ChevronDown（size-3.5）间距 gap-1；分隔符 size-3.5
    constexpr int kChevronGap = 4;
    constexpr int kSmallIconSize = 14;
    // BreadcrumbEllipsis size-9，图标 size-4；按钮 icon-sm 为 32px
    constexpr int kEllipsisBoxSize = 36;
    constexpr int kEllipsisButtonSize = 32;
    constexpr int kIconSize = 16;
} // namespace

ShadBreadcrumbItem::ShadBreadcrumbItem(Kind kind, QWidget* parent)
    : QWidget(parent), _kind(kind)
{
    _hoverTransition = new ShadTransition(this);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    if (isInteractive())
    {
        setFocusPolicy(Qt::StrongFocus);
        setCursor(Qt::PointingHandCursor);
    }
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadBreadcrumbItem::~ShadBreadcrumbItem()
{
}

ShadBreadcrumbItem::Kind ShadBreadcrumbItem::kind() const
{
    return _kind;
}

bool ShadBreadcrumbItem::isInteractive() const
{
    return _kind == Kind::Link || _kind == Kind::Dropdown || _kind == Kind::EllipsisButton;
}

bool ShadBreadcrumbItem::hasText() const
{
    return _kind == Kind::Link || _kind == Kind::Page || _kind == Kind::Dropdown;
}

void ShadBreadcrumbItem::setText(const QString& text)
{
    if (_text == text)
    {
        return;
    }
    _text = text;
    setAccessibleName(text);
    updateGeometry();
    updateElidedToolTip();
    update();
}

QString ShadBreadcrumbItem::text() const
{
    return _text;
}

void ShadBreadcrumbItem::setShadIcon(ShadIconType::IconName icon)
{
    _icon = icon;
    update();
}

void ShadBreadcrumbItem::setMenu(ShadDropdownMenu* menu)
{
    _menu = menu;
}

QSize ShadBreadcrumbItem::sizeHint() const
{
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    QSize body;
    switch (_kind)
    {
        case Kind::Link:
        case Kind::Page:
            {
                body = QSize(ShadStyleHelper::textWidth(font(), _text), kLineHeight);
                break;
            }
        case Kind::Dropdown:
            {
                body = QSize(ShadStyleHelper::textWidth(font(), _text) + kChevronGap + kSmallIconSize, kLineHeight);
                break;
            }
        case Kind::Separator:
            {
                body = QSize(kSmallIconSize, kLineHeight);
                break;
            }
        case Kind::Ellipsis:
            {
                body = QSize(kEllipsisBoxSize, kEllipsisBoxSize);
                break;
            }
        case Kind::EllipsisButton:
            {
                body = QSize(kEllipsisButtonSize, kEllipsisButtonSize);
                break;
            }
    }
    return body + QSize(margin, margin);
}

QSize ShadBreadcrumbItem::minimumSizeHint() const
{
    const QSize hint = sizeHint();
    if (!hasText())
    {
        return hint;
    }
    // 文字至少保留一个字符加省略号的宽度
    const int minimumText = qMin(ShadStyleHelper::textWidth(font(), _text), ShadStyleHelper::textWidth(font(), QStringLiteral("M…")));
    const int extra = _kind == Kind::Dropdown ? kChevronGap + kSmallIconSize : 0;
    return QSize(minimumText + extra + 2 * ShadStyleHelper::kFocusRingWidth, hint.height());
}

void ShadBreadcrumbItem::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange)
    {
        _hoverTransition->jumpTo(isEnabled() && underMouse() ? 1.0 : 0.0);
    }
    else if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
        updateElidedToolTip();
    }
    QWidget::changeEvent(event);
}

void ShadBreadcrumbItem::enterEvent(QEnterEvent* event)
{
    if (isInteractive() && isEnabled())
    {
        _hoverTransition->animateTo(1.0);
    }
    QWidget::enterEvent(event);
}

void ShadBreadcrumbItem::leaveEvent(QEvent* event)
{
    _hoverTransition->animateTo(0.0);
    QWidget::leaveEvent(event);
}

void ShadBreadcrumbItem::mousePressEvent(QMouseEvent* event)
{
    if (isInteractive() && event->button() == Qt::LeftButton && bodyRect().contains(event->position().toPoint()))
    {
        _isPressed = true;
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadBreadcrumbItem::mouseReleaseEvent(QMouseEvent* event)
{
    if (_isPressed && event->button() == Qt::LeftButton)
    {
        _isPressed = false;
        event->accept();
        if (bodyRect().contains(event->position().toPoint()))
        {
            activate();
        }
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void ShadBreadcrumbItem::keyPressEvent(QKeyEvent* event)
{
    if (isInteractive() && !event->isAutoRepeat())
    {
        // 与 button 一致：回车按下时触发，空格松开时触发
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
        {
            event->accept();
            activate();
            return;
        }
        if (event->key() == Qt::Key_Space)
        {
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void ShadBreadcrumbItem::keyReleaseEvent(QKeyEvent* event)
{
    if (isInteractive() && !event->isAutoRepeat() && event->key() == Qt::Key_Space)
    {
        event->accept();
        activate();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void ShadBreadcrumbItem::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QWidget::focusInEvent(event);
    update();
}

void ShadBreadcrumbItem::focusOutEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QWidget::focusOutEvent(event);
    update();
}

void ShadBreadcrumbItem::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateElidedToolTip();
}

void ShadBreadcrumbItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const QRectF body(bodyRect());
    const Qt::LayoutDirection direction = layoutDirection();
    const qreal progress = _hoverTransition->value();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    const QColor muted = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const bool showFocusRing = hasFocus() && _isFocusVisible;

    switch (_kind)
    {
        case Kind::Separator:
            {
                // 方向性图标（例如 ChevronRight）在 RTL 下水平翻转，对应 rtl:rotate-180
                const QRectF iconRect(body.center().x() - kSmallIconSize / 2.0, body.center().y() - kSmallIconSize / 2.0, kSmallIconSize, kSmallIconSize);
                painter.save();
                if (direction == Qt::RightToLeft)
                {
                    const QPointF center = iconRect.center();
                    painter.translate(center);
                    painter.scale(-1.0, 1.0);
                    painter.translate(-center);
                }
                ShadIcon::paint(&painter, iconRect, _icon, muted, kSmallIconSize);
                painter.restore();
                return;
            }
        case Kind::Ellipsis:
        case Kind::EllipsisButton:
            {
                const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
                QColor iconColor = muted;
                if (_kind == Kind::EllipsisButton)
                {
                    // ghost 按钮：hover:bg-accent hover:text-accent-foreground（暗色 hover:bg-accent/50）
                    const QColor accent = shadTheme->getColor(ShadThemeType::Accent);
                    fillRoundedRect(&painter, body, radius, withAlpha(shadTheme->isDark() ? withAlpha(accent, 0.5) : accent, progress));
                    iconColor = mix(muted, shadTheme->getColor(ShadThemeType::AccentForeground), progress);
                    if (showFocusRing)
                    {
                        drawFocusRing(&painter, body, radius, focusRingColor(false));
                    }
                }
                const QRectF iconRect(body.center().x() - kIconSize / 2.0, body.center().y() - kIconSize / 2.0, kIconSize, kIconSize);
                ShadIcon::paint(&painter, iconRect, ShadIconType::Ellipsis, iconColor, kIconSize);
                return;
            }
        case Kind::Link:
        case Kind::Page:
        case Kind::Dropdown:
            {
                break;
            }
    }

    // 链接与下拉项 transition-colors hover:text-foreground；当前页 text-foreground
    const QColor color = _kind == Kind::Page ? foreground : mix(muted, foreground, progress);
    if (showFocusRing)
    {
        drawFocusRing(&painter, body, shadTheme->getRadius(ShadThemeType::Small), focusRingColor(false));
    }
    const int textWidth = textAvailableWidth();
    const QRectF logicalText(body.left(), body.top(), textWidth, body.height());
    painter.setFont(font());
    painter.setPen(color);
    painter.drawText(visualRect(direction, body, logicalText), visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), elideText(font(), _text, textWidth));
    if (_kind == Kind::Dropdown)
    {
        const QRectF chevronRect(body.right() - kSmallIconSize, body.center().y() - kSmallIconSize / 2.0, kSmallIconSize, kSmallIconSize);
        ShadIcon::paint(&painter, visualRect(direction, body, chevronRect), ShadIconType::ChevronDown, color, kSmallIconSize);
    }
}

QRect ShadBreadcrumbItem::bodyRect() const
{
    const int margin = ShadStyleHelper::kFocusRingWidth;
    return rect().adjusted(margin, margin, -margin, -margin);
}

int ShadBreadcrumbItem::textAvailableWidth() const
{
    const int extra = _kind == Kind::Dropdown ? kChevronGap + kSmallIconSize : 0;
    return qMax(0, bodyRect().width() - extra);
}

void ShadBreadcrumbItem::updateElidedToolTip()
{
    if (!hasText())
    {
        return;
    }
    // 被省略时悬停显示完整文字
    const bool isElided = ShadStyleHelper::textWidth(font(), _text) > textAvailableWidth();
    setToolTip(isElided ? _text : QString());
}

void ShadBreadcrumbItem::activate()
{
    // 先弹出菜单再发信号：信号处理中可能重建面包屑并删除本项
    if (!_menu.isNull() && (_kind == Kind::Dropdown || _kind == Kind::EllipsisButton))
    {
        if (_menu->isVisible())
        {
            _menu->hide();
        }
        else
        {
            _menu->popup(this, Qt::AlignLeft);
        }
    }
    Q_EMIT clicked();
}
