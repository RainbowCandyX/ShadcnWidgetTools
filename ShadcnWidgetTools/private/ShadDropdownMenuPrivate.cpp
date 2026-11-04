#include "ShadDropdownMenuPrivate.h"

#include <QAction>
#include <QColor>
#include <QCoreApplication>
#include <QEvent>
#include <QFont>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QPointer>
#include <QRectF>
#include <QScreen>
#include <QStringList>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>
#include <QtGlobal>

#include <utility>

#include "ShadComboBoxSearch.h"
#include "ShadDropdownMenu.h"
#include "ShadIcon.h"
#include "ShadPlacement.h"
#include "ShadSidebarMenuButton.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // p-1、1px ring；min-w-32，菜单栏 min-w-36，子菜单 min-w-[96px]
    constexpr int kPadding = 4;
    constexpr int kBorderWidth = 1;
    constexpr int kMinWidth = 128;
    constexpr int kMenubarMinWidth = 144;
    constexpr int kSubmenuMinWidth = 96;
    // 菜单项 px-2 py-1.5；分组标题 py-1.5；分隔线 h-px my-1
    constexpr int kItemHeight = 32;
    constexpr int kLabelHeight = 28;
    constexpr int kMenubarLabelHeight = 32;
    constexpr int kSeparatorHeight = 9;
    constexpr int kItemPaddingX = 8;
    // data-inset:pl-8
    constexpr int kInsetPadding = 32;
    // 图标 size-4，与文字间距 gap-2；快捷键与文字之间至少间隔 gap-2
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 8;
    constexpr int kShortcutGap = 8;
    // 勾选标记：pr-8 right-2 size-3.5，菜单栏 pl-8 left-2 size-4
    constexpr int kCheckablePaddingRight = 32;
    constexpr int kIndicatorOffset = 8;
    constexpr int kIndicatorSize = 14;
    constexpr int kMenubarIndicatorSize = 16;
    constexpr int kCheckIconSize = 16;
    constexpr int kSmallFontSize = 12;
    constexpr int kMenubarLabelFontSize = 14;
    // 快捷键 tracking-widest（0.1em）
    constexpr qreal kShortcutLetterSpacing = 1.2;
    // 子菜单 alignOffset=-3
    constexpr int kSubmenuAlignOffset = -3;
    constexpr int kSubmenuDelayMs = 100;

    // text-xs font-medium，菜单栏 text-sm
    QFont labelFont(const QFont& baseFont, bool isMenubarStyle)
    {
        QFont font = baseFont;
        font.setPixelSize(isMenubarStyle ? kMenubarLabelFontSize : kSmallFontSize);
        font.setWeight(QFont::Medium);
        return font;
    }

    // 快捷键字体：text-xs tracking-widest
    QFont shortcutFont(const QFont& baseFont)
    {
        QFont font = baseFont;
        font.setPixelSize(kSmallFontSize);
        font.setWeight(QFont::Normal);
        font.setLetterSpacing(QFont::AbsoluteSpacing, kShortcutLetterSpacing);
        return font;
    }

    // 去掉 QAction 文字中的助记符 &，&& 还原为 &
    QString displayText(const QAction* action)
    {
        const QString text = action->text();
        QString result;
        result.reserve(text.size());
        for (int index = 0; index < int(text.size()); ++index)
        {
            if (text.at(index) == QLatin1Char('&') && index + 1 < int(text.size()))
            {
                ++index;
            }
            result.append(text.at(index));
        }
        return result;
    }
} // namespace

ShadDropdownMenuPrivate::ShadDropdownMenuPrivate()
{
}

ShadDropdownMenuPrivate::~ShadDropdownMenuPrivate()
{
}

bool ShadDropdownMenuPrivate::isVisibleEntry(int index) const
{
    if (index < 0 || index >= int(_entries.size()))
    {
        return false;
    }
    const Entry& entry = _entries.at(index);
    if (entry.kind == EntryKind::Widget)
    {
        return !entry.widget.isNull() && !entry.widget->isHidden();
    }
    return entry.action == nullptr || entry.action->isVisible();
}

bool ShadDropdownMenuPrivate::isSelectable(int index) const
{
    if (!isVisibleEntry(index))
    {
        return false;
    }
    const Entry& entry = _entries.at(index);
    const bool isItem = entry.kind == EntryKind::Item || entry.kind == EntryKind::CheckableItem || entry.kind == EntryKind::RadioItem || entry.kind == EntryKind::Submenu;
    return isItem && entry.action != nullptr && entry.action->isEnabled();
}

