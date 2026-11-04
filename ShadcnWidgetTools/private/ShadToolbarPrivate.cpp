#include "ShadToolbarPrivate.h"

#include <QAction>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadDef.h"
#include "ShadDropdownMenu.h"
#include "ShadIconType.h"
#include "ShadStyleHelper.h"
#include "ShadToolbar.h"
#include "ShadToolbarButton.h"

ShadToolbarPrivate::ShadToolbarPrivate()
{
}

ShadToolbarPrivate::~ShadToolbarPrivate()
{
}

bool ShadToolbarPrivate::isHorizontal() const
{
    return _orientation == Qt::Horizontal;
}

ShadToolbarButton* ShadToolbarPrivate::createButton(QAction* action, ShadIconType::IconName icon, ShadDropdownMenu* menu)
{
    Q_Q(ShadToolbar);
    ShadToolbarButton* button = new ShadToolbarButton(action, icon, menu, q);
    configureButton(button);
    button->hide();
    QObject::connect(button, &ShadToolbarButton::contentChanged, q, [this]() { requestLayout(); });
    return button;
}

void ShadToolbarPrivate::configureButton(ShadToolbarButton* button) const
{
    const bool horizontal = isHorizontal();
    button->setButtonStyle(_buttonStyle);
    button->setStartAligned(!horizontal && _buttonStyle != ShadToolbarType::IconOnly);
    button->setMenuSide(horizontal ? ShadPlacementType::Bottom : ShadPlacementType::Right);
}

void ShadToolbarPrivate::addActionItem(QAction* action, ShadIconType::IconName icon, bool isOwned)
{
    Q_Q(ShadToolbar);
    Item item;
    item.kind = ItemKind::Action;
    item.action = action;
    item.isOwnedAction = isOwned;
    item.button = createButton(action, icon, nullptr);
    q->QWidget::addAction(action);
    if (!isOwned)
    {
        QObject::connect(action, &QObject::destroyed, q, [this]() { removeDeadItems(); });
    }
    appendItem(item);
}

void ShadToolbarPrivate::appendItem(const Item& item)
{
    _items.append(item);
    requestLayout();
}

int ShadToolbarPrivate::indexOfAction(const QAction* action) const
{
    for (int i = 0; i < _items.size(); ++i)
    {
        if (_items.at(i).kind == ItemKind::Action && _items.at(i).action == action)
        {
            return i;
        }
    }
    return -1;
}

void ShadToolbarPrivate::removeItemAt(int index)
{
    Q_Q(ShadToolbar);
    if (index < 0 || index >= _items.size())
    {
        return;
    }
    const Item item = _items.takeAt(index);
    if (item.button != nullptr)
    {
        QObject::disconnect(item.button, nullptr, q, nullptr);
        delete item.button;
    }
    if (!item.action.isNull())
    {
        QObject::disconnect(item.action, nullptr, q, nullptr);
        q->QWidget::removeAction(item.action);
        if (item.isOwnedAction)
        {
            delete item.action.data();
        }
    }
    if (!item.menu.isNull())
    {
        delete item.menu.data();
    }
    if (!item.widget.isNull())
    {
        QObject::disconnect(item.widget, nullptr, q, nullptr);
        ShadStyleHelper::clearFocusWithin(item.widget);
        delete item.widget.data();
    }
    requestLayout();
}

void ShadToolbarPrivate::removeDeadItems()
{
    for (int i = _items.size() - 1; i >= 0; --i)
    {
        const Item& item = _items.at(i);
        const bool isDeadAction = (item.kind == ItemKind::Action || item.kind == ItemKind::Menu) && item.action.isNull();
        const bool isDeadWidget = item.kind == ItemKind::Widget && item.widget.isNull();
        if (isDeadAction || isDeadWidget)
        {
            removeItemAt(i);
        }
    }
}

bool ShadToolbarPrivate::isItemShown(const Item& item) const
{
    switch (item.kind)
    {
        case ItemKind::Action:
        case ItemKind::Menu:
            return !item.action.isNull() && item.action->isVisible();
        case ItemKind::Widget:
            return !item.widget.isNull();
        case ItemKind::Separator:
        case ItemKind::Stretch:
            return true;
    }
    return false;
}

int ShadToolbarPrivate::itemLength(const Item& item) const
{
    const bool horizontal = isHorizontal();
    switch (item.kind)
    {
        case ItemKind::Action:
        case ItemKind::Menu:
            {
                const QSize hint = item.button->sizeHint();
                return horizontal ? hint.width() : hint.height();
            }
        case ItemKind::Widget:
            {
                const QSize hint = item.widget->sizeHint().expandedTo(item.widget->minimumSizeHint()).expandedTo(QSize(0, 0));
                return horizontal ? hint.width() : hint.height();
            }
        case ItemKind::Separator:
            return 1 + 2 * kSeparatorMargin;
        case ItemKind::Stretch:
            return 0;
    }
    return 0;
}

