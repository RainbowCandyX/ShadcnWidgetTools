#include "ShadCommandList.h"

#include <QAction>
#include <QColor>
#include <QFont>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 分组 p-1，对话框中 px-2
    constexpr int kGroupPadding = 4;
    constexpr int kLargeGroupPaddingX = 8;
    // 分组标题 px-2 py-1.5 text-xs
    constexpr int kHeadingHeight = 28;
    constexpr int kHeadingFontSize = 12;
    // 菜单项 px-2 py-1.5，对话框中 py-3
    constexpr int kItemHeight = 32;
    constexpr int kLargeItemHeight = 44;
    constexpr int kItemPaddingX = 8;
    constexpr int kIconSize = 16;
    constexpr int kLargeIconSize = 20;
    constexpr int kGap = 8;
    // 快捷键 text-xs tracking-widest
    constexpr int kShortcutFontSize = 12;
    constexpr qreal kShortcutLetterSpacing = 1.2;
    // 空状态 py-6 text-sm
    constexpr int kEmptyHeight = 68;
} // namespace

ShadCommandList::ShadCommandList(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadCommandList::~ShadCommandList()
{
}

void ShadCommandList::setRows(const QList<ShadCommandRow>& rows)
{
    _rows = rows;
    _highlightIndex = -1;
    updateRowTops();
    updateGeometry();
    update();
}

const QList<ShadCommandRow>& ShadCommandList::getRows() const
{
    return _rows;
}

void ShadCommandList::setEmptyText(const QString& text)
{
    _emptyText = text;
    update();
}

void ShadCommandList::setLarge(bool large)
{
    if (_isLarge == large)
    {
        return;
    }
    _isLarge = large;
    updateRowTops();
    updateGeometry();
    update();
}

void ShadCommandList::setHighlightIndex(int index)
{
    const int validIndex = isSelectable(index) ? index : -1;
    if (_highlightIndex == validIndex)
    {
        return;
    }
    _highlightIndex = validIndex;
    update();
}

int ShadCommandList::getHighlightIndex() const
{
    return _highlightIndex;
}

void ShadCommandList::moveHighlight(int step)
{
    const int count = int(_rows.size());
    const int direction = step > 0 ? 1 : -1;
    int index = _highlightIndex;
    if (index < 0)
    {
        index = direction > 0 ? -1 : count;
    }
    for (index += direction; index >= 0 && index < count; index += direction)
    {
        if (isSelectable(index))
        {
            setHighlightIndex(index);
            return;
        }
    }
}

void ShadCommandList::highlightFirst()
{
    for (int index = 0; index < int(_rows.size()); ++index)
    {
        if (isSelectable(index))
        {
            setHighlightIndex(index);
            return;
        }
    }
    setHighlightIndex(-1);
}

void ShadCommandList::highlightLast()
{
    for (int index = int(_rows.size()) - 1; index >= 0; --index)
    {
        if (isSelectable(index))
        {
            setHighlightIndex(index);
            return;
        }
    }
    setHighlightIndex(-1);
}

bool ShadCommandList::isSelectable(int index) const
{
    if (index < 0 || index >= int(_rows.size()))
    {
        return false;
    }
    const ShadCommandRow& row = _rows.at(index);
    return row.kind == ShadCommandRow::Kind::Item && row.action != nullptr && row.action->isEnabled();
}

QRect ShadCommandList::rowRect(int index) const
{
    if (index < 0 || index >= int(_rows.size()))
    {
        return QRect();
    }
    if (_rows.at(index).kind == ShadCommandRow::Kind::Separator)
    {
        return QRect(0, _rowTops.at(index), width(), rowHeight(index));
    }
    return QRect(paddingX(), _rowTops.at(index), width() - 2 * paddingX(), rowHeight(index));
}

int ShadCommandList::itemHeight() const
{
    return _isLarge ? kLargeItemHeight : kItemHeight;
}

QSize ShadCommandList::sizeHint() const
{
    using ShadStyleHelper::textWidth;
    if (!hasItems())
    {
        return QSize(textWidth(font(), _emptyText) + 2 * kItemPaddingX, kEmptyHeight);
    }
    QFont headingFont = font();
    headingFont.setPixelSize(kHeadingFontSize);
    headingFont.setWeight(QFont::Medium);
    QFont shortcutFont = font();
    shortcutFont.setPixelSize(kShortcutFontSize);
    shortcutFont.setLetterSpacing(QFont::AbsoluteSpacing, kShortcutLetterSpacing);
    int contentWidth = 0;
    for (const ShadCommandRow& row : _rows)
    {
        int width = 0;
        if (row.kind == ShadCommandRow::Kind::Heading)
        {
            width = textWidth(headingFont, row.text);
        }
        else if (row.kind == ShadCommandRow::Kind::Item)
        {
            width = textWidth(font(), row.text);
            if (row.icon != ShadIconType::None)
            {
                width += iconSize() + kGap;
            }
            if (!row.shortcutText.isEmpty())
            {
                width += kGap + textWidth(shortcutFont, row.shortcutText);
            }
        }
        contentWidth = qMax(contentWidth, width);
    }
    return QSize(contentWidth + 2 * kItemPaddingX + 2 * paddingX(), _rowTops.last());
}

void ShadCommandList::mouseMoveEvent(QMouseEvent* event)
{
    const int index = indexAt(event->position().toPoint());
    if (isSelectable(index))
    {
        setHighlightIndex(index);
    }
    QWidget::mouseMoveEvent(event);
}

void ShadCommandList::mousePressEvent(QMouseEvent* event)
{
    event->accept();
}

void ShadCommandList::mouseReleaseEvent(QMouseEvent* event)
{
    const int index = indexAt(event->position().toPoint());
    if (event->button() == Qt::LeftButton && isSelectable(index))
    {
        Q_EMIT itemClicked(index);
    }
    event->accept();
}

void ShadCommandList::paintEvent(QPaintEvent* event)
{
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    if (!hasItems())
    {
        painter.setFont(font());
        painter.setPen(shadTheme->getColor(ShadThemeType::PopoverForeground));
        painter.drawText(rect(), Qt::AlignCenter, _emptyText);
        return;
    }
    const Qt::LayoutDirection direction = layoutDirection();
    const qreal radius = shadTheme->getRadius(ShadThemeType::Small);
    const QColor mutedColor = shadTheme->getColor(ShadThemeType::MutedForeground);
    QFont headingFont = font();
    headingFont.setPixelSize(kHeadingFontSize);
    headingFont.setWeight(QFont::Medium);
    QFont shortcutFont = font();
    shortcutFont.setPixelSize(kShortcutFontSize);
    shortcutFont.setLetterSpacing(QFont::AbsoluteSpacing, kShortcutLetterSpacing);
    for (int index = 0; index < int(_rows.size()); ++index)
    {
        const ShadCommandRow& row = _rows.at(index);
        const QRect bounds = rowRect(index);
        if (!bounds.intersects(event->rect()))
        {
            continue;
        }
        switch (row.kind)
        {
            case ShadCommandRow::Kind::Separator:
                {
                    painter.fillRect(bounds, shadTheme->getColor(ShadThemeType::Border));
                    break;
                }
            case ShadCommandRow::Kind::Heading:
                {
                    const QRect textRect = bounds.adjusted(kItemPaddingX, 0, -kItemPaddingX, 0);
                    painter.setFont(headingFont);
                    painter.setPen(mutedColor);
                    painter.drawText(textRect, visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), elideText(headingFont, row.text, textRect.width()));
                    break;
                }
            case ShadCommandRow::Kind::Item:
                {
                    const bool isHighlighted = index == _highlightIndex;
                    painter.setOpacity(row.action != nullptr && row.action->isEnabled() ? 1.0 : kDisabledOpacity);
                    // data-[selected=true]:bg-accent
                    if (isHighlighted)
                    {
                        fillRoundedRect(&painter, QRectF(bounds), radius, shadTheme->getColor(ShadThemeType::Accent));
                    }
                    int left = bounds.left() + kItemPaddingX;
                    int right = bounds.right() + 1 - kItemPaddingX;
                    if (row.icon != ShadIconType::None)
                    {
                        const int size = iconSize();
                        const QRectF iconRect(left, bounds.top() + (bounds.height() - size) / 2.0, size, size);
                        ShadIcon::paint(&painter, visualRect(direction, QRectF(bounds), iconRect), row.icon, mutedColor, size);
                        left += size + kGap;
                    }
                    if (!row.shortcutText.isEmpty())
                    {
                        const int shortcutWidth = textWidth(shortcutFont, row.shortcutText);
                        painter.setFont(shortcutFont);
                        painter.setPen(mutedColor);
                        painter.drawText(visualRect(direction, bounds, QRect(right - shortcutWidth, bounds.top(), shortcutWidth, bounds.height())), visualAlignment(direction, Qt::AlignRight | Qt::AlignVCenter), row.shortcutText);
                        right -= shortcutWidth + kGap;
                    }
                    const QRect textRect = visualRect(direction, bounds, QRect(left, bounds.top(), qMax(0, right - left), bounds.height()));
                    painter.setFont(font());
                    painter.setPen(shadTheme->getColor(isHighlighted ? ShadThemeType::AccentForeground : ShadThemeType::Foreground));
                    painter.drawText(textRect, visualAlignment(direction, Qt::AlignLeft | Qt::AlignVCenter), elideText(font(), row.text, textRect.width()));
                    painter.setOpacity(1.0);
                    break;
                }
            case ShadCommandRow::Kind::GroupTop:
            case ShadCommandRow::Kind::GroupBottom:
                {
                    break;
                }
        }
    }
}