int ShadDropdownMenuPrivate::entryHeight(int index) const
{
    const Entry& entry = _entries.at(index);
    switch (entry.kind)
    {
        case EntryKind::Label:
            return _isMenubarStyle ? kMenubarLabelHeight : kLabelHeight;
        case EntryKind::Widget:
            return entry.widget.isNull() ? 0 : entry.widget->sizeHint().height();
        case EntryKind::Separator:
            return kSeparatorHeight;
        case EntryKind::Item:
        case EntryKind::CheckableItem:
        case EntryKind::RadioItem:
        case EntryKind::Submenu:
            break;
    }
    return kItemHeight;
}

QRect ShadDropdownMenuPrivate::cardRect() const
{
    Q_Q(const ShadDropdownMenu);
    // 从锚点一侧滑入（slide-in-from-top-2 等）
    constexpr int kSlideDistance = 8;
    const int slide = qRound((1.0 - _openProgress) * kSlideDistance);
    QRect card = q->rect().adjusted(kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);
    switch (_side)
    {
        case ShadPlacementType::Top:
            card.translate(0, slide);
            break;
        case ShadPlacementType::Left:
            card.translate(slide, 0);
            break;
        case ShadPlacementType::Right:
            card.translate(-slide, 0);
            break;
        case ShadPlacementType::Bottom:
            card.translate(0, -slide);
            break;
    }
    return card;
}

QRect ShadDropdownMenuPrivate::entryRect(int index) const
{
    if (!isVisibleEntry(index))
    {
        return QRect();
    }
    const QRect card = cardRect();
    int top = card.top() + kBorderWidth + kPadding;
    for (int previous = 0; previous < index; ++previous)
    {
        if (isVisibleEntry(previous))
        {
            top += entryHeight(previous);
        }
    }
    const int inset = kBorderWidth + kPadding;
    return QRect(card.left() + inset, top, card.width() - 2 * inset, entryHeight(index));
}

int ShadDropdownMenuPrivate::entryAt(const QPoint& pos) const
{
    for (int index = 0; index < int(_entries.size()); ++index)
    {
        if (entryRect(index).contains(pos))
        {
            return index;
        }
    }
    return -1;
}

QSize ShadDropdownMenuPrivate::cardSize() const
{
    Q_Q(const ShadDropdownMenu);
    using ShadStyleHelper::textWidth;
    const QFont itemFont = q->font();
    const QFont itemLabelFont = labelFont(q->font(), _isMenubarStyle);
    const QFont itemShortcutFont = shortcutFont(q->font());
    int contentWidth = 0;
    int contentHeight = 0;
    for (int index = 0; index < int(_entries.size()); ++index)
    {
        if (!isVisibleEntry(index))
        {
            continue;
        }
        const Entry& entry = _entries.at(index);
        contentHeight += entryHeight(index);
        int width = 0;
        switch (entry.kind)
        {
            case EntryKind::Item:
            case EntryKind::Submenu:
                {
                    width = (entry.isInset ? kInsetPadding : kItemPaddingX) + textWidth(itemFont, displayText(entry.action)) + kItemPaddingX;
                    if (entry.icon != ShadIconType::None)
                    {
                        width += kIconSize + kIconGap;
                    }
                    if (entry.kind == EntryKind::Item && !entry.shortcutText.isEmpty())
                    {
                        width += kShortcutGap + textWidth(itemShortcutFont, entry.shortcutText);
                    }
                    if (entry.kind == EntryKind::Submenu)
                    {
                        width += kShortcutGap + kIconSize;
                    }
                    break;
                }
            case EntryKind::CheckableItem:
            case EntryKind::RadioItem:
                {
                    const int start = (_isMenubarStyle || entry.isInset) ? kInsetPadding : kItemPaddingX;
                    const int end = _isMenubarStyle ? kItemPaddingX : kCheckablePaddingRight;
                    width = start + textWidth(itemFont, displayText(entry.action)) + end;
                    break;
                }
            case EntryKind::Label:
                {
                    width = (entry.isInset ? kInsetPadding : kItemPaddingX) + textWidth(itemLabelFont, entry.labelText) + kItemPaddingX;
                    break;
                }
            case EntryKind::Widget:
                {
                    width = entry.widget->sizeHint().width();
                    break;
                }
            case EntryKind::Separator:
                {
                    break;
                }
        }
        contentWidth = qMax(contentWidth, width);
    }
    const int chrome = 2 * (kBorderWidth + kPadding);
    const int anchorWidth = _isMatchAnchorWidth ? _anchorWidth : 0;
    const bool isSubmenu = !_parentMenu.isNull();
    const int minimumWidth = _isMenubarStyle ? ((isSubmenu || _isContextMenu) ? kMinWidth : kMenubarMinWidth) : (isSubmenu ? kSubmenuMinWidth : kMinWidth);
    return QSize(qMax(qMax(minimumWidth, anchorWidth), contentWidth + chrome), contentHeight + chrome);
}