int ShadToolbarPrivate::crossLength() const
{
    const bool horizontal = isHorizontal();
    const QSize overflowHint = _overflowButton->sizeHint();
    int cross = horizontal ? overflowHint.height() : overflowHint.width();
    for (const Item& item : _items)
    {
        if (!isItemShown(item))
        {
            continue;
        }
        QSize hint;
        if (item.button != nullptr)
        {
            hint = item.button->sizeHint();
        }
        else if (!item.widget.isNull())
        {
            hint = item.widget->sizeHint().expandedTo(item.widget->minimumSizeHint());
        }
        cross = qMax(cross, horizontal ? hint.height() : hint.width());
    }
    return cross;
}

void ShadToolbarPrivate::requestLayout()
{
    Q_Q(ShadToolbar);
    q->updateGeometry();
    layoutItems();
}

void ShadToolbarPrivate::layoutItems()
{
    Q_Q(ShadToolbar);
    const bool horizontal = isHorizontal();
    const QRect bounds = q->rect();
    const QRect contents = bounds.adjusted(kPadding, kPadding, -kPadding, -kPadding);
    const int available = horizontal ? contents.width() : contents.height();
    const int cross = horizontal ? contents.height() : contents.width();
    const Qt::LayoutDirection direction = q->layoutDirection();

    QList<int> shown;
    QList<int> lengths;
    int total = 0;
    int fixedCount = 0;
    int stretchCount = 0;
    for (int i = 0; i < _items.size(); ++i)
    {
        Item& item = _items[i];
        item.separatorRect = QRect();
        if (!isItemShown(item))
        {
            if (item.button != nullptr)
            {
                item.button->hide();
            }
            continue;
        }
        const int length = itemLength(item);
        shown.append(i);
        lengths.append(length);
        if (item.kind == ItemKind::Stretch)
        {
            ++stretchCount;
            continue;
        }
        total += length;
        ++fixedCount;
    }
    total += kSpacing * qMax(0, fixedCount - 1);

    int visibleEnd = shown.size();
    _overflowStart = -1;
    bool hasOverflowEntry = false;
    const QSize overflowHint = _overflowButton->sizeHint();
    const int overflowLength = horizontal ? overflowHint.width() : overflowHint.height();
    if (total > available)
    {
        int used = 0;
        bool hasPlaced = false;
        visibleEnd = 0;
        for (int p = 0; p < shown.size(); ++p)
        {
            if (_items.at(shown.at(p)).kind == ItemKind::Stretch)
            {
                visibleEnd = p + 1;
                continue;
            }
            const int next = used + (hasPlaced ? kSpacing : 0) + lengths.at(p);
            if (next + kSpacing + overflowLength > available)
            {
                break;
            }
            used = next;
            hasPlaced = true;
            visibleEnd = p + 1;
        }
        if (visibleEnd < shown.size())
        {
            _overflowStart = shown.at(visibleEnd);
            for (int p = visibleEnd; p < shown.size(); ++p)
            {
                const ItemKind kind = _items.at(shown.at(p)).kind;
                if (kind == ItemKind::Action || kind == ItemKind::Menu)
                {
                    hasOverflowEntry = true;
                    break;
                }
            }
        }
        while (visibleEnd > 0 && (_items.at(shown.at(visibleEnd - 1)).kind == ItemKind::Separator || _items.at(shown.at(visibleEnd - 1)).kind == ItemKind::Stretch))
        {
            --visibleEnd;
        }
    }
    const int extra = _overflowStart < 0 && stretchCount > 0 ? qMax(0, available - total) : 0;

    int pos = horizontal ? contents.left() : contents.top();
    bool hasPlaced = false;
    int stretchIndex = 0;
    for (int p = 0; p < shown.size(); ++p)
    {
        Item& item = _items[shown.at(p)];
        QWidget* widget = item.button != nullptr ? static_cast<QWidget*>(item.button) : item.widget.data();
        if (p >= visibleEnd)
        {
            if (widget != nullptr)
            {
                widget->hide();
            }
            continue;
        }
        if (item.kind == ItemKind::Stretch)
        {
            if (extra > 0)
            {
                const int share = extra / stretchCount + (stretchIndex < extra % stretchCount ? 1 : 0);
                pos += share;
            }
            ++stretchIndex;
            continue;
        }
        if (hasPlaced)
        {
            pos += kSpacing;
        }
        hasPlaced = true;
        const int length = lengths.at(p);
        if (item.kind == ItemKind::Separator)
        {
            if (horizontal)
            {
                const QRect logical(pos + kSeparatorMargin, contents.top() + (cross - kSeparatorLength) / 2, 1, kSeparatorLength);
                item.separatorRect = ShadStyleHelper::visualRect(direction, bounds, logical);
            }
            else
            {
                item.separatorRect = QRect(contents.left(), pos + kSeparatorMargin, contents.width(), 1);
            }
        }
        else if (widget != nullptr)
        {
            QRect geometry;
            if (horizontal)
            {
                const int height = item.button != nullptr ? item.button->sizeHint().height() : qMin(qMax(widget->sizeHint().height(), widget->minimumSizeHint().height()), cross);
                geometry = ShadStyleHelper::visualRect(direction, bounds, QRect(pos, contents.top() + (cross - height) / 2, length, height));
            }
            else
            {
                const int width = item.button != nullptr ? cross : qMin(qMax(widget->sizeHint().width(), widget->minimumSizeHint().width()), cross);
                geometry = QRect(contents.left() + (cross - width) / 2, pos, width, length);
            }
            widget->setGeometry(geometry);
            widget->show();
        }
        pos += length;
    }

    if (hasOverflowEntry)
    {
        QRect geometry;
        if (horizontal)
        {
            geometry = ShadStyleHelper::visualRect(direction, bounds, QRect(contents.right() + 1 - overflowLength, contents.top() + (cross - overflowHint.height()) / 2, overflowLength, overflowHint.height()));
        }
        else
        {
            geometry = QRect(contents.left(), contents.bottom() + 1 - overflowLength, cross, overflowLength);
        }
        _overflowButton->setGeometry(geometry);
        _overflowButton->show();
        _overflowButton->raise();
    }
    else
    {
        _overflowButton->hide();
    }
    q->update();
}