bool ShadCommandList::hasItems() const
{
    for (const ShadCommandRow& row : _rows)
    {
        if (row.kind == ShadCommandRow::Kind::Item)
        {
            return true;
        }
    }
    return false;
}

int ShadCommandList::rowHeight(int index) const
{
    const ShadCommandRow& row = _rows.at(index);
    switch (row.kind)
    {
        case ShadCommandRow::Kind::GroupTop:
            return (_isLarge && !row.isFirstGroup) ? 0 : kGroupPadding;
        case ShadCommandRow::Kind::Heading:
            return kHeadingHeight;
        case ShadCommandRow::Kind::Item:
            return itemHeight();
        case ShadCommandRow::Kind::GroupBottom:
            return kGroupPadding;
        case ShadCommandRow::Kind::Separator:
            return 1;
    }
    return 0;
}

int ShadCommandList::paddingX() const
{
    return _isLarge ? kLargeGroupPaddingX : kGroupPadding;
}

int ShadCommandList::iconSize() const
{
    return _isLarge ? kLargeIconSize : kIconSize;
}

int ShadCommandList::indexAt(const QPoint& pos) const
{
    for (int index = 0; index < int(_rows.size()); ++index)
    {
        if (pos.y() >= _rowTops.at(index) && pos.y() < _rowTops.at(index + 1))
        {
            return rowRect(index).contains(pos) ? index : -1;
        }
    }
    return -1;
}

void ShadCommandList::updateRowTops()
{
    _rowTops.clear();
    _rowTops.reserve(_rows.size() + 1);
    int top = 0;
    for (int index = 0; index < int(_rows.size()); ++index)
    {
        _rowTops.append(top);
        top += rowHeight(index);
    }
    _rowTops.append(top);
}