void ShadDropdownMenuPrivate::layoutWidgets()
{
    for (int index = 0; index < int(_entries.size()); ++index)
    {
        const Entry& entry = _entries.at(index);
        if (entry.kind == EntryKind::Widget && isVisibleEntry(index))
        {
            entry.widget->setGeometry(entryRect(index));
        }
    }
}

void ShadDropdownMenuPrivate::setHighlight(int index)
{
    Q_Q(ShadDropdownMenu);
    const int validIndex = isSelectable(index) ? index : -1;
    if (_highlightIndex == validIndex)
    {
        return;
    }
    _highlightIndex = validIndex;
    q->update();
}

void ShadDropdownMenuPrivate::moveHighlight(int step)
{
    const int count = int(_entries.size());
    if (count == 0)
    {
        return;
    }
    const int direction = step > 0 ? 1 : -1;
    int index = _highlightIndex;
    if (index < 0)
    {
        index = direction > 0 ? -1 : count;
    }
    for (int attempt = 0; attempt < count; ++attempt)
    {
        index = ((index + direction) % count + count) % count;
        if (isSelectable(index))
        {
            setHighlight(index);
            return;
        }
    }
}

void ShadDropdownMenuPrivate::highlightEdge(bool isFirst)
{
    const int count = int(_entries.size());
    for (int step = 0; step < count; ++step)
    {
        const int index = isFirst ? step : count - 1 - step;
        if (isSelectable(index))
        {
            setHighlight(index);
            return;
        }
    }
}

void ShadDropdownMenuPrivate::activate(int index, bool isKeyboard)
{
    Q_Q(ShadDropdownMenu);
    if (!isSelectable(index))
    {
        return;
    }
    const EntryKind kind = _entries.at(index).kind;
    QAction* action = _entries.at(index).action;
    if (kind == EntryKind::Submenu)
    {
        openSubmenu(index, isKeyboard);
        return;
    }
    if (kind == EntryKind::CheckableItem || kind == EntryKind::RadioItem)
    {
        action->trigger();
        q->update();
        return;
    }
    rootMenu()->hide();
    q->hide();
    Q_EMIT q->itemTriggered(action);
    action->trigger();
}

void ShadDropdownMenuPrivate::hoverEntry(int index)
{
    const bool isSubmenuOpen = !_openSubmenu.isNull() && _openSubmenu->isVisible();
    if (!isSelectable(index))
    {
        if (!isSubmenuOpen)
        {
            setHighlight(-1);
        }
        if (_pendingSubmenuIndex >= 0)
        {
            _submenuTimer->stop();
        }
        return;
    }
    setHighlight(index);
    if (isSubmenuOpen && _openSubmenuIndex == index)
    {
        _submenuTimer->stop();
        return;
    }
    const bool isSubmenu = _entries.at(index).kind == EntryKind::Submenu;
    _pendingSubmenuIndex = isSubmenu ? index : -1;
    if (isSubmenu || isSubmenuOpen)
    {
        _submenuTimer->start(kSubmenuDelayMs);
    }
    else
    {
        _submenuTimer->stop();
    }
}