void ShadToolbarPrivate::rebuildOverflowMenu()
{
    Q_Q(ShadToolbar);
    _overflowMenu->clear();
    if (_overflowStart < 0)
    {
        return;
    }
    bool hasEntry = false;
    bool isSeparatorPending = false;
    for (int i = _overflowStart; i < _items.size(); ++i)
    {
        const Item& item = _items.at(i);
        if (!isItemShown(item))
        {
            continue;
        }
        if (item.kind == ItemKind::Separator)
        {
            isSeparatorPending = hasEntry;
            continue;
        }
        if (item.kind != ItemKind::Action && item.kind != ItemKind::Menu)
        {
            continue;
        }
        if (isSeparatorPending)
        {
            _overflowMenu->addSeparator();
            isSeparatorPending = false;
        }
        QAction* source = item.action.data();
        QAction* mirror = nullptr;
        if (item.kind == ItemKind::Action && source->isCheckable())
        {
            mirror = _overflowMenu->addCheckableItem(source->text(), source->isChecked());
        }
        else
        {
            mirror = _overflowMenu->addItem(source->text(), item.button->getShadIcon());
        }
        mirror->setEnabled(source->isEnabled());
        if (item.kind == ItemKind::Action)
        {
            const QPointer<QAction> target = source;
            QObject::connect(mirror, &QAction::triggered, q, [target]()
            {
                if (!target.isNull())
                {
                    target->trigger();
                }
            });
        }
        else
        {
            const QPointer<ShadDropdownMenu> menu = item.menu;
            QObject::connect(mirror, &QAction::triggered, q, [this, menu]()
            {
                QTimer::singleShot(0, q_ptr, [this, menu]()
                {
                    if (!menu.isNull() && _overflowButton->isVisible())
                    {
                        menu->popup(_overflowButton, isHorizontal() ? ShadPlacementType::Bottom : ShadPlacementType::Right, ShadPlacementType::Start);
                    }
                });
            });
        }
        hasEntry = true;
    }
}

void ShadToolbarPrivate::disconnectItems()
{
    Q_Q(ShadToolbar);
    for (const Item& item : std::as_const(_items))
    {
        if (item.button != nullptr)
        {
            QObject::disconnect(item.button, nullptr, q, nullptr);
        }
        if (!item.action.isNull())
        {
            QObject::disconnect(item.action, nullptr, q, nullptr);
        }
        if (!item.widget.isNull())
        {
            QObject::disconnect(item.widget, nullptr, q, nullptr);
        }
    }
    QObject::disconnect(_overflowButton, nullptr, q, nullptr);
}