void ShadDropdownMenuPrivate::openSubmenu(int index, bool highlightFirst)
{
    Q_Q(ShadDropdownMenu);
    _submenuTimer->stop();
    _pendingSubmenuIndex = -1;
    if (!isSelectable(index) || _entries.at(index).kind != EntryKind::Submenu || _entries.at(index).submenu.isNull())
    {
        return;
    }
    ShadDropdownMenu* submenu = _entries.at(index).submenu;
    ShadDropdownMenuPrivate * submenuPrivate = submenu->d_func();
    if (_openSubmenu == submenu && submenu->isVisible())
    {
        if (highlightFirst)
        {
            submenuPrivate->highlightEdge(true);
        }
        return;
    }
    closeSubmenu();
    _openSubmenu = submenu;
    _openSubmenuIndex = index;
    setHighlight(index);
    const Qt::LayoutDirection direction = q->layoutDirection();
    submenu->setLayoutDirection(direction);
    submenuPrivate->_anchorWidth = 0;
    const QRect row = entryRect(index);
    const QRect anchor(q->mapToGlobal(row.topLeft()), row.size());
    const ShadPlacement::Result placement = ShadPlacement::place(anchor, submenuPrivate->cardSize(), ShadPlacementType::Right, ShadPlacementType::Start, 0, direction);
    submenuPrivate->showAt(placement.rect.topLeft() + QPoint(0, kSubmenuAlignOffset), placement.side);
    if (highlightFirst)
    {
        submenuPrivate->highlightEdge(true);
    }
    q->update();
}

void ShadDropdownMenuPrivate::closeSubmenu()
{
    Q_Q(ShadDropdownMenu);
    _submenuTimer->stop();
    _pendingSubmenuIndex = -1;
    if (_openSubmenu.isNull())
    {
        return;
    }
    ShadDropdownMenu* submenu = _openSubmenu;
    _openSubmenu = nullptr;
    _openSubmenuIndex = -1;
    submenu->hide();
    q->update();
}

bool ShadDropdownMenuPrivate::forwardToParent(QMouseEvent* event)
{
    Q_Q(ShadDropdownMenu);
    if (_parentMenu.isNull() || cardRect().contains(event->position().toPoint()))
    {
        return false;
    }
    const QPointer<ShadDropdownMenu> parent = _parentMenu;
    const QPointF globalPos = event->globalPosition();
    QMouseEvent forwarded(event->type(), parent->mapFromGlobal(globalPos), globalPos, event->button(), event->buttons(), event->modifiers());
    QCoreApplication::sendEvent(parent, &forwarded);
    if (event->type() == QEvent::MouseButtonPress && !parent.isNull() && parent->testAttribute(Qt::WA_NoMouseReplay))
    {
        q->setAttribute(Qt::WA_NoMouseReplay);
    }
    return true;
}

ShadDropdownMenu* ShadDropdownMenuPrivate::rootMenu()
{
    Q_Q(ShadDropdownMenu);
    ShadDropdownMenu* menu = q;
    while (!menu->d_func()->_parentMenu.isNull())
    {
        menu = menu->d_func()->_parentMenu;
    }
    return menu;
}

void ShadDropdownMenuPrivate::setMenubarStyle(bool isMenubarStyle)
{
    _isMenubarStyle = isMenubarStyle;
    for (const Entry& entry : std::as_const(_entries))
    {
        if (!entry.submenu.isNull())
        {
            entry.submenu->d_func()->setMenubarStyle(isMenubarStyle);
        }
    }
}

void ShadDropdownMenuPrivate::handleTypeahead(const QString& text)
{
    if (!_typeaheadTimer.isValid() || _typeaheadTimer.elapsed() > ShadComboBoxSearch::kTypeaheadTimeoutMs)
    {
        _typeaheadBuffer.clear();
    }
    _typeaheadBuffer += text;
    _typeaheadTimer.restart();
    QStringList texts;
    texts.reserve(_entries.size());
    for (int index = 0; index < int(_entries.size()); ++index)
    {
        texts.append(isSelectable(index) ? displayText(_entries.at(index).action) : QString());
    }
    const int index = ShadComboBoxSearch::typeaheadIndex(texts, _typeaheadBuffer, _highlightIndex);
    if (index >= 0)
    {
        setHighlight(index);
    }
}

void ShadDropdownMenuPrivate::showAt(const QPoint& cardTopLeft, ShadPlacementType::Side side)
{
    Q_Q(ShadDropdownMenu);
    _side = side;
    _highlightIndex = -1;
    _typeaheadBuffer.clear();
    const QSize card = cardSize();
    q->setGeometry(cardTopLeft.x() - kShadowMargin, cardTopLeft.y() - kShadowMargin, card.width() + 2 * kShadowMargin, card.height() + 2 * kShadowMargin);
    _openAnimation->stop();
    _openProgress = 0.0;
    layoutWidgets();
    q->setWindowOpacity(0.0);
    q->show();
    q->raise();
    q->setFocus(Qt::PopupFocusReason);
    _openAnimation->start();
}

QRect ShadDropdownMenuPrivate::anchorRect(const QWidget* anchor)
{
    const int focusMargin = qobject_cast<const ShadSidebarMenuButton*>(anchor) != nullptr ? 0 : ShadStyleHelper::kFocusRingWidth;
    const QRect localRect = anchor->rect().adjusted(focusMargin, focusMargin, -focusMargin, -focusMargin);
    return QRect(anchor->mapToGlobal(localRect.topLeft()), localRect.size());
}

void ShadDropdownMenuPrivate::popupForRect(const QRect& anchorRect, Qt::Alignment alignment, Qt::LayoutDirection direction)
{
    Q_Q(ShadDropdownMenu);
    // 弹出窗口不会继承父组件的布局方向，这里显式同步
    q->setLayoutDirection(direction);
    _anchorWidth = anchorRect.width();
    const QSize card = cardSize();
    const QScreen* screen = QGuiApplication::screenAt(anchorRect.center());
    const QRect available = screen != nullptr ? screen->availableGeometry() : QRect();

    bool isAbove = false;
    // 逻辑对齐转为物理对齐：RTL 下起始侧为右侧
    const bool isRightAligned = ShadStyleHelper::visualAlignment(direction, alignment).testFlag(Qt::AlignRight);
    int x = isRightAligned ? anchorRect.right() + 1 - card.width() : anchorRect.left();
    if (available.isValid())
    {
        // 下方放不下且上方空间更大时显示在上方
        const int spaceBelow = available.bottom() - kScreenMargin - anchorRect.bottom() - kSideOffset;
        const int spaceAbove = anchorRect.top() - kSideOffset - available.top() - kScreenMargin;
        isAbove = card.height() > spaceBelow && spaceAbove > spaceBelow;
        x = qBound(available.left() + kScreenMargin, x, available.right() + 1 - kScreenMargin - card.width());
    }
    const int y = isAbove ? anchorRect.top() - kSideOffset - card.height() : anchorRect.bottom() + 1 + kSideOffset;
    _anchor = nullptr;
    showAt(QPoint(x, y), isAbove ? ShadPlacementType::Top : ShadPlacementType::Bottom);
}

Qt::LayoutDirection ShadDropdownMenuPrivate::fallbackDirection() const
{
    Q_Q(const ShadDropdownMenu);
    const QWidget* parent = q->parentWidget();
    return parent != nullptr ? parent->layoutDirection() : QGuiApplication::layoutDirection();
}

void ShadDropdownMenuPrivate::paintEntry(QPainter* painter, int index) const
{
    Q_Q(const ShadDropdownMenu);
    using namespace ShadStyleHelper;
    const Entry& entry = _entries.at(index);
    const QRect rowRect = entryRect(index);
    switch (entry.kind)
    {
        case EntryKind::Separator:
            {
                // -mx-1 抵消内边距，分隔线横跨整个卡片
                const QRect card = cardRect();
                painter->fillRect(QRect(card.left() + kBorderWidth, rowRect.top() + (kSeparatorHeight - 1) / 2, card.width() - 2 * kBorderWidth, 1), shadTheme->getColor(ShadThemeType::Border));
                return;
            }
        case EntryKind::Label:
            {
                const QFont font = labelFont(q->font(), _isMenubarStyle);
                const Qt::LayoutDirection direction = q->layoutDirection();
                const QRect textRect = visualRect(direction, rowRect, rowRect.adjusted(entry.isInset ? kInsetPadding : kItemPaddingX, 0, -kItemPaddingX, 0));
                painter->setFont(font);
                painter->setPen(shadTheme->getColor(_isMenubarStyle ? ShadThemeType::PopoverForeground : ShadThemeType::MutedForeground));
                painter->drawText(textRect, visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), elideText(font, entry.labelText, textRect.width()));
                return;
            }
        case EntryKind::Widget:
            {
                // 展示组件自己绘制
                return;
            }
        case EntryKind::Item:
        case EntryKind::CheckableItem:
        case EntryKind::RadioItem:
        case EntryKind::Submenu:
            {
                break;
            }
    }

    // data-open:bg-accent
    const bool isSubmenuOpen = entry.kind == EntryKind::Submenu && !_openSubmenu.isNull() && _openSubmenu->isVisible() && _openSubmenuIndex == index;
    const bool isHighlighted = index == _highlightIndex || isSubmenuOpen;
    painter->save();
    painter->setOpacity(entry.action->isEnabled() ? 1.0 : kDisabledOpacity);
    // 高亮：focus:bg-accent；危险项 focus:bg-destructive/10（暗色 /20），文字保持 destructive 色
    if (isHighlighted)
    {
        const QColor highlight = entry.isDestructive ? withAlpha(shadTheme->getColor(ShadThemeType::Destructive), shadTheme->isDark() ? 0.2 : 0.1) : shadTheme->getColor(ShadThemeType::Accent);
        fillRoundedRect(painter, QRectF(rowRect), shadTheme->getRadius(ShadThemeType::Small), highlight);
    }
    QColor textColor = shadTheme->getColor(isHighlighted ? ShadThemeType::AccentForeground : ShadThemeType::PopoverForeground);
    if (entry.isDestructive)
    {
        textColor = shadTheme->getColor(ShadThemeType::Destructive);
    }
    // left / right 为逻辑位置，RTL 时镜像
    const Qt::LayoutDirection direction = q->layoutDirection();
    const bool isCheckable = entry.kind == EntryKind::CheckableItem || entry.kind == EntryKind::RadioItem;
    int left = rowRect.left() + ((entry.isInset || (isCheckable && _isMenubarStyle)) ? kInsetPadding : kItemPaddingX);
    int right = rowRect.right() + 1 - ((isCheckable && !_isMenubarStyle) ? kCheckablePaddingRight : kItemPaddingX);
    if (!isCheckable && entry.icon != ShadIconType::None)
    {
        const QRectF iconRect(left, rowRect.top() + (rowRect.height() - kIconSize) / 2, kIconSize, kIconSize);
        ShadIcon::paint(painter, visualRect(direction, QRectF(rowRect), iconRect), entry.icon, textColor, kIconSize);
        left += kIconSize + kIconGap;
    }
    if (entry.kind == EntryKind::Submenu)
    {
        const QRectF chevronRect(right - kIconSize, rowRect.top() + (rowRect.height() - kIconSize) / 2, kIconSize, kIconSize);
        const ShadIconType::IconName chevron = direction == Qt::RightToLeft ? ShadIconType::ChevronLeft : ShadIconType::ChevronRight;
        ShadIcon::paint(painter, visualRect(direction, QRectF(rowRect), chevronRect), chevron, textColor, kIconSize);
        right -= kIconSize + kShortcutGap;
    }
    if (entry.kind == EntryKind::Item && !entry.shortcutText.isEmpty())
    {
        // 快捷键位于结束侧（ml-auto），高亮时与文字同色
        const QFont font = shortcutFont(q->font());
        const int shortcutWidth = textWidth(font, entry.shortcutText);
        const QColor shortcutColor = isHighlighted && !entry.isDestructive ? shadTheme->getColor(ShadThemeType::AccentForeground) : shadTheme->getColor(ShadThemeType::MutedForeground);
        painter->setFont(font);
        painter->setPen(shortcutColor);
        painter->drawText(visualRect(direction, rowRect, QRect(right - shortcutWidth, rowRect.top(), shortcutWidth, rowRect.height())), visualAlignment(direction, Qt::AlignRight | Qt::AlignVCenter), entry.shortcutText);
        right -= shortcutWidth + kShortcutGap;
    }
    const QRect textRect = visualRect(direction, rowRect, QRect(left, rowRect.top(), qMax(0, right - left), rowRect.height()));
    painter->setFont(q->font());
    painter->setPen(textColor);
    painter->drawText(textRect, visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), elideText(q->font(), displayText(entry.action), textRect.width()));
    if (isCheckable && entry.action->isChecked())
    {
        const int indicatorSize = _isMenubarStyle ? kMenubarIndicatorSize : kIndicatorSize;
        const qreal indicatorLeft = _isMenubarStyle ? rowRect.left() + kIndicatorOffset : rowRect.right() + 1 - kIndicatorOffset - indicatorSize;
        const QRectF indicatorRect(indicatorLeft, rowRect.top() + (rowRect.height() - indicatorSize) / 2.0, indicatorSize, indicatorSize);
        ShadIcon::paint(painter, visualRect(direction, QRectF(rowRect), indicatorRect), ShadIconType::Check, textColor, kCheckIconSize);
    }
    painter->restore();
}
